// Simple webcam application that uses Video4Linux for retrieving video stream
// and SDL2 for viewing it in a window.
#include "argparser.hpp"
#include "sdl.hpp"
#include "tcp.hpp"
#include "v4l.hpp"

#include <iostream>

using namespace std;

int main(int argc, const char *argv[]) {
  try {
    ArgParser parser("Tittut");
    parser.description("Streaming application using Video4Linux and SDL2.");
    parser.addArg("width").optional("-x").defaultValue(320);
    parser.addArg("height").optional("-y").defaultValue(180);
    parser.addArg("debug").optional("-d").defaultValue(false).description(
        "Enable verbose printing.");
    parser.addArg("tcp").optional("-t").defaultValue(false).description(
        "Connect to video stream over tcp.");

    parser.parse(argc, argv);

    if (parser.get<bool>("tcp")) {
      connectToServer();
    } else {
      int width = parser.get<int>("width");
      int height = parser.get<int>("height");

      SDLWindow win("SDL window", width, height);
      win.run();
    }
  } catch (exception &e) {
    cout << "ERROR: " << e.what() << endl;
  }
}
