// Copyright (c) 2026 Desktop Browser project.
#ifndef DESKTOP_BROWSER_ADAPTIVE_MEMORY_MANAGER_H_
#define DESKTOP_BROWSER_ADAPTIVE_MEMORY_MANAGER_H_

#include "resource_monitor.h"

enum class MemoryPressureLevel {
  kNormal,
  kModerate,
  kHigh,
  kCritical,
};

// Phase-one policy evaluator only. It classifies pressure for future
// graduated responses; it does not suspend, discard, or reload any tab.
class AdaptiveMemoryManager {
 public:
  MemoryPressureLevel Evaluate(const ResourceSnapshot& snapshot) const;
};

#endif  // DESKTOP_BROWSER_ADAPTIVE_MEMORY_MANAGER_H_
