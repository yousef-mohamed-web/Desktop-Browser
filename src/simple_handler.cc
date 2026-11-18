// Copyright (c) 2026 Desktop Browser project.
#include "simple_handler.h"

#include <cstdlib>
#include <fstream>
#include <utility>

#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/wrapper/cef_helpers.h"

namespace {
const char* MemoryPressureName(MemoryPressureLevel level) {
  switch (level) {
    case MemoryPressureLevel::kModerate: return "moderate";
    case MemoryPressureLevel::kHigh: return "high";
    case MemoryPressureLevel::kCritical: return "critical";
    case MemoryPressureLevel::kNormal:
    default: return "normal";
  }
}

std::ofstream OpenMetricsFile() {
  const char* path = std::getenv("DESKTOP_BROWSER_METRICS_FILE");
  if (!path || path[0] == '\0') return std::ofstream();
  return std::ofstream(path, std::ios::out | std::ios::app);
}
}  // namespace

CefRefPtr<CefLifeSpanHandler> SimpleHandler::GetLifeSpanHandler() {
  return this;
}

CefRefPtr<CefLoadHandler> SimpleHandler::GetLoadHandler() {
  return this;
}

void SimpleHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  tab_manager_.Add(browser);
}

void SimpleHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  if (browser) {
    load_start_by_browser_.erase(browser->GetIdentifier());
    tab_manager_.Remove(browser->GetIdentifier());
  }
  if (tab_manager_.IsEmpty()) CefQuitMessageLoop();
}

void SimpleHandler::OnLoadStart(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                TransitionType transition_type) {
  CEF_REQUIRE_UI_THREAD();
  if (browser && frame && frame->IsMain()) {
    load_start_by_browser_[browser->GetIdentifier()] =
        std::chrono::steady_clock::now();
  }
}

void SimpleHandler::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefFrame> frame,
                              int http_status_code) {
  CEF_REQUIRE_UI_THREAD();
  if (!browser || !frame || !frame->IsMain()) return;

  const int browser_id = browser->GetIdentifier();
  const auto started = load_start_by_browser_.find(browser_id);
  if (started == load_start_by_browser_.end()) return;

  const auto elapsed = std::chrono::steady_clock::now() - started->second;
  const auto elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
  load_start_by_browser_.erase(started);

  auto metrics = OpenMetricsFile();
  if (metrics.is_open()) {
    metrics << "page_load_ms=" << elapsed_ms
            << " http_status=" << http_status_code
            << " browser_id=" << browser_id
            << " url=" << frame->GetURL() << std::endl;
  }
}

CefRefPtr<CefBrowser> SimpleHandler::GetBrowser() const {
  CEF_REQUIRE_UI_THREAD();
  return tab_manager_.GetActiveBrowser();
}

CefRefPtr<CefBrowser> SimpleHandler::GetBrowser(int browser_id) const {
  CEF_REQUIRE_UI_THREAD();
  return tab_manager_.GetBrowser(browser_id);
}

bool SimpleHandler::SetActiveBrowser(int browser_id) {
  CEF_REQUIRE_UI_THREAD();
  return tab_manager_.SetActive(browser_id);
}

bool SimpleHandler::TryCloseAllBrowsers() {
  CEF_REQUIRE_UI_THREAD();
  bool all_can_close = true;
  const auto browsers = tab_manager_.GetBrowsers();
  for (const auto& browser : browsers) {
    if (browser && !browser->GetHost()->TryCloseBrowser()) {
      all_can_close = false;
    }
  }
  return all_can_close;
}

size_t SimpleHandler::GetTabCount() const {
  CEF_REQUIRE_UI_THREAD();
  return tab_manager_.GetCount();
}

void SimpleHandler::RecordResourceSample() {
  CEF_REQUIRE_UI_THREAD();
  ResourceSnapshot snapshot;
  if (!resource_monitor_.Sample(&snapshot)) return;

  const MemoryPressureLevel pressure =
      adaptive_memory_manager_.Evaluate(snapshot);
  auto metrics = OpenMetricsFile();
  if (metrics.is_open()) {
    metrics << "resource_sample"
            << " working_set_mb=" << snapshot.process_working_set_mb
            << " private_bytes_mb=" << snapshot.process_private_bytes_mb
            << " available_physical_mb=" << snapshot.available_physical_mb
            << " system_memory_load_percent="
            << snapshot.system_memory_load_percent
            << " process_cpu_percent=" << snapshot.process_cpu_percent
            << " memory_pressure=" << MemoryPressureName(pressure)
            << " open_browser_instances=" << tab_manager_.GetCount()
            << std::endl;
  }
}
