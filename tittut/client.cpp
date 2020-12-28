// Simple webcam application that uses Video4Linux for retrieving video stream
// and SDL2 for viewing it in a window.
#include "argparser.hpp"
#include "sdl.hpp"
#include "v4l.hpp"

#include <iostream>


using namespace std;


int main(int argc, const char *argv[]) {
  ArgParser parser("Tittut");
  parser.description("Streaming application using Video4Linux and SDL2.");
  parser.addArg("width").optional("-x").defaultValue(320);
  parser.addArg("height").optional("-y").defaultValue(180);
  parser.addArg("debug").optional("-d").defaultValue(false);

  parser.parse(argc, argv);

  int width = parser.get<int>("width");
  int height = parser.get<int>("height");

  SDLWindow win("SDL window", width, height);
  win.run();
}
