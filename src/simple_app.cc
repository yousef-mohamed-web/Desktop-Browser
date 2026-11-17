// Copyright (c) 2026 Desktop Browser project.
// Minimal native Win32 host window containing a CEF browser child window.

#include "simple_app.h"

#include <windows.h>

#include <string>

#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/cef_request_context.h"
#include "include/wrapper/cef_helpers.h"
#include "simple_handler.h"

namespace {

constexpr wchar_t kWindowClassName[] = L"DesktopBrowserCEFHostWindow";

SimpleHandler* GetWindowHandler(HWND window) {
  return reinterpret_cast<SimpleHandler*>(
      GetWindowLongPtrW(window, GWLP_USERDATA));
}

LRESULT CALLBACK BrowserWindowProc(HWND window,
                                   UINT message,
                                   WPARAM wparam,
                                   LPARAM lparam) {
  if (message == WM_NCCREATE) {
    const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lparam);
    SetWindowLongPtrW(
        window, GWLP_USERDATA,
        reinterpret_cast<LONG_PTR>(create->lpCreateParams));
  }

  SimpleHandler* handler = GetWindowHandler(window);

  switch (message) {
    case WM_SIZE: {
      if (handler) {
        CefRefPtr<CefBrowser> browser = handler->GetBrowser();
        if (browser) {
          HWND browser_window = browser->GetHost()->GetWindowHandle();
          if (browser_window) {
            const int width = LOWORD(lparam);
            const int height = HIWORD(lparam);
            SetWindowPos(browser_window, nullptr, 0, 0, width, height,
                         SWP_NOZORDER | SWP_NOACTIVATE);
          }
        }
      }
      return 0;
    }

    case WM_CLOSE: {
      if (handler) {
        CefRefPtr<CefBrowser> browser = handler->GetBrowser();
        if (browser && !browser->GetHost()->TryCloseBrowser()) {
          return 0;
        }
      }
      DestroyWindow(window);
      return 0;
    }

    case WM_DESTROY:
      return 0;
  }

  return DefWindowProcW(window, message, wparam, lparam);
}

bool RegisterBrowserWindowClass(HINSTANCE instance) {
  WNDCLASSEXW window_class = {};
  window_class.cbSize = sizeof(window_class);
  window_class.style = CS_HREDRAW | CS_VREDRAW;
  window_class.lpfnWndProc = BrowserWindowProc;
  window_class.hInstance = instance;
  window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  window_class.hbrBackground =
      reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
  window_class.lpszClassName = kWindowClassName;

  if (RegisterClassExW(&window_class)) {
    return true;
  }

  return GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

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

  HINSTANCE instance = GetModuleHandleW(nullptr);
  if (!RegisterBrowserWindowClass(instance)) {
    CefQuitMessageLoop();
    return;
  }

  HWND window = CreateWindowExW(
      0, kWindowClassName, L"Desktop Browser - CEF Host",
      WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1100, 760,
      nullptr, nullptr, instance, handler_.get());

  if (!window) {
    CefQuitMessageLoop();
    return;
  }

  ShowWindow(window, SW_SHOWNORMAL);
  UpdateWindow(window);

  RECT client_rect = {};
  GetClientRect(window, &client_rect);

  CefWindowInfo window_info;
  window_info.SetAsChild(
      window,
      CefRect(0, 0, client_rect.right - client_rect.left,
              client_rect.bottom - client_rect.top));

  // Alloy runtime style is intentional here: the project owns the native
  // Win32 top-level window and will add its own browser controls in later
  // implementation stages. CEF Chrome Bootstrap and the sandbox remain on.
  window_info.runtime_style = CEF_RUNTIME_STYLE_ALLOY;

  std::string initial_url =
      CefCommandLine::GetGlobalCommandLine()->GetSwitchValue("url");
  if (initial_url.empty()) {
    initial_url = "https://www.google.com/";
  }

  CefBrowserSettings browser_settings;
  if (!CefBrowserHost::CreateBrowser(
          window_info, handler_, initial_url, browser_settings, nullptr,
          nullptr)) {
    DestroyWindow(window);
    CefQuitMessageLoop();
  }
}
