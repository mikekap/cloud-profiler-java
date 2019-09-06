/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CLOUD_PROFILER_AGENT_MEMORY_INFO_H_
#define CLOUD_PROFILER_AGENT_MEMORY_INFO_H_

#include "globals.h"
#include <atomic>
#include <map>
#include <mutex>
#include <vector>

namespace cloud {
namespace profiler {

enum IntervalType {
  COMPILED_CODE,
  NATIVE,
};

struct MemoryInterval {
  MemoryInterval() : start(0), length(0), interval_type(COMPILED_CODE), method_id(0), name("") {}
  uintptr_t start;
  uint64_t length;

  IntervalType interval_type;
  jmethodID method_id;
  const char *name;

  bool Contains(uintptr_t other) const {
    return other >= start && other - start < length;
  }

  bool operator<(const MemoryInterval& other) {
    return start < other.start;
  }
};

class MemoryInfo {
 public:
  MemoryInfo() {};

  void AddExecutableMemoryRange(uintptr_t start, uint64_t length, jmethodID method_id) {
    std::lock_guard<std::mutex> l(lock_);
    MemoryInterval interval;
    interval.start = start;
    interval.length = length;
    interval.interval_type = COMPILED_CODE;
    interval.method_id = method_id;
    memory_ranges_.emplace_back(interval);
  }

  void RemoveExecutableMemoryRange(uintptr_t start, jmethodID method_id) {
    std::lock_guard<std::mutex> l(lock_);
    for (auto it = memory_ranges_.begin(); it != memory_ranges_.end(); it++) {
      if (it->start == start && it->method_id == method_id) {
        memory_ranges_.erase(it);
        break;
      }
    }
  }

  void AddNativeMemoryRange(uintptr_t start, uint64_t length, const char* name) {
    std::lock_guard<std::mutex> l(lock_);
    MemoryInterval interval;
    interval.start = start;
    interval.length = length;
    interval.interval_type = NATIVE;
    interval.name = strdup(name);
    memory_ranges_.emplace_back(interval);
  }

  MemoryInterval GetMemoryInterval(uintptr_t point) {
    std::lock_guard<std::mutex> l(lock_);
    for (const auto &interval : memory_ranges_) {
      if (interval.Contains(point)) {
        return interval;
      }
    }
    return MemoryInterval();
  }

  uint64_t Count() {
    std::lock_guard<std::mutex> l(lock_);
    return memory_ranges_.size();
  }

 private:
   std::vector<MemoryInterval> memory_ranges_;
   std::mutex lock_;

   DISALLOW_COPY_AND_ASSIGN(MemoryInfo);
};

}
}

#endif
