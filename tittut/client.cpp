// Simple webcam application that uses Video4Linux for retrieving video stream
// and SDL2 for viewing it in a window.
#include "sdl.hpp"
#include "v4l.hpp"

#include <chrono>
#include <iostream>
#include <string.h>
#include <thread>

using namespace std;


int main() {
  V4L &v4l = V4L::instance();

  int width = 320;
  int height = 180;
  v4l.setFormat(width, height, V4L2_PIX_FMT_YUYV);

  void *buffer = v4l.getBuffer();

  SDLWindow win("SDL window", width, height);

  for (int i = 0; i < 200; ++i) {
    v4l.update();
    win.updateTexture(buffer);
    win.render();
  }
}
