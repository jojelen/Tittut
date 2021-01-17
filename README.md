# Build

## Docker

Build docker image with
```
docker build -t tittut .
```
Run with
```
docker run --volume <TITTUT_DIR>:/tittut --device=/dev/video0 -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix --privileged -it tittut
```

You might have to run `xhost +x` before running docker container.
