#pragma once
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>
#include <unistd.h>
#include <csignal>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <algorithm>
#include <iterator>

struct Drive {
  std::string dir;
  int temp = -1;
  std::string model;
};

struct Field {
  std::string key;
  int tk;
};
