FROM debian:bookworm

RUN apt-get update && apt-get install -y build-essential make libx11-dev libglfw3-dev libglfw3 xorg-dev cmake xauth




