#pragma once

#include "tcp-interface.hpp"
#include "video-stream.hpp"

#include <iostream>
#include <string>

class TcpStream : public VideoStream, public TcpInterface {
    int socket_ = -1;
    Package frame_ = {};

    void setupStream(int width, int height) const {
        std::cout << "Setting up stream\n";
        StreamConfig cfg = {.width = static_cast<uint64_t>(width),
                            .height = static_cast<uint64_t>(height)};

        sendStreamConfig(socket_, cfg);
    }

    void streamConfigHandler(int sck, uint64_t size) override {
        Package pkg = {.type = PKG_TYPE::TEXT, .data = {}};
        readPackageData(sck, pkg, size);
        std::cerr << "WARNING: Throwing away stream config.\n";
    }

    void frameHandler(int sck, uint64_t size) override {
        if (size != frame_.data.size()) {
            std::cerr << "WARNING: Frame changed size\n";
        }

        readPackageData(sck, frame_, size);
    }

  public:
    TcpStream(const std::string &ip, int port, int width, int height) {
        socket_ = connectTo(ip, port);

        setupStream(width, height);

        frame_.type = PKG_TYPE::FRAME;
        frame_.data.resize(width * height * 2); // YUYV format.
    }

    ~TcpStream() {
        sendMsg(socket_, "Client is closing down");
        close(socket_);
    }

    inline void *getBuffer() override {
        return static_cast<void *>(frame_.data.data());
    }

    void update() override {
        // Handle recieved packages until we get a FRAME.
        while (true) {
            auto type = handlePackage(socket_);
            if (type.value() == PKG_TYPE::FRAME)
                break;
        }
    }
};