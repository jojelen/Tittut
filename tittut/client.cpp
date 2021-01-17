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
        parser.addArg("ip")
            .optional("-i")
            .defaultValue("127.0.0.1")
            .description("Ip address to connect to if tcp option is set.");
        parser.addArg("port").optional("-p").defaultValue(4097).description(
            "Port to use in combination with the ip address.");
        parser.addArg("debug").optional("-d").defaultValue(false).description(
            "Enable verbose printing.");
        parser.addArg("tcp").optional("-t").defaultValue(false).description(
            "Connect to video stream over tcp.");

        parser.parse(argc, argv);

        int width = parser.get<int>("width");
        int height = parser.get<int>("height");

        if (parser.get<bool>("tcp")) {
            std::string ip = parser.get<std::string>("ip");
            int port = parser.get<int>("port");
            connectToServer(ip, port, width, height);
        } else {
            unique_ptr<VideoStream> stream =
                make_unique<V4L>(width, height, V4L2_PIX_FMT_YUYV);
            SDLWindow win("SDL window", width, height, stream);
            win.run();
        }
    } catch (exception &e) {
        cout << "ERROR: " << e.what() << endl;
    }
}
