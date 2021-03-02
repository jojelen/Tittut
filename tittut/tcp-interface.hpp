#pragma once

#include "sdl.hpp"
#include "utils.hpp"
#include "video-stream.hpp"

#include <arpa/inet.h>
#include <assert.h>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <tuple>
#include <unistd.h>
#include <vector>

// TODO: Endieness has to be the same on each machine.
class TcpInterface {
  protected:
    // The package that is sent over tcp is
    // uint64_t data size, i.e. data.size() HEADER
    // uint64_t type                        HEADER
    // uint8_t data                         BODY
    // uint8_t ...                          BODY
    // uint8_t data                         BODY
    static constexpr size_t HEADER_SIZE = 16;

    enum class PKG_TYPE {
        INVALID = -1,
        CLOSED = 0,
        STREAM_CONFIG = 1,
        FRAME = 2,
        TEXT = 3,
        NUM_TYPES = 4
    };

    std::string typeToString(const PKG_TYPE &type) const {
        switch (type) {
        case PKG_TYPE::INVALID:
            return "INVALID";
        case PKG_TYPE::CLOSED:
            return "CLOSED";
        case PKG_TYPE::STREAM_CONFIG:
            return "STREAM_CONFIG";
        case PKG_TYPE::FRAME:
            return "FRAME";
        case PKG_TYPE::TEXT:
            return "TEXT";
        case PKG_TYPE::NUM_TYPES:
            return "NUM_TYPES";
        default:
            throw std::invalid_argument("Got invalid PKG_TYPE");
        }
    }

    struct StreamConfig {
        uint64_t width;
        uint64_t height;
        uint64_t format;
    };

    struct Package {
        PKG_TYPE type;
        std::vector<uint8_t> data;
    };

    void sendPackage(int socket, PKG_TYPE type,
                     const std::vector<uint8_t> &data) const {
        std::vector<uint8_t> header;
        addNumToVec(data.size(), header);
        addNumToVec(static_cast<size_t>(type), header);

        assert(header.size() == HEADER_SIZE);

        int bytes = send(socket, static_cast<const void *>(header.data()),
                         HEADER_SIZE, 0);
        if (bytes < 0) {
            throw std::runtime_error("Could not send header of package");
        }

        bytes = send(socket, static_cast<const void *>(data.data()),
                     data.size(), 0);
        if (bytes < 0) {
            throw std::runtime_error("Could not send data of package");
        }

        LOG(std::string("Sent ") + std::to_string(bytes) +
            " data with data size " + std::to_string(data.size()));
    }

    void sendBuffer(int socket, const void *buffer, size_t bufferSize) const {
        std::vector<uint8_t> header;
        addNumToVec(bufferSize, header);
        addNumToVec(static_cast<size_t>(PKG_TYPE::FRAME), header);

        assert(header.size() == HEADER_SIZE);

        int bytes = send(socket, static_cast<const void *>(header.data()),
                         HEADER_SIZE, MSG_WAITALL | MSG_NOSIGNAL);
        if (bytes < 0) {
            throw std::runtime_error("Could not send header of package");
        }

        bytes = send(socket, buffer, bufferSize, MSG_NOSIGNAL);
        if (bytes < 0) {
            throw std::runtime_error("Could not send data of package");
        }

        LOG(std::string("Sent ") + std::to_string(bytes) +
            " data with data size " + std::to_string(bufferSize));
    }

    void sendMsg(int socket, std::string_view msg) const {
        std::vector<uint8_t> data(msg.length());
        std::copy(msg.begin(), msg.end(), data.begin());

        LOG(std::string("Sending msg \"") + std::string(msg) +
            "\", data.size=" + std::to_string(data.size()));
        sendPackage(socket, PKG_TYPE::TEXT, data);
    }

    void sendStreamConfig(int socket, const StreamConfig &cfg) const {
        std::vector<uint8_t> data;

        addNumToVec(static_cast<size_t>(cfg.width), data);
        addNumToVec(static_cast<size_t>(cfg.height), data);
        addNumToVec(static_cast<size_t>(cfg.format), data);

        sendPackage(socket, PKG_TYPE::STREAM_CONFIG, data);
    }

