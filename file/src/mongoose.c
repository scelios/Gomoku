#include "../include/gomoku.h"
#include <cjson/cJSON.h>

// Broadcast board state to all connected clients
static void broadcast_board_state(struct mg_mgr *mgr, game *gameData, screen *windows)
{
    if (!mgr) return;  // Safety check
    
    cJSON *json = cJSON_CreateObject();
    
    // Create board array
    cJSON *board_array = cJSON_CreateArray();
    for (int i = 0; i < MAX_BOARD; i++)
        cJSON_AddNumberToObject(board_array, "", gameData->board[i]);
    
    cJSON_AddItemToObject(json, "board", board_array);
    cJSON_AddNumberToObject(json, "turn", gameData->turn);
    cJSON_AddNumberToObject(json, "captures_p1", gameData->captures[P1]);
    cJSON_AddNumberToObject(json, "captures_p2", gameData->captures[P2]);
    cJSON_AddBoolToObject(json, "game_over", gameData->game_over);
    cJSON_AddNumberToObject(json, "hint_idx", gameData->hint_idx);
    
    char *json_str = cJSON_Print(json);
    
    // Send to all connected websockets
    for (struct mg_connection *c = mgr->conns; c != NULL; c = c->next)
    {
        if (c->is_websocket)
            mg_ws_send(c, json_str, strlen(json_str), WEBSOCKET_OP_TEXT);
    }
    
    free(json_str);
    cJSON_Delete(json);
}

// ✅ PUBLIC FUNCTION: Can be called from other files
void broadcast_board_state_external(struct mg_mgr *mgr, game *gameData, screen *windows)
{
    broadcast_board_state(mgr, gameData, windows);
}

// Handle received messages from client
static void handle_client_message(struct mg_connection *c, const char *msg, both *args)
{
    cJSON *json = cJSON_Parse(msg);
    if (!json) return;
    
    cJSON *action = cJSON_GetObjectItem(json, "action");
    if (!action || action->type != cJSON_String)
    {
        cJSON_Delete(json);
        return;
    }
    
    // Action: "play" - Play a move
    if (strcmp(action->valuestring, "play") == 0)
    {
        cJSON *idx = cJSON_GetObjectItem(json, "index");
        if (idx && idx->type == cJSON_Number)
        {
            int index = idx->valueint;
            
            // Verification
            if (index >= 0 && index < MAX_BOARD && 
                args->gameData->board[index] == EMPTY && 
                !args->gameData->game_over &&
                !isIaTurn(args->gameData->iaTurn, args->gameData->turn))
            {
                // Place piece
                args->gameData->board[index] = args->gameData->turn;
                int x = GET_X(index);
                int y = GET_Y(index);
                
                drawSquare(args->windows, x, y, args->gameData->turn);
                checkPieceCapture(args->gameData, args->windows, x, y);
                
                args->gameData->turn = (args->gameData->turn == P1) ? P2 : P1;
                args->gameData->hint_idx = -1;
                args->windows->changed = true;
                
                // Broadcast updated state
                broadcast_board_state(args->mgr, args->gameData, args->windows);
            }
        }
    }
    
    // Action: "reset" - Reset the game
    else if (strcmp(action->valuestring, "reset") == 0)
    {
        resetGame(args->gameData, args->windows);
        broadcast_board_state(args->mgr, args->gameData, args->windows);
    }
    
    // Action: "hint" - Request a hint
    else if (strcmp(action->valuestring, "hint") == 0)
    {
        if (!args->gameData->game_over)
            suggest_move(args->gameData, args->windows, args->gameData->turn);
        
        broadcast_board_state(args->mgr, args->gameData, args->windows);
    }
    
    // Action: "toggle_ia" - Enable/Disable AI
    else if (strcmp(action->valuestring, "toggle_ia") == 0)
    {
        args->gameData->iaTurn = (args->gameData->iaTurn == 0) ? P2 : 0;
        broadcast_board_state(args->mgr, args->gameData, args->windows);
    }
    
    cJSON_Delete(json);
}

