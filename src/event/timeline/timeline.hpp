#ifndef TIMELINE_HPP
#define TIMELINE_HPP

#include "event/timestamp/timestamp.hpp"

class timeline {
 public:
  static timestamp get_time();
};

#endif