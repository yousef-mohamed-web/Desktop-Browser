// Copyright (c) 2026 Desktop Browser project.

#ifndef DESKTOP_BROWSER_SIMPLE_APP_H_
#define DESKTOP_BROWSER_SIMPLE_APP_H_

#include "include/cef_app.h"

class SimpleHandler;

class SimpleApp : public CefApp, public CefBrowserProcessHandler {
 public:
  SimpleApp();

  CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override;
  CefRefPtr<CefClient> GetDefaultClient() override;
  void OnContextInitialized() override;

 private:
  CefRefPtr<SimpleHandler> handler_;

  IMPLEMENT_REFCOUNTING(SimpleApp);
  DISALLOW_COPY_AND_ASSIGN(SimpleApp);
};

#endif  // DESKTOP_BROWSER_SIMPLE_APP_H_