// Main websocket handler
void websocket_handler(struct mg_connection *c, int ev, void *ev_data)
{
    both *args = (both *)c->mgr->userdata;
    
    if (ev == MG_EV_HTTP_MSG)  // ✅ Handle HTTP upgrade request
    {
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;
        
        #ifdef DEBUG
            printf("📥 HTTP request received, upgrading to WebSocket\n");
        #endif
        
        // Upgrade to WebSocket
        mg_ws_upgrade(c, hm, NULL);
        
        // Mark connection as WebSocket
        c->data[0] = 'W';
    }
    else if (ev == MG_EV_WS_OPEN)  // ✅ WebSocket connection established
    {
        #ifdef DEBUG
            printf("✅ Websocket client connected\n");
            printf("📤 Sending initial board state to new client\n");
        #endif
        
        // Send current board state to the newly connected client
        cJSON *json = cJSON_CreateObject();
        
        cJSON *board_array = cJSON_CreateArray();
        for (int i = 0; i < MAX_BOARD; i++)
            cJSON_AddNumberToObject(board_array, "", args->gameData->board[i]);
        
        cJSON_AddItemToObject(json, "board", board_array);
        cJSON_AddNumberToObject(json, "turn", args->gameData->turn);
        cJSON_AddNumberToObject(json, "captures_p1", args->gameData->captures[P1]);
        cJSON_AddNumberToObject(json, "captures_p2", args->gameData->captures[P2]);
        cJSON_AddBoolToObject(json, "game_over", args->gameData->game_over);
        cJSON_AddNumberToObject(json, "hint_idx", args->gameData->hint_idx);
        
        char *json_str = cJSON_Print(json);
        
        #ifdef DEBUG
            printf("📤 Sending: %s\n", json_str);
        #endif
        
        // Send to this specific client
        mg_ws_send(c, json_str, strlen(json_str), WEBSOCKET_OP_TEXT);
        
        free(json_str);
        cJSON_Delete(json);
    }
    else if (ev == MG_EV_WS_MSG)
    {
        struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;
        
        // Convert to null-terminated string
        char msg_buffer[1024];
        int len = wm->data.len;
        if (len > 1023) len = 1023;
        memcpy(msg_buffer, wm->data.buf, len);
        msg_buffer[len] = '\0';
        
        #ifdef DEBUG
            printf("📨 Message received: %s\n", msg_buffer);
        #endif
        
        handle_client_message(c, msg_buffer, args);
    }
    else if (ev == MG_EV_CLOSE)
    {
        #ifdef DEBUG
            printf("❌ Websocket client disconnected\n");
        #endif
    }
}

// Initialize Mongoose websocket server
void init_websocket(both *args)
{
    args->mgr = (struct mg_mgr *)malloc(sizeof(struct mg_mgr));
    if (!args->mgr)
    {
        perror("malloc failed for mg_mgr");
        return;
    }
    
    mg_mgr_init(args->mgr);
    args->mgr->userdata = args;

    // ✅ Listen on HTTP (not ws://)
    struct mg_connection *c = mg_http_listen(args->mgr, "http://0.0.0.0:8000", 
                                              websocket_handler, NULL);
    
    if (c == NULL)
    {
        #ifdef DEBUG
            fprintf(stderr, "❌ Error: Failed to start server\n");
        #endif
        free(args->mgr);
        args->mgr = NULL;
        return;
    }
    
    #ifdef DEBUG
        printf("✅ Server started on http://0.0.0.0:8000\n");
        printf("   WebSocket endpoint: ws://localhost:8000\n");
    #endif
}

// Clean up Mongoose resources
void cleanup_websocket(both *args)
{
    if (args->mgr)
    {
        mg_mgr_free(args->mgr);
        free(args->mgr);
        args->mgr = NULL;
    }
}