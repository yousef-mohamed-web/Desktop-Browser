// Copyright (c) 2026 Desktop Browser project.

#include "simple_handler.h"

#include <cstdlib>
#include <fstream>

#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/wrapper/cef_helpers.h"

CefRefPtr<CefLifeSpanHandler> SimpleHandler::GetLifeSpanHandler() {
  return this;
}

CefRefPtr<CefLoadHandler> SimpleHandler::GetLoadHandler() {
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

void SimpleHandler::OnLoadStart(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                TransitionType transition_type) {
  CEF_REQUIRE_UI_THREAD();
  if (frame && frame->IsMain()) {
    main_frame_load_start_ = std::chrono::steady_clock::now();
  }
}

void SimpleHandler::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefFrame> frame,
                              int http_status_code) {
  CEF_REQUIRE_UI_THREAD();
  if (!frame || !frame->IsMain()) {
    return;
  }

  const auto elapsed = std::chrono::steady_clock::now() -
                       main_frame_load_start_;
  const auto elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

  const char* metrics_path =
      std::getenv("DESKTOP_BROWSER_METRICS_FILE");
  if (metrics_path && metrics_path[0] != '\0') {
    std::ofstream metrics(metrics_path, std::ios::out | std::ios::app);
    if (metrics.is_open()) {
      metrics << "page_load_ms=" << elapsed_ms
              << " http_status=" << http_status_code
              << " url=" << frame->GetURL() << std::endl;
    }
  }
}

CefRefPtr<CefBrowser> SimpleHandler::GetBrowser() const {
  return browser_;
}
