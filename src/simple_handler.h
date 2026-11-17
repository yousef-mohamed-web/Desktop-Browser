// Copyright (c) 2026 Desktop Browser project.

#ifndef DESKTOP_BROWSER_SIMPLE_HANDLER_H_
#define DESKTOP_BROWSER_SIMPLE_HANDLER_H_

#include "include/cef_client.h"

class SimpleHandler : public CefClient, public CefLifeSpanHandler {
 public:
  SimpleHandler() = default;

  CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override;
  void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
  void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

 private:
  int browser_count_ = 0;

  IMPLEMENT_REFCOUNTING(SimpleHandler);
  DISALLOW_COPY_AND_ASSIGN(SimpleHandler);
};

#endif  // DESKTOP_BROWSER_SIMPLE_HANDLER_H_
