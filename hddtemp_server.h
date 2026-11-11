#pragma once

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include <arpa/inet.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

struct Drive {
  std::string dir;
  int temp = -1;
  std::string model;
};

struct Field {
  std::string key;
  int tk;
};
