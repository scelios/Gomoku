#include "../include/gomoku.h"
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <cjson/cJSON.h>


void flashbang(screen *windows)
{
    if (!windows) return;

    // Flash the screen white briefly
    for (int y = 0; y < windows->height; y++)
    {
        for (int x = 0; x < windows->width; x++)
        {
            mlx_put_pixel(windows->img, x, y, 0xFFFFFFFF); // White pixel
        }
    }
    mlx_image_to_window(windows->mlx, windows->img, 0, 0);
    windows->changed = true;
}

void openBrainRot()
{
    const char *url[] = {"https://www.youtube.com/shorts/6e0ctwJ_yrI",
                         "https://www.youtube.com/shorts/iqnLbqsS8-w",
                         "https://www.youtube.com/shorts/rKWMzbS2zU8",
                         "https://www.youtube.com/shorts/twyGVqu6kmI",
                         "https://www.youtube.com/shorts/twyGVqu6kmI",
    };
    int choice = rand() % 5;
    char command[256];
    int returnValue;
    
    // Open browser to a funny video and sleep for the duration of the video
#ifdef _WIN32
        snprintf(command, sizeof(command), "start %s", url[choice]);
        returnValue = system(command);
#elif __APPLE__
        snprintf(command, sizeof(command), "open %s", url[choice]);
        returnValue = system(command);
#else // Linux and others
        snprintf(command, sizeof(command), "xdg-open %s", url[choice]);
        returnValue = system(command);
#endif
    (void)returnValue; // Suppress unused variable warning
}


// Callback to handle API response
size_t write_callback(void *contents, size_t size, size_t nmemb, char *userp)
{
    strncat(userp, (char *)contents, size * nmemb);
    return size * nmemb;
}

// Function to read API key from .env file
char *get_env_value(const char *key)
{
    FILE *fp = fopen(".env", "r");
    if (!fp)
        return NULL;
    
    char line[256];
    static char value[256];
    size_t key_len = strlen(key);
    
    while (fgets(line, sizeof(line), fp))
    {
        if (strncmp(line, key, key_len) == 0 && line[key_len] == '=')
        {
            strcpy(value, line + key_len + 1);
            // Remove trailing newline
            value[strcspn(value, "\n")] = 0;
            fclose(fp);
            return value;
        }
    }
    
    fclose(fp);
    return NULL;
}

char* getBoardString(int *board)
{
    static char boardStr[BOARD_SIZE * BOARD_SIZE + 1 + 18]; // +1 for null terminator, +18 for \n
    for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; i++)
    {
        if (board[i] == P1)
            boardStr[i] = 'X';
        else if (board[i] == P2)
            boardStr[i] = 'O';
        else
            boardStr[i] = '.';
        if ((i + 1) % BOARD_SIZE == 0 && i != BOARD_SIZE * BOARD_SIZE - 1)
        {
            boardStr[i + 1] = '\n';
            i++;
        }
    }
    boardStr[BOARD_SIZE * BOARD_SIZE + 18] = '\0';
    return boardStr;
}

void llmRoast(int *board)
{
    CURL *curl;
    CURLcode res;
    char *api_key = get_env_value("GEMINI_API_KEY");
    
    if (!api_key)
    {
        fprintf(stderr, "Error: GEMINI_API_KEY not found in .env file\n");
        return;
    }
    
    char url[512];
    char response[8192] = {0};
    char json_payload[1024];
    char *boardString = getBoardString(board);
    // printf("Board State for LLM:\n%s\n", boardString); // Debug print

    snprintf(json_payload, sizeof(json_payload),
             "{\"contents\": [{\"parts\": [{\"text\": \"Give a 2-line roast for a Gomoku player. Here's the board (you are X, the player is O):\n%s. Be funny and brief and say something about what the board looks like. Don't say anything else\"}]}]}",
             boardString);

    snprintf(url, sizeof(url),
             "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash-lite:generateContent?key=%s",
             api_key);

    curl = curl_easy_init();
    if (curl)
    {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)response);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        else
        {
            // Parse JSON to extract the roast text
            cJSON *json = cJSON_Parse(response);
            if (json)
            {
                cJSON *candidates = cJSON_GetObjectItem(json, "candidates");
                if (candidates && candidates->child)
                {
                    cJSON *content = cJSON_GetObjectItem(candidates->child, "content");
                    cJSON *parts = cJSON_GetObjectItem(content, "parts");
                    if (parts && parts->child)
                    {
                        cJSON *text = cJSON_GetObjectItem(parts->child, "text");
                        if (text && text->valuestring)
                            printf("🤖 AI Roast:\n%s\n", text->valuestring);
                    }
                }
                cJSON_Delete(json);
            }
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}