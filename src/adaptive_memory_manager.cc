// Copyright (c) 2026 Desktop Browser project.
#include "adaptive_memory_manager.h"

MemoryPressureLevel AdaptiveMemoryManager::Evaluate(
    const ResourceSnapshot& snapshot) const {
  if (snapshot.system_memory_load_percent >= 92 ||
      snapshot.available_physical_mb < 128.0) {
    return MemoryPressureLevel::kCritical;
  }
  if (snapshot.system_memory_load_percent >= 85 ||
      snapshot.available_physical_mb < 256.0) {
    return MemoryPressureLevel::kHigh;
  }
  if (snapshot.system_memory_load_percent >= 75 ||
      snapshot.available_physical_mb < 512.0) {
    return MemoryPressureLevel::kModerate;
  }
  return MemoryPressureLevel::kNormal;
}
