/**
 * OpenCV video streaming over TCP/IP
 * Server: Captures video from a webcam and send it to a client
 * by Isaac Maia
 */
#include "tcp.hpp"
#include "v4l.hpp"

#include <algorithm>

#include <arpa/inet.h>
#include <iostream>
#include <net/if.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

class VideoServer {
    int localSocket_ = -1;
    int port_ = -1;
    struct sockaddr_in localAddress_ = {};

    void connectionWork(int socket) {
        try {
            sendMsg(socket, "Connection established");

            // Get video stream config.
            Package pkg = {};
            getPackage(socket, pkg);
            int width = static_cast<int>(getNumFromVec(0, pkg.data));
            int height = static_cast<int>(getNumFromVec(1, pkg.data));
            std::cout << "Got width = " << width << std::endl;
            std::cout << "Got height = " << height << std::endl;
            V4L v4l(width, height, V4L2_PIX_FMT_YUYV);

            void *buffer = v4l.getBuffer();
            size_t bufferSize = v4l.getBufferSize();

            while (true) {
                sendBuffer(socket, buffer, bufferSize);
                v4l.update();
            }

            std::cout << "Finished sending bytes!\n";
        } catch (std::exception const &e) {
            std::cerr << "ERROR: Failed to process client: " << e.what()
                      << std::endl;
        }

        close(socket);
    }

  public:
    VideoServer(int port) : port_(port) {
        localSocket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (localSocket_ == -1) {
            perror("socket() call failed!!");
        }

        localAddress_.sin_family = AF_INET;
        localAddress_.sin_addr.s_addr = INADDR_ANY;
        localAddress_.sin_port = htons(port_);

        if (bind(localSocket_, (struct sockaddr *)&localAddress_,
                 sizeof(localAddress_)) < 0) {
            perror("Can't bind() socket");
            exit(1);
        }
    }
    ~VideoServer() { close(localSocket_); }

    void run() {
        listen(localSocket_, 3);

        std::cout << "Waiting for connections...\n"
                  << "Server Port:" << port_ << std::endl;

        // accept connection from an incoming client
        int remoteSocket = -1;
        constexpr int addrLen = sizeof(struct sockaddr_in);
        struct sockaddr_in remoteAddr = {};
        while (true) {
            remoteSocket = accept(localSocket_, (struct sockaddr *)&remoteAddr,
                                  (socklen_t *)&addrLen);
            if (remoteSocket < 0) {
                throw std::runtime_error("Could not accept connection");
            }
            std::cout << "Connection accepted" << std::endl; // DEBUG

            connectionWork(remoteSocket);
        }
    }
};

int main() {
    VideoServer server(4097);
    server.run();
}
