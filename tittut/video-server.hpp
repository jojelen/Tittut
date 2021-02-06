#pragma once

#include "tcp-interface.hpp"
#include "v4l-stream.hpp"

#include <iostream>
#include <string.h>

class VideoServer : TcpInterface {
    int localSocket_ = -1;
    int port_ = -1;
    int width_ = 0;
    int height_ = 0;

    void streamConfigHandler(int sck, uint64_t size) override {
        Package pkg = {.type = PKG_TYPE::STREAM_CONFIG, .data = {}};
        readPackageData(sck, pkg, size);
        width_ = static_cast<int>(getNumFromVec(0, pkg.data));
        height_ = static_cast<int>(getNumFromVec(1, pkg.data));

        std::cout << "Recieved stream configuration:\n";
        std::cout << "Got width = " << width_ << std::endl;
        std::cout << "Got height = " << height_ << std::endl;
    }

    void frameHandler(int sck, uint64_t size) override {
        Package pkg = {.type = PKG_TYPE::FRAME, .data = {}};
        readPackageData(sck, pkg, size);
        std::cerr << "WARNING: Server recieved a frame. Throwing it away.\n";
    }

    void connectionWork(int socket) {
        try {
            sendMsg(socket, "Connection established");
            sendMsg(socket, "Please send stream configuration");

            // Handle recieved packages until we get a STREAM_CONFIG.
            while (true) {
                auto type = handlePackage(socket);
                if (type.value() == PKG_TYPE::STREAM_CONFIG)
                    break;
            }
            V4LStream v4l(width_, height_, V4L2_PIX_FMT_YUYV);

            sendMsg(socket, "Server configured the video stream successfully");

            while (true) {
                v4l.update();
                sendBuffer(socket, v4l.getBuffer(), v4l.getBufferSize());
                handlePackage(socket, MSG_DONTWAIT);
            }
        } catch (std::exception const &e) {
            std::cerr << "Closing connection: " << e.what() << std::endl;
        }

        close(socket);
    }

  public:
    VideoServer(int port) : port_(port) {
        localSocket_ = createListenSocket(port_);
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
