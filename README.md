# Tittut

A minimalistic video streamer that is using Video4Linux2 for retrieving frames
and SDL2 to display them. The client program can also connect to a server over
TCP/IP, i.e. remote streaming.

## Docker

Build docker image with
```
docker build -t tittut .
```
Run first
```
    xhost +
```
then
```
docker run --volume <TITTUT_DIR>:/tittut --device=/dev/video0 -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix --privileged -it tittut
```
