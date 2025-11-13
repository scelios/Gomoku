Gomoku will be a project about a min max algorithm on a gomoku game.
Please check the roadmap

# create a dedicated Xauthority for docker and merge current cookie
xauth nlist $DISPLAY | sed -e 's/^..../ffff/' | xauth -f /tmp/.docker.xauth nmerge -