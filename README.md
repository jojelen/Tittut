# Tittut

Tittut is a video streamer that features remote streaming over TCP/IP. It is
pretty low-level and minimalistic; essentially, the only dependencies are:
- Video4Linux2: for fetching video frames.
- SDL2: for displaying the frames.

## Build and Run

Tittut uses meson to build:
```
meson build
cd build && ninja
```

### Run

Run server with
```
./tittut/server
```
Connect to server with
```
./tittut/client -i <ip> -t
```
Or run without any server, i.e. locally
```
./tittut/client
```

### Docker

Not needed, but if one wants to one can run in docker. Build docker image with
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
