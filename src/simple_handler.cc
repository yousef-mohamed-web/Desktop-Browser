// Copyright (c) 2026 Desktop Browser project.
#include "simple_handler.h"
#include <windows.h>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/wrapper/cef_helpers.h"

namespace {
constexpr UINT kShellChangedMessage = WM_APP + 41;
constexpr int kNavigationBarHeight = 72;
const char* MemoryPressureName(MemoryPressureLevel level) {
  switch (level) {
    case MemoryPressureLevel::kModerate: return "moderate";
    case MemoryPressureLevel::kHigh: return "high";
    case MemoryPressureLevel::kCritical: return "critical";
    default: return "normal";
  }
}
std::ofstream OpenMetricsFile() {
  const char* path = std::getenv("DESKTOP_BROWSER_METRICS_FILE");
  if (!path || path[0] == '\0') return std::ofstream();
  return std::ofstream(path, std::ios::out | std::ios::app);
}
}  // namespace

CefRefPtr<CefLifeSpanHandler> SimpleHandler::GetLifeSpanHandler() { return this; }
CefRefPtr<CefLoadHandler> SimpleHandler::GetLoadHandler() { return this; }
CefRefPtr<CefDisplayHandler> SimpleHandler::GetDisplayHandler() { return this; }

void SimpleHandler::SetMainWindow(HWND window) {
  CEF_REQUIRE_UI_THREAD();
  main_window_ = window;
}
void SimpleHandler::NotifyShellChanged() {
  if (main_window_ && IsWindow(main_window_))
    PostMessageW(main_window_, kShellChangedMessage, 0, 0);
}
bool SimpleHandler::CreateBrowserTab(const std::string& url) {
  CEF_REQUIRE_UI_THREAD();
  if (!main_window_ || !IsWindow(main_window_)) return false;
  RECT rc = {};
  if (!GetClientRect(main_window_, &rc)) return false;

  // Win32 RECT coordinates use LONG while CefRect and std::max use int.
  // Normalize dimensions before constructing the CEF child-window bounds.
  const int client_width = static_cast<int>(rc.right - rc.left);
  const int client_height = static_cast<int>(rc.bottom - rc.top);
  const int browser_width = (std::max)(1, client_width);
  const int browser_height =
      (std::max)(1, client_height - kNavigationBarHeight);

  CefWindowInfo wi;
  wi.SetAsChild(main_window_,
                CefRect(0, kNavigationBarHeight, browser_width,
                        browser_height));
  wi.runtime_style = CEF_RUNTIME_STYLE_ALLOY;
  CefBrowserSettings settings;
  return CefBrowserHost::CreateBrowser(wi, this, url, settings, nullptr, nullptr);
}
void SimpleHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  if (!browser) return;
  tab_manager_.Add(browser);
  tab_titles_[browser->GetIdentifier()] = L"New Tab";
  NotifyShellChanged();
  RECT rc = {};
  if (main_window_ && GetClientRect(main_window_, &rc))
    LayoutBrowsers(0, kNavigationBarHeight, rc.right - rc.left,
                   rc.bottom - rc.top - kNavigationBarHeight);
}
void SimpleHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  if (browser) {
    const int id = browser->GetIdentifier();
    load_start_by_browser_.erase(id);
    tab_titles_.erase(id);
    tab_manager_.Remove(id);
  }
  NotifyShellChanged();
  if (tab_manager_.IsEmpty()) CefQuitMessageLoop();
}
void SimpleHandler::OnLoadStart(CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame, TransitionType transition_type) {
  CEF_REQUIRE_UI_THREAD();
  if (browser && frame && frame->IsMain())
    load_start_by_browser_[browser->GetIdentifier()] =
        std::chrono::steady_clock::now();
}
void SimpleHandler::OnLoadEnd(CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame, int http_status_code) {
  CEF_REQUIRE_UI_THREAD();
  if (!browser || !frame || !frame->IsMain()) return;
  const int id = browser->GetIdentifier();
  const auto started = load_start_by_browser_.find(id);
  if (started == load_start_by_browser_.end()) return;
  const auto elapsed = std::chrono::steady_clock::now() - started->second;
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
  load_start_by_browser_.erase(started);
  auto metrics = OpenMetricsFile();
  if (metrics.is_open())
    metrics << "page_load_ms=" << ms << " http_status=" << http_status_code
            << " browser_id=" << id << " url=" << frame->GetURL() << std::endl;
  NotifyShellChanged();
}
void SimpleHandler::OnAddressChange(CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame, const CefString& url) {
  CEF_REQUIRE_UI_THREAD();
  if (browser && frame && frame->IsMain()) NotifyShellChanged();
}
void SimpleHandler::OnTitleChange(CefRefPtr<CefBrowser> browser,
    const CefString& title) {
  CEF_REQUIRE_UI_THREAD();
  if (!browser) return;
  tab_titles_[browser->GetIdentifier()] = title.ToWString();
  NotifyShellChanged();
}
bool SimpleHandler::SetActiveBrowser(int browser_id) {
  CEF_REQUIRE_UI_THREAD();
  if (!tab_manager_.SetActive(browser_id)) return false;
  RECT rc = {};
  if (main_window_ && GetClientRect(main_window_, &rc))
    LayoutBrowsers(0, kNavigationBarHeight, rc.right - rc.left,
                   rc.bottom - rc.top - kNavigationBarHeight);
  NotifyShellChanged();
  return true;
}
void SimpleHandler::LayoutBrowsers(int x, int y, int width, int height) {
  CEF_REQUIRE_UI_THREAD();
  const auto active = GetBrowser();
  const int active_id = active ? active->GetIdentifier() : -1;
  for (const auto& browser : tab_manager_.GetBrowsers()) {
    if (!browser) continue;
    HWND child = browser->GetHost()->GetWindowHandle();
    if (!child) continue;
    SetWindowPos(child, nullptr, x, y, (std::max)(1, width),
                 (std::max)(1, height), SWP_NOZORDER | SWP_NOACTIVATE);
    const bool hidden = browser->GetIdentifier() != active_id;
    ShowWindow(child, hidden ? SW_HIDE : SW_SHOW);
    browser->GetHost()->WasHidden(hidden);
  }
}
void SimpleHandler::NavigateActive(const std::string& url) {
  CEF_REQUIRE_UI_THREAD();
  auto b = GetBrowser();
  if (b && b->GetMainFrame()) b->GetMainFrame()->LoadURL(url);
}
void SimpleHandler::GoBack() {
  CEF_REQUIRE_UI_THREAD();
  auto b = GetBrowser();
  if (b && b->CanGoBack()) b->GoBack();
}
void SimpleHandler::GoForward() {
  CEF_REQUIRE_UI_THREAD();
  auto b = GetBrowser();
  if (b && b->CanGoForward()) b->GoForward();
}
void SimpleHandler::Reload() {
  CEF_REQUIRE_UI_THREAD();
  auto b = GetBrowser();
  if (b) b->Reload();
}
void SimpleHandler::StopLoad() {
  CEF_REQUIRE_UI_THREAD();
  auto b = GetBrowser();
  if (b) b->StopLoad();
}
void SimpleHandler::CloseActiveTab() {
  CEF_REQUIRE_UI_THREAD();
  auto b = GetBrowser();
  if (!b) return;
  if (GetTabCount() <= 1) {
    NavigateActive("data:text/html,<html><body style='font-family:Segoe UI;padding:40px'>"
                   "<h2>Desktop Browser</h2><form action='https://www.google.com/search'>"
                   "<input name='q' autofocus placeholder='Search Google or enter address'>"
                   "<button>Search</button></form></body></html>");
    return;
  }
  b->GetHost()->CloseBrowser(false);
}
void SimpleHandler::ActivateNextTab(bool reverse) {
  CEF_REQUIRE_UI_THREAD();
  const auto ids = GetTabIds();
  if (ids.size() < 2) return;
  const auto active = GetBrowser();
  const int current = active ? active->GetIdentifier() : ids.front();
  const auto it = std::find(ids.begin(), ids.end(), current);
  const size_t i = it == ids.end() ? 0 : static_cast<size_t>(it - ids.begin());
  const size_t n = reverse ? (i + ids.size() - 1) % ids.size()
                           : (i + 1) % ids.size();
  SetActiveBrowser(ids[n]);
}
CefRefPtr<CefBrowser> SimpleHandler::GetBrowser() const {
  CEF_REQUIRE_UI_THREAD();
  return tab_manager_.GetActiveBrowser();
}
CefRefPtr<CefBrowser> SimpleHandler::GetBrowser(int id) const {
  CEF_REQUIRE_UI_THREAD();
  return tab_manager_.GetBrowser(id);
}
bool SimpleHandler::TryCloseAllBrowsers() {
  CEF_REQUIRE_UI_THREAD();
  bool can_close = true;
  for (const auto& b : tab_manager_.GetBrowsers())
    if (b && !b->GetHost()->TryCloseBrowser()) can_close = false;
  return can_close;
}
size_t SimpleHandler::GetTabCount() const {
  CEF_REQUIRE_UI_THREAD();
  return tab_manager_.GetCount();
}
std::vector<int> SimpleHandler::GetTabIds() const {
  CEF_REQUIRE_UI_THREAD();
  std::vector<int> ids;
  for (const auto& b : tab_manager_.GetBrowsers())
    if (b) ids.push_back(b->GetIdentifier());
  return ids;
}
std::wstring SimpleHandler::GetTabTitle(int id) const {
  CEF_REQUIRE_UI_THREAD();
  const auto it = tab_titles_.find(id);
  return it == tab_titles_.end() ? L"New Tab" : it->second;
}
void SimpleHandler::RecordResourceSample() {
  CEF_REQUIRE_UI_THREAD();
  ResourceSnapshot s;
  if (!resource_monitor_.Sample(&s)) return;
  const auto pressure = adaptive_memory_manager_.Evaluate(s);
  auto metrics = OpenMetricsFile();
  if (metrics.is_open())
    metrics << "resource_sample working_set_mb=" << s.process_working_set_mb
            << " private_bytes_mb=" << s.process_private_bytes_mb
            << " available_physical_mb=" << s.available_physical_mb
            << " system_memory_load_percent=" << s.system_memory_load_percent
            << " process_cpu_percent=" << s.process_cpu_percent
            << " memory_pressure=" << MemoryPressureName(pressure)
            << " open_browser_instances=" << tab_manager_.GetCount() << std::endl;
}
