#pragma once
#include <string>
#include <iostream>
#include <unistd.h>

namespace Protocol {
    bool is_delay(std::string &req) {
      std::string DELAY_ENABLED = "-slow";
      while(! req.empty() && (req.back() == '\n' || req.back() == '\r')) req.pop_back();
      
      if (req.size() - DELAY_ENABLED.size() > 0 && req.compare(req.size() - DELAY_ENABLED.size(), DELAY_ENABLED.size(), DELAY_ENABLED) == 0) {
        req.resize(req.size() - (DELAY_ENABLED.size()));
        return true;
      }

      return false;
    }
}