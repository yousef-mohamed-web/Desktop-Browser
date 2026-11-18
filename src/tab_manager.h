// Copyright (c) 2026 Desktop Browser project.
#ifndef DESKTOP_BROWSER_TAB_MANAGER_H_
#define DESKTOP_BROWSER_TAB_MANAGER_H_

#include <cstddef>
#include <map>
#include <vector>

#include "include/cef_browser.h"

// UI-thread-owned registry. The explicit order vector is the visual tab order;
// browser identifiers remain stable while tabs are dragged.
class TabManager {
 public:
  void Add(CefRefPtr<CefBrowser> browser);
  void Remove(int browser_id);
  bool SetActive(int browser_id);
  bool Move(int from_index, int to_index);

  CefRefPtr<CefBrowser> GetActiveBrowser() const;
  CefRefPtr<CefBrowser> GetBrowser(int browser_id) const;
  std::vector<CefRefPtr<CefBrowser>> GetBrowsers() const;
  std::vector<int> GetOrderedIds() const;
  std::size_t GetCount() const;
  bool IsEmpty() const;

 private:
  std::map<int, CefRefPtr<CefBrowser>> browsers_;
  std::vector<int> order_;
  int active_browser_id_ = -1;
};

#endif  // DESKTOP_BROWSER_TAB_MANAGER_H_
