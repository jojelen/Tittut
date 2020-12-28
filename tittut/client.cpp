// Simple webcam application that uses Video4Linux for retrieving video stream
// and SDL2 for viewing it in a window.
#include "sdl.hpp"
#include "v4l.hpp"

#include <iostream>


using namespace std;

int main() {
  int width = 320;
  int height = 180;

  SDLWindow win("SDL window", width, height);
  win.run();
}
