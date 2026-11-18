// Copyright (c) 2026 Desktop Browser project.
#ifndef DESKTOP_BROWSER_SIMPLE_HANDLER_H_
#define DESKTOP_BROWSER_SIMPLE_HANDLER_H_

#include <windows.h>
#include <chrono>
#include <map>
#include <string>
#include <vector>
#include "adaptive_memory_manager.h"
#include "include/cef_client.h"
#include "include/cef_display_handler.h"
#include "include/cef_load_handler.h"
#include "resource_monitor.h"
#include "tab_manager.h"

class SimpleHandler : public CefClient, public CefLifeSpanHandler,
                      public CefLoadHandler, public CefDisplayHandler {
 public:
  SimpleHandler() = default;
  CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override;
  CefRefPtr<CefLoadHandler> GetLoadHandler() override;
  CefRefPtr<CefDisplayHandler> GetDisplayHandler() override;
  void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
  void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;
  void OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                   TransitionType transition_type) override;
  void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                 int http_status_code) override;
  void OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                       const CefString& url) override;
  void OnTitleChange(CefRefPtr<CefBrowser> browser,
                     const CefString& title) override;
  void SetMainWindow(HWND window);
  bool CreateBrowserTab(const std::string& url);
  void NavigateActive(const std::string& url);
  void GoBack();
  void GoForward();
  void Reload();
  void StopLoad();
  void CloseActiveTab();
  void ActivateNextTab(bool reverse);
  void LayoutBrowsers(int x, int y, int width, int height);
  CefRefPtr<CefBrowser> GetBrowser() const;
  CefRefPtr<CefBrowser> GetBrowser(int browser_id) const;
  bool SetActiveBrowser(int browser_id);
  bool MoveTab(int from_index, int to_index);
  bool TryCloseAllBrowsers();
  size_t GetTabCount() const;
  std::vector<int> GetTabIds() const;
  std::wstring GetTabTitle(int browser_id) const;
  void RecordResourceSample();
  static std::vector<std::string> LoadSavedSession();
  void SaveSession() const;
  void SetSessionRestoreCount(size_t count);
 private:
  void NotifyShellChanged();
  HWND main_window_ = nullptr;
  TabManager tab_manager_;
  ResourceMonitor resource_monitor_;
  AdaptiveMemoryManager adaptive_memory_manager_;
  std::map<int, std::chrono::steady_clock::time_point> load_start_by_browser_;
  std::map<int, std::wstring> tab_titles_;
  size_t session_restore_pending_ = 0;
  IMPLEMENT_REFCOUNTING(SimpleHandler);
  DISALLOW_COPY_AND_ASSIGN(SimpleHandler);
};
#endif  // DESKTOP_BROWSER_SIMPLE_HANDLER_H_
