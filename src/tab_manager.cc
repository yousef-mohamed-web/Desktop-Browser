// Copyright (c) 2026 Desktop Browser project.
#include "tab_manager.h"

#include <algorithm>

#include "include/wrapper/cef_helpers.h"

void TabManager::Add(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  if (!browser) return;
  const int id = browser->GetIdentifier();
  if (browsers_.find(id) == browsers_.end()) order_.push_back(id);
  browsers_[id] = browser;
  active_browser_id_ = id;
}

void TabManager::Remove(int browser_id) {
  CEF_REQUIRE_UI_THREAD();
  browsers_.erase(browser_id);
  order_.erase(std::remove(order_.begin(), order_.end(), browser_id), order_.end());
  if (active_browser_id_ == browser_id)
    active_browser_id_ = order_.empty() ? -1 : order_.front();
}

bool TabManager::SetActive(int browser_id) {
  CEF_REQUIRE_UI_THREAD();
  if (browsers_.find(browser_id) == browsers_.end()) return false;
  active_browser_id_ = browser_id;
  return true;
}

bool TabManager::Move(int from_index, int to_index) {
  CEF_REQUIRE_UI_THREAD();
  if (from_index < 0 || to_index < 0 ||
      static_cast<std::size_t>(from_index) >= order_.size() ||
      static_cast<std::size_t>(to_index) >= order_.size()) return false;
  if (from_index == to_index) return true;
  const int id = order_[static_cast<std::size_t>(from_index)];
  order_.erase(order_.begin() + from_index);
  order_.insert(order_.begin() + to_index, id);
  return true;
}

CefRefPtr<CefBrowser> TabManager::GetActiveBrowser() const {
  CEF_REQUIRE_UI_THREAD();
  const auto active = browsers_.find(active_browser_id_);
  if (active != browsers_.end()) return active->second;
  return order_.empty() ? nullptr : GetBrowser(order_.front());
}

CefRefPtr<CefBrowser> TabManager::GetBrowser(int browser_id) const {
  CEF_REQUIRE_UI_THREAD();
  const auto found = browsers_.find(browser_id);
  return found == browsers_.end() ? nullptr : found->second;
}

std::vector<CefRefPtr<CefBrowser>> TabManager::GetBrowsers() const {
  CEF_REQUIRE_UI_THREAD();
  std::vector<CefRefPtr<CefBrowser>> result;
  result.reserve(order_.size());
  for (int id : order_) {
    const auto found = browsers_.find(id);
    if (found != browsers_.end()) result.push_back(found->second);
  }
  return result;
}

std::vector<int> TabManager::GetOrderedIds() const {
  CEF_REQUIRE_UI_THREAD();
  return order_;
}

std::size_t TabManager::GetCount() const {
  CEF_REQUIRE_UI_THREAD();
  return browsers_.size();
}

bool TabManager::IsEmpty() const {
  CEF_REQUIRE_UI_THREAD();
  return browsers_.empty();
}
