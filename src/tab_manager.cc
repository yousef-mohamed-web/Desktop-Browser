// Copyright (c) 2026 Desktop Browser project.
#include "tab_manager.h"

#include "include/wrapper/cef_helpers.h"

void TabManager::Add(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  if (!browser) return;
  const int id = browser->GetIdentifier();
  browsers_[id] = browser;
  active_browser_id_ = id;
}

void TabManager::Remove(int browser_id) {
  CEF_REQUIRE_UI_THREAD();
  browsers_.erase(browser_id);
  if (active_browser_id_ == browser_id) {
    active_browser_id_ = browsers_.empty() ? -1 : browsers_.begin()->first;
  }
}

bool TabManager::SetActive(int browser_id) {
  CEF_REQUIRE_UI_THREAD();
  if (browsers_.find(browser_id) == browsers_.end()) return false;
  active_browser_id_ = browser_id;
  return true;
}

CefRefPtr<CefBrowser> TabManager::GetActiveBrowser() const {
  CEF_REQUIRE_UI_THREAD();
  const auto active = browsers_.find(active_browser_id_);
  if (active != browsers_.end()) return active->second;
  return browsers_.empty() ? nullptr : browsers_.begin()->second;
}

CefRefPtr<CefBrowser> TabManager::GetBrowser(int browser_id) const {
  CEF_REQUIRE_UI_THREAD();
  const auto found = browsers_.find(browser_id);
  return found == browsers_.end() ? nullptr : found->second;
}

std::vector<CefRefPtr<CefBrowser>> TabManager::GetBrowsers() const {
  CEF_REQUIRE_UI_THREAD();
  std::vector<CefRefPtr<CefBrowser>> result;
  result.reserve(browsers_.size());
  for (const auto& entry : browsers_) result.push_back(entry.second);
  return result;
}

std::size_t TabManager::GetCount() const {
  CEF_REQUIRE_UI_THREAD();
  return browsers_.size();
}

bool TabManager::IsEmpty() const {
  CEF_REQUIRE_UI_THREAD();
  return browsers_.empty();
}
