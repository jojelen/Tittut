#pragma once

#include <iostream>
#include <vector>
#include <string_view>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define HEADER_SIZE 16

// TODO: Endieness has to be the same on each machine.

// The package that is sent over tcp is
// uint64_t data size
// uint64_t flags
// uint8_t data
// uint8_t ...
// uint8_t data
struct Package {
    int64_t flags;
    std::vector<uint8_t> data;
};

bool recPackage(int socket, Package& pkg) {
  std::vector<uint8_t> header(HEADER_SIZE);
  int bytes = recv(socket, static_cast<void*>(header.data()), HEADER_SIZE, MSG_WAITALL);
  if (bytes == 0) {
      std::cout << "Connection closed\n";
      return false;
  }
  else if (bytes < 0) {
      throw std::runtime_error("recv failed");
  }

  uint64_t dataSize = 0;
  std::memcpy(&dataSize, header.data(), sizeof(uint64_t));;

  pkg.data.resize(dataSize);
  bytes = recv(socket, static_cast<void*>(pkg.data.data()), dataSize, MSG_WAITALL);
  if (bytes < 0) {
      throw std::runtime_error("recv failed");
  }

  return true;
}

void sendPackage(int socket, Package &pkg) {
  std::vector<uint8_t> header(HEADER_SIZE);
  uint64_t dataSize = static_cast<uint64_t>(pkg.data.size());

  std::memcpy(header.data(), &dataSize, sizeof(uint64_t));

  int bytes = send(socket, static_cast<const void *>(header.data()), HEADER_SIZE, 0);
  if (bytes < 0) {
    std::cerr << "bytes = " << bytes << std::endl;
  }

  bytes = send(socket, static_cast<const void *>(pkg.data.data()), dataSize, 0);
  if (bytes < 0) {
    std::cerr << "bytes = " << bytes << std::endl;
  }
  std::cout << "Sent data with data size " << dataSize << std::endl;
}

void sendMsg(int socket, std::string_view msg) {
  Package pkg = {};

  //std::string msg("Hello there!");
  pkg.data.resize(msg.length());
  std::copy(msg.begin(), msg.end(), pkg.data.begin());
  sendPackage(socket, pkg);
}

void readPkg(const Package& pkg) {
  std::string msg(pkg.data.begin(), pkg.data.end());
  std::cout << "Recieved msg: " << msg << std::endl;
}

void connectToServer() {
    std::cout << "Connecting to tcp server...\n";

  //--------------------------------------------------------
  // networking stuff: socket , connect
  //--------------------------------------------------------
  int sokt;
  int serverPort;

  std::string serverIP("127.0.0.1");
  serverPort = 4097;

  struct sockaddr_in serverAddr = {};
  socklen_t addrLen = sizeof(struct sockaddr_in);

  if ((sokt = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    std::cerr << "socket() failed" << std::endl;
  }

  serverAddr.sin_family = PF_INET;
  serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());
  serverAddr.sin_port = htons(serverPort);

  if (connect(sokt, (sockaddr *)&serverAddr, addrLen) < 0) {
    std::cerr << "connect() failed!" << std::endl;
  }

  //----------------------------------------------------------
  // OpenCV Code
  //----------------------------------------------------------

  // Mat img = {};
  // img = Mat::zeros(480, 640, CV_8UC1);
  // int imgSize = img.total() * img.elemSize();
  // uchar *iptr = img.data;
  // int bytes = 0;
  // int key = -1;

  //// make img continuos
  // if (!img.isContinuous()) {
  //  img = img.clone();
  //}

  // std::cout << "Image Size:" << imgSize << std::endl;

  // namedWindow("CV Video Client");

  // while (true) {

    // cv::imshow("CV Video Client", img);

    // if (cv::waitKey(10) == 27)
    //  break;
    //}

    // first
  //std::cout << "Printing\n";
  //const char *buffer[100];
  //int bytes = recv(sokt, buffer, 100, MSG_WAITALL);
  //if (bytes < 0) {
  //  std::cerr << "recv failed, received bytes = " << bytes << std::endl;
  //  }

  //std::cout << "Recieved buffer: " << std::string(buffer) << std::endl;
  //
  bool connected = true;
  while (connected) {
    Package pkg = {};
    connected = recPackage(sokt, pkg);
    if (connected)
      readPkg(pkg);
  }

    close(sokt);
}

