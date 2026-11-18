// Copyright (c) 2026 Desktop Browser project.
#include "resource_monitor.h"

#include <psapi.h>

#pragma comment(lib, "Psapi.lib")

namespace {
ULONGLONG FileTimeToUInt64(const FILETIME& value) {
  ULARGE_INTEGER converted = {};
  converted.LowPart = value.dwLowDateTime;
  converted.HighPart = value.dwHighDateTime;
  return converted.QuadPart;
}
}  // namespace

bool ResourceMonitor::Sample(ResourceSnapshot* snapshot) {
  if (!snapshot) return false;

  MEMORYSTATUSEX memory = {};
  memory.dwLength = sizeof(memory);
  if (!GlobalMemoryStatusEx(&memory)) return false;

  PROCESS_MEMORY_COUNTERS_EX counters = {};
  counters.cb = sizeof(counters);
  if (!GetProcessMemoryInfo(
          GetCurrentProcess(),
          reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
          sizeof(counters))) {
    return false;
  }

  FILETIME creation_time = {};
  FILETIME exit_time = {};
  FILETIME kernel_time = {};
  FILETIME user_time = {};
  if (!GetProcessTimes(GetCurrentProcess(), &creation_time, &exit_time,
                       &kernel_time, &user_time)) {
    return false;
  }

  snapshot->process_working_set_mb =
      static_cast<double>(counters.WorkingSetSize) / (1024.0 * 1024.0);
  snapshot->process_private_bytes_mb =
      static_cast<double>(counters.PrivateUsage) / (1024.0 * 1024.0);
  snapshot->available_physical_mb =
      static_cast<double>(memory.ullAvailPhys) / (1024.0 * 1024.0);
  snapshot->system_memory_load_percent = memory.dwMemoryLoad;
  snapshot->process_cpu_percent = 0.0;

  const ULONGLONG now = GetTickCount64();
  if (has_previous_cpu_sample_) {
    const ULONGLONG elapsed_ms = now - previous_wall_time_;
    const ULONGLONG cpu_delta =
        (FileTimeToUInt64(kernel_time) - FileTimeToUInt64(previous_kernel_time_)) +
        (FileTimeToUInt64(user_time) - FileTimeToUInt64(previous_user_time_));
    SYSTEM_INFO system_info = {};
    GetSystemInfo(&system_info);
    const DWORD processors =
        system_info.dwNumberOfProcessors > 0
            ? system_info.dwNumberOfProcessors
            : 1;
    if (elapsed_ms > 0) {
      snapshot->process_cpu_percent =
          (static_cast<double>(cpu_delta) / 10000.0 /
           static_cast<double>(elapsed_ms) /
           static_cast<double>(processors)) * 100.0;
    }
  }

  previous_kernel_time_ = kernel_time;
  previous_user_time_ = user_time;
  previous_wall_time_ = now;
  has_previous_cpu_sample_ = true;
  return true;
}
