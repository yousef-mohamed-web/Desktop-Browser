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
std::wstring SessionFilePath() {
  wchar_t local_app_data[MAX_PATH] = {};
  const DWORD length = GetEnvironmentVariableW(
      L"LOCALAPPDATA", local_app_data, MAX_PATH);
  if (length == 0 || length >= MAX_PATH) return {};
  std::wstring directory(local_app_data);
  directory += L"\\DesktopBrowser";
  if (!CreateDirectoryW(directory.c_str(), nullptr) &&
      GetLastError() != ERROR_ALREADY_EXISTS) return {};
  return directory + L"\\session.txt";
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
  if (session_restore_pending_ > 0) {
    --session_restore_pending_;
    if (session_restore_pending_ == 0) SaveSession();
  } else {
    SaveSession();
  }
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
  SaveSession();
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
  if (browser && frame && frame->IsMain()) {
    if (session_restore_pending_ == 0) SaveSession();
    NotifyShellChanged();
  }
}
void SimpleHandler::OnTitleChange(CefRefPtr<CefBrowser> browser,
    const CefString& title) {
  CEF_REQUIRE_UI_THREAD();
  if (!browser) return;
  tab_titles_[browser->GetIdentifier()] = title.ToWString();
  NotifyShellChanged();
}
bool SimpleHandler::MoveTab(int from_index, int to_index) {
  CEF_REQUIRE_UI_THREAD();
  if (!tab_manager_.Move(from_index, to_index)) return false;
  SaveSession();
  NotifyShellChanged();
  return true;
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
  ids = tab_manager_.GetOrderedIds();
  return ids;
}
std::wstring SimpleHandler::GetTabTitle(int id) const {
  CEF_REQUIRE_UI_THREAD();
  const auto it = tab_titles_.find(id);
  return it == tab_titles_.end() ? L"New Tab" : it->second;
}
std::vector<std::string> SimpleHandler::LoadSavedSession() {
  std::vector<std::string> urls;
  const std::wstring path = SessionFilePath();
  if (path.empty()) return urls;
  HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
      nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) return urls;
  LARGE_INTEGER size = {};
  if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0 ||
      size.QuadPart > 1024 * 1024) {
    CloseHandle(file);
    return urls;
  }
  std::string data(static_cast<size_t>(size.QuadPart), '\0');
  DWORD bytes_read = 0;
  const BOOL ok = ReadFile(file, data.data(),
      static_cast<DWORD>(data.size()), &bytes_read, nullptr);
  CloseHandle(file);
  if (!ok) return urls;
  data.resize(bytes_read);
  size_t start = 0;
  while (start < data.size()) {
    const size_t end = data.find('\n', start);
    std::string url = data.substr(start,
        end == std::string::npos ? std::string::npos : end - start);
    if (!url.empty() && url.back() == '\r') url.pop_back();
    if (!url.empty() && url.rfind("about:blank", 0) != 0)
      urls.push_back(url);
    if (end == std::string::npos) break;
    start = end + 1;
  }
  return urls;
}

void SimpleHandler::SetSessionRestoreCount(size_t count) {
  CEF_REQUIRE_UI_THREAD();
  session_restore_pending_ = count;
}

void SimpleHandler::SaveSession() const {
  CEF_REQUIRE_UI_THREAD();
  if (session_restore_pending_ > 0) return;
  const std::wstring path = SessionFilePath();
  if (path.empty()) return;
  std::string data;
  for (int id : tab_manager_.GetOrderedIds()) {
    const auto browser = tab_manager_.GetBrowser(id);
    if (!browser || !browser->GetMainFrame()) continue;
    const std::string url = browser->GetMainFrame()->GetURL().ToString();
    if (url.empty() || url == "about:blank") continue;
    if (url.find_first_of("\r\n") != std::string::npos) continue;
    data += url;
    data.push_back('\n');
  }
  HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr,
      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) return;
  DWORD written = 0;
  if (!data.empty())
    WriteFile(file, data.data(), static_cast<DWORD>(data.size()),
              &written, nullptr);
  FlushFileBuffers(file);
  CloseHandle(file);
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
