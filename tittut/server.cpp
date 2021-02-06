#include "argparser.hpp"
#include "video-server.hpp"

int main(int argc, const char *argv[]) {
    ArgParser parser("Tittut server");
    parser.description("Streaming application using Video4Linux and SDL2.");
    parser.parse(argc, argv);

    VideoServer server(4097);
    server.run();
}
