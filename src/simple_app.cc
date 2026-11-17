// Copyright (c) 2026 Desktop Browser project.

#include "simple_app.h"

#include <string>

#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_helpers.h"
#include "simple_handler.h"

namespace {

class BrowserWindowDelegate : public CefWindowDelegate {
 public:
  explicit BrowserWindowDelegate(CefRefPtr<CefBrowserView> browser_view)
      : browser_view_(browser_view) {}

  void OnWindowCreated(CefRefPtr<CefWindow> window) override {
    window->AddChildView(browser_view_);
    window->Show();
  }

  void OnWindowDestroyed(CefRefPtr<CefWindow> window) override {
    browser_view_ = nullptr;
  }

  bool CanClose(CefRefPtr<CefWindow> window) override {
    CefRefPtr<CefBrowser> browser = browser_view_->GetBrowser();
    return !browser || browser->GetHost()->TryCloseBrowser();
  }

  CefSize GetPreferredSize(CefRefPtr<CefView> view) override {
    return CefSize(1000, 700);
  }

  cef_runtime_style_t GetWindowRuntimeStyle() override {
    return CEF_RUNTIME_STYLE_DEFAULT;
  }

 private:
  CefRefPtr<CefBrowserView> browser_view_;

  IMPLEMENT_REFCOUNTING(BrowserWindowDelegate);
  DISALLOW_COPY_AND_ASSIGN(BrowserWindowDelegate);
};

class BrowserViewDelegate : public CefBrowserViewDelegate {
 public:
  bool OnPopupBrowserViewCreated(
      CefRefPtr<CefBrowserView> browser_view,
      CefRefPtr<CefBrowserView> popup_browser_view,
      bool is_devtools) override {
    CefWindow::CreateTopLevelWindow(
        new BrowserWindowDelegate(popup_browser_view));
    return true;
  }

  cef_runtime_style_t GetBrowserRuntimeStyle() override {
    return CEF_RUNTIME_STYLE_DEFAULT;
  }

 private:
  IMPLEMENT_REFCOUNTING(BrowserViewDelegate);
  DISALLOW_COPY_AND_ASSIGN(BrowserViewDelegate);
};

}  // namespace

SimpleApp::SimpleApp() : handler_(new SimpleHandler()) {}

CefRefPtr<CefBrowserProcessHandler> SimpleApp::GetBrowserProcessHandler() {
  return this;
}

CefRefPtr<CefClient> SimpleApp::GetDefaultClient() {
  return handler_;
}

void SimpleApp::OnContextInitialized() {
  CEF_REQUIRE_UI_THREAD();

  CefRefPtr<CefCommandLine> command_line =
      CefCommandLine::GetGlobalCommandLine();

  std::string initial_url = command_line->GetSwitchValue("url");
  if (initial_url.empty()) {
    initial_url = "https://www.google.com/";
  }

  CefBrowserSettings browser_settings;
  CefRefPtr<CefBrowserView> browser_view =
      CefBrowserView::CreateBrowserView(
          handler_, initial_url, browser_settings, nullptr, nullptr,
          new BrowserViewDelegate());

  if (browser_view) {
    CefWindow::CreateTopLevelWindow(
        new BrowserWindowDelegate(browser_view));
  }
}
