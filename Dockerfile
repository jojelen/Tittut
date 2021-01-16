FROM ubuntu:18.04
MAINTAINER Joel joel_ored@hotmail.com

RUN apt-get -y update && \
    apt-get -y upgrade && \
    apt-get install -y \
    build-essential \
    pkg-config \
    g++ \
    meson \
    libsdl2-dev \
    libsdl2-image-dev
