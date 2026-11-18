// Copyright (c) 2026 Desktop Browser project.

#ifndef DESKTOP_BROWSER_SIMPLE_HANDLER_H_
#define DESKTOP_BROWSER_SIMPLE_HANDLER_H_

#include <chrono>

#include "include/cef_client.h"
#include "include/cef_load_handler.h"

class SimpleHandler : public CefClient,
                      public CefLifeSpanHandler,
                      public CefLoadHandler {
 public:
  SimpleHandler() = default;

  CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override;
  CefRefPtr<CefLoadHandler> GetLoadHandler() override;
  void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
  void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;
  void OnLoadStart(CefRefPtr<CefBrowser> browser,
                   CefRefPtr<CefFrame> frame,
                   TransitionType transition_type) override;
  void OnLoadEnd(CefRefPtr<CefBrowser> browser,
                 CefRefPtr<CefFrame> frame,
                 int http_status_code) override;

  CefRefPtr<CefBrowser> GetBrowser() const;

 private:
  CefRefPtr<CefBrowser> browser_;
  std::chrono::steady_clock::time_point main_frame_load_start_;

  IMPLEMENT_REFCOUNTING(SimpleHandler);
  DISALLOW_COPY_AND_ASSIGN(SimpleHandler);
};

#endif  // DESKTOP_BROWSER_SIMPLE_HANDLER_H_
