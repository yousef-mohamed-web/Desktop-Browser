// Copyright (c) 2026 Desktop Browser project.
#ifndef DESKTOP_BROWSER_RESOURCE_MONITOR_H_
#define DESKTOP_BROWSER_RESOURCE_MONITOR_H_

#include <windows.h>

struct ResourceSnapshot {
  double process_working_set_mb = 0.0;
  double process_private_bytes_mb = 0.0;
  double available_physical_mb = 0.0;
  DWORD system_memory_load_percent = 0;
  double process_cpu_percent = 0.0;
};

// Lightweight, read-only sampler for the browser host process and system
// memory. It never changes process priority, terminates processes, or
// suspends browser work.
class ResourceMonitor {
 public:
  bool Sample(ResourceSnapshot* snapshot);

 private:
  FILETIME previous_kernel_time_ = {};
  FILETIME previous_user_time_ = {};
  ULONGLONG previous_wall_time_ = 0;
  bool has_previous_cpu_sample_ = false;
};

#endif  // DESKTOP_BROWSER_RESOURCE_MONITOR_H_
