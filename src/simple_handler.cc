// Copyright (c) 2026 Desktop Browser project.

#include "simple_handler.h"

#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/wrapper/cef_helpers.h"

CefRefPtr<CefLifeSpanHandler> SimpleHandler::GetLifeSpanHandler() {
  return this;
}

void SimpleHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  browser_ = browser;
}

void SimpleHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();
  browser_ = nullptr;
  CefQuitMessageLoop();
}

CefRefPtr<CefBrowser> SimpleHandler::GetBrowser() const {
  return browser_;
}