    // Read out the next package's type and data size. It can return an emtpy
    // optional if flags is set to MSG_DONTWAIT.
    std::optional<std::tuple<PKG_TYPE, uint64_t>>
    readPackageHeader(int socket, int flags = MSG_WAITALL) const {
        std::vector<uint8_t> header(HEADER_SIZE);
        int bytes = recv(socket, static_cast<void *>(header.data()),
                         HEADER_SIZE, flags);
        if (bytes == 0) {
            std::cout << "Connection closed\n";
            return std::tuple{PKG_TYPE::CLOSED, 0};
        } else if (bytes < 0 && errno == EAGAIN && (flags & MSG_DONTWAIT)) {
            return {};
        } else if (bytes < 0) {
            throw std::runtime_error(
                std::string("Failed reading out package header from socket: ") +
                strerror(errno) + " (" + std::to_string(errno) + ")");
        }

        uint64_t dataSize = 0;
        std::memcpy(&dataSize, header.data(), sizeof(uint64_t));
        uint64_t pkgType = 0;
        std::memcpy(&pkgType, header.data() + sizeof(uint64_t),
                    sizeof(uint64_t));

        const static uint64_t MAX_TYPES =
            static_cast<uint64_t>(PKG_TYPE::NUM_TYPES);
        PKG_TYPE type = (pkgType >= MAX_TYPES) ? PKG_TYPE::INVALID
                                               : static_cast<PKG_TYPE>(pkgType);

        LOG(std::string("Recieved ") + typeToString(type) +
            " type message of " + std::to_string(dataSize) + " bytes");

        return std::tuple{type, dataSize};
    }

    void readPackageData(int socket, Package &pkg, uint64_t size) const {
        if (size > pkg.data.size())
            pkg.data.resize(size);

        int bytes = recv(socket, static_cast<void *>(pkg.data.data()), size,
                         MSG_WAITALL);
        if (bytes < 0) {
            throw std::runtime_error(
                std::string(
                    "Failed reading reading out package data from socket: ") +
                strerror(errno));
        }

        LOG(std::string("Read ") + std::to_string(bytes) + " bytes of " +
            typeToString(pkg.type) + " message");
    }

    void printTextPackage(const Package &pkg) const {
        if (pkg.type == PKG_TYPE::TEXT) {
            std::string msg(pkg.data.begin(), pkg.data.end());
            std::cout << "Recieved msg: " << msg << std::endl;
        } else {
            std::cerr << "Package is not of text type (type="
                      << static_cast<int>(pkg.type) << ")\n";
        }
    }

  protected:
    virtual void errorHandler(int sck, uint64_t size) {
        Package pkg = {.type = PKG_TYPE::TEXT, .data = {}};
        readPackageData(sck, pkg, size);
        throw std::runtime_error("Got invalid package type");
    }

    virtual void closedHandler(int sck, uint64_t size) {
        Package pkg = {.type = PKG_TYPE::TEXT, .data = {}};
        readPackageData(sck, pkg, size);
        throw std::runtime_error("Recieved connection closed message");
    }

    virtual void streamConfigHandler(int sck, uint64_t size) = 0;
    virtual void frameHandler(int sck, uint64_t size) = 0;

    virtual void textHandler(int sck, uint64_t size) {
        Package pkg = {.type = PKG_TYPE::TEXT, .data = {}};
        readPackageData(sck, pkg, size);
        printTextPackage(pkg);
    }

  public:
    TcpInterface(){};
    virtual ~TcpInterface(){};

    std::optional<PKG_TYPE> handlePackage(int sck, int flags = MSG_WAITALL) {
        auto package = readPackageHeader(sck, flags);
        if (!package.has_value())
            return {};
        auto [type, dataSize] = package.value();
        switch (type) {
        case PKG_TYPE::INVALID: {
            errorHandler(sck, dataSize);
            return type;
        }
        case PKG_TYPE::CLOSED: {
            closedHandler(sck, dataSize);
            return type;
        }
        case PKG_TYPE::STREAM_CONFIG: {
            streamConfigHandler(sck, dataSize);
            return type;
        }
        case PKG_TYPE::FRAME: {
            frameHandler(sck, dataSize);
            return type;
        }
        case PKG_TYPE::TEXT: {
            textHandler(sck, dataSize);
            return type;
        }
        default: { throw std::runtime_error("ERROR: Unknown type"); }
        }
    }
};
