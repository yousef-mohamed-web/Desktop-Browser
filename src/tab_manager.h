// Copyright (c) 2026 Desktop Browser project.
#ifndef DESKTOP_BROWSER_TAB_MANAGER_H_
#define DESKTOP_BROWSER_TAB_MANAGER_H_

#include <cstddef>
#include <map>
#include <vector>

#include "include/cef_browser.h"

// UI-thread-owned registry for browser instances. This is the lifecycle
// foundation for tabs; it deliberately does not define tab UI or UX.
class TabManager {
 public:
  void Add(CefRefPtr<CefBrowser> browser);
  void Remove(int browser_id);
  bool SetActive(int browser_id);

  CefRefPtr<CefBrowser> GetActiveBrowser() const;
  CefRefPtr<CefBrowser> GetBrowser(int browser_id) const;
  std::vector<CefRefPtr<CefBrowser>> GetBrowsers() const;
  std::size_t GetCount() const;
  bool IsEmpty() const;

 private:
  std::map<int, CefRefPtr<CefBrowser>> browsers_;
  int active_browser_id_ = -1;
};

#endif  // DESKTOP_BROWSER_TAB_MANAGER_H_
