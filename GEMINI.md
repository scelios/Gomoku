# Project Overview

This project, named "Gomoku", is a C language application that utilizes the `MLX42` graphics library. It is designed to run within a Docker container, leveraging `docker-compose` for environment setup and execution. The application itself appears to be a basic graphical program that creates a window, randomly colors pixels, and allows user interaction via arrow keys to move an image and the escape key to close the window.

**Technologies Used:**
*   **C Language:** The primary programming language.
*   **MLX42:** A graphics library used for rendering.
*   **GLFW:** A library used by MLX42 for window and input management.
*   **Docker:** Containerization platform for consistent environment setup.
*   **Docker Compose:** Tool for defining and running multi-container Docker applications.
*   **Makefile:** Used to automate build, run, and cleanup tasks.

# Building and Running

The project uses Docker and Docker Compose to manage its build and runtime environment.

## Prerequisites

*   Docker
*   Docker Compose

## Build and Run Commands

All primary operations are automated via the `Makefile`.

*   **`make all`**: This command will build the Docker image (if not already built or if changes are detected), start the Docker container in detached mode, and then attempt to execute the `gomoku` application inside the container.
    ```bash
    make all
    ```

*   **`make build`**: Builds the Docker image and starts the container in detached mode.
    ```bash
    make build
    ```

*   **`make clean`**: Stops and removes the Docker container.
    ```bash
    make clean
    ```

*   **`make prune`**: Stops and removes the Docker container, then prunes unused Docker system data (images, containers, networks, volumes).
    ```bash
    make prune
    ```

*   **`make fclean`**: Performs a more aggressive cleanup, stopping and removing all Docker containers, images, and volumes. Use with caution.
    ```bash
    make fclean
    ```

*   **`make exec`**: Builds the Docker image (if necessary) and then provides an interactive bash shell inside the running `gomoku` container.
    ```bash
    make exec
    ```

*   **`make re`**: Cleans the project completely (`fclean`) and then rebuilds and runs it (`all`).
    ```bash
    make re
    ```

## Running the Application Manually (after `make build` or `make all` has been run)

1.  **Ensure X Server Access:** Before starting the container, you need to grant access to your host's X server. Open a terminal on your host machine and run:
    ```bash
    xhost +
    ```
    *Note: This command disables access control for your X server, which is not recommended for production environments due to security implications. For development and debugging, it's generally acceptable.*

2.  **Start the Container:**
    ```bash
    docker compose up -d
    ```

3.  **Execute the Gomoku Application:**
    ```bash
    docker exec -it gomoku ./gomoku
    ```

# Development Conventions

*   **Language:** C
*   **Graphics Library:** MLX42 (which uses GLFW)
*   **Build System:** Makefile
*   **Containerization:** Docker
*   **Code Structure:** Source files are located in `file/src`, and headers in `file/include`. The `MLX42` library is included as a submodule or directly in `file/MLX42`.
*   **Error Handling:** Basic error checking for MLX42 initialization and image creation is present in `main.c`.
*   **Input Handling:** Keyboard input (arrow keys for movement, ESC for exit) is handled via MLX42 hooks.
