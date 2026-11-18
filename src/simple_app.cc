// Copyright (c) 2026 Desktop Browser project.
// Native Win32 browser shell. Website rendering is provided by CEF.

#include "simple_app.h"

#include <windows.h>
#include <algorithm>
#include <string>
#include <vector>

#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/wrapper/cef_helpers.h"
#include "simple_handler.h"

namespace {
constexpr wchar_t kWindowClassName[] = L"DesktopBrowserCEFHostWindow";
constexpr UINT_PTR kResourceMonitorTimerId = 1;
constexpr UINT kResourceMonitorIntervalMs = 3000;
constexpr UINT kShellChangedMessage = WM_APP + 41;
constexpr UINT kAddressSubmitMessage = WM_APP + 42;
constexpr int kTabStripHeight = 32;
constexpr int kToolbarHeight = 40;
constexpr int kContentTop = kTabStripHeight + kToolbarHeight;

enum ControlId {
  kBack = 100, kForward, kReload, kStop, kHome, kAddress, kNewTab,
  kCloseTab, kDownloads, kHistory, kBookmarks, kTools, kSettings
};

HWND g_address = nullptr;
HWND g_tab_buttons[128] = {};
int g_tab_button_count = 0;
HFONT g_ui_font = nullptr;
WNDPROC g_original_address_proc = nullptr;

SimpleHandler* GetWindowHandler(HWND window) {
  return reinterpret_cast<SimpleHandler*>(
      GetWindowLongPtrW(window, GWLP_USERDATA));
}

std::string WideToUtf8(const std::wstring& value) {
  if (value.empty()) return {};
  const int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(),
      static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
  std::string result(static_cast<size_t>(size), '\0');
  if (size > 0)
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(),
        static_cast<int>(value.size()), result.data(), size, nullptr, nullptr);
  return result;
}

std::string PercentEncode(const std::string& value) {
  static const char hex[] = "0123456789ABCDEF";
  std::string result;
  for (unsigned char ch : value) {
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
        (ch >= '0' && ch <= '9') || ch == '-' || ch == '_' ||
        ch == '.' || ch == '~') {
      result.push_back(static_cast<char>(ch));
    } else {
      result.push_back('%');
      result.push_back(hex[ch >> 4]);
      result.push_back(hex[ch & 15]);
    }
  }
  return result;
}

std::string ResolveAddress(const std::wstring& input) {
  const size_t first = input.find_first_not_of(L" \t\r\n");
  if (first == std::wstring::npos) return "about:blank";
  const size_t last = input.find_last_not_of(L" \t\r\n");
  const std::wstring value = input.substr(first, last - first + 1);
  const std::string utf8 = WideToUtf8(value);
  if (utf8.find("://") != std::string::npos ||
      utf8.rfind("about:", 0) == 0 || utf8.rfind("file:", 0) == 0)
    return utf8;
  if (value.find(L' ') != std::wstring::npos ||
      (value.find(L'.') == std::wstring::npos &&
       value.find(L':') == std::wstring::npos))
    return "https://www.google.com/search?q=" + PercentEncode(utf8);
  return "https://" + utf8;
}

std::string BuiltInHomePage() {
  static const char html[] =
      "<!doctype html><html><head><meta charset='utf-8'><title>Home</title>"
      "<style>body{font:16px Segoe UI,Arial;background:#f4f6f8;color:#202124;"
      "margin:0}.wrap{max-width:760px;margin:13vh auto;padding:24px;text-align:center}"
      "h1{font-size:34px;font-weight:500}form{display:flex;gap:8px;margin:28px 0}"
      "input{flex:1;padding:15px 18px;border:1px solid #dadce0;border-radius:24px;"
      "font-size:17px}button{padding:0 22px;border:0;border-radius:20px;"
      "background:#1a73e8;color:white;font-size:16px}.links{display:flex;"
      "justify-content:center;gap:18px;flex-wrap:wrap}.links a{color:#1a73e8;"
      "text-decoration:none;padding:12px}</style></head><body><main class='wrap'>"
      "<h1>Desktop Browser</h1><form action='https://www.google.com/search'>"
      "<input name='q' autofocus placeholder='Search Google or enter an address'>"
      "<button>Search</button></form><nav class='links'>"
      "<a href='https://www.youtube.com/'>YouTube</a>"
      "<a href='https://www.wikipedia.org/'>Wikipedia</a>"
      "<a href='https://github.com/'>GitHub</a>"
      "<a href='https://mail.google.com/'>Gmail</a>"
      "</nav></main></body></html>";
  return "data:text/html;charset=utf-8," + PercentEncode(html);
}

void SetControlFont(HWND control) {
  if (control && g_ui_font)
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(g_ui_font), TRUE);
}

HWND AddButton(HWND parent, const wchar_t* text, int id, int width) {
  HWND control = CreateWindowExW(0, L"BUTTON", text,
      WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
      0, 0, width, 30, parent,
      reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
      GetModuleHandleW(nullptr), nullptr);
  SetControlFont(control);
  return control;
}

LRESULT CALLBACK AddressEditProc(HWND edit, UINT message,
                                 WPARAM wparam, LPARAM lparam) {
  if (message == WM_KEYDOWN && wparam == VK_RETURN) {
    SendMessageW(GetParent(edit), kAddressSubmitMessage, 0,
                 reinterpret_cast<LPARAM>(edit));
    return 0;
  }
  return CallWindowProcW(g_original_address_proc, edit, message, wparam, lparam);
}

void CreateBrowserControls(HWND window) {
  g_ui_font = CreateFontW(-15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
      CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

  AddButton(window, L"‹", kBack, 36);
  AddButton(window, L"›", kForward, 36);
  AddButton(window, L"↻", kReload, 36);
  AddButton(window, L"×", kStop, 32);
  AddButton(window, L"⌂", kHome, 36);

  g_address = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
      WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
      0, 0, 400, 30, window,
      reinterpret_cast<HMENU>(static_cast<INT_PTR>(kAddress)),
      GetModuleHandleW(nullptr), nullptr);
  SetControlFont(g_address);
  g_original_address_proc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
      g_address, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(AddressEditProc)));

  AddButton(window, L"+", kNewTab, 36);
  AddButton(window, L"Close tab", kCloseTab, 78);
  AddButton(window, L"Downloads", kDownloads, 82);
  AddButton(window, L"History", kHistory, 72);
  AddButton(window, L"Bookmarks", kBookmarks, 86);
  AddButton(window, L"Tools", kTools, 62);
  AddButton(window, L"Settings", kSettings, 76);
}

void SubmitAddress(HWND window, SimpleHandler* handler) {
  if (!handler || !g_address) return;
  const int length = GetWindowTextLengthW(g_address);
  std::wstring text(static_cast<size_t>(length) + 1, L'\0');
  GetWindowTextW(g_address, text.data(), length + 1);
  text.resize(static_cast<size_t>(length));
  handler->NavigateActive(ResolveAddress(text));
}

void LayoutControls(HWND window, SimpleHandler* handler) {
  RECT rc = {};
  GetClientRect(window, &rc);
  const int width = rc.right - rc.left;
  const int height = rc.bottom - rc.top;
  const int y = kTabStripHeight + 4;
  int x = 6;
  const int ids[] = {kBack, kForward, kReload, kStop, kHome};
  const int widths[] = {36, 36, 36, 32, 36};
  for (size_t i = 0; i < 5; ++i) {
    HWND control = GetDlgItem(window, ids[i]);
    if (control)
      SetWindowPos(control, nullptr, x, y, widths[i], 30,
                   SWP_NOZORDER | SWP_NOACTIVATE);
    x += widths[i] + 2;
  }

  const int tail_ids[] = {kNewTab, kCloseTab, kDownloads, kHistory,
                          kBookmarks, kTools, kSettings};
  const int tail_widths[] = {36, 78, 82, 72, 86, 62, 76};
  int tail_total = 0;
  for (int w : tail_widths) tail_total += w + 2;
  const int address_width = (std::max)(120, width - x - tail_total - 12);
  if (g_address)
    SetWindowPos(g_address, nullptr, x, y, address_width, 30,
                 SWP_NOZORDER | SWP_NOACTIVATE);
  x += address_width + 4;
  for (size_t i = 0; i < 7; ++i) {
    HWND control = GetDlgItem(window, tail_ids[i]);
    if (control)
      SetWindowPos(control, nullptr, x, y, tail_widths[i], 30,
                   SWP_NOZORDER | SWP_NOACTIVATE);
    x += tail_widths[i] + 2;
  }

  for (int i = 0; i < g_tab_button_count; ++i) {
    if (g_tab_buttons[i]) DestroyWindow(g_tab_buttons[i]);
    g_tab_buttons[i] = nullptr;
  }
  g_tab_button_count = 0;
  int tab_x = 4;
  if (handler) {
    for (int browser_id : handler->GetTabIds()) {
      if (g_tab_button_count >= 128) break;
      std::wstring title = handler->GetTabTitle(browser_id);
      if (title.empty()) title = L"New Tab";
      if (title.size() > 22) title = title.substr(0, 19) + L"...";
      const auto active = handler->GetBrowser();
      if (active && active->GetIdentifier() == browser_id)
        title = L"● " + title;
      HWND button = CreateWindowExW(0, L"BUTTON", title.c_str(),
          WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
          tab_x, 1, 180, 29, window,
          reinterpret_cast<HMENU>(static_cast<INT_PTR>(5000 + g_tab_button_count)),
          GetModuleHandleW(nullptr), nullptr);
      SetWindowLongPtrW(button, GWLP_USERDATA, static_cast<LONG_PTR>(browser_id));
      SetControlFont(button);
      g_tab_buttons[g_tab_button_count++] = button;
      tab_x += 182;
    }
    handler->LayoutBrowsers(0, kContentTop, width,
                            (std::max)(1, height - kContentTop));
  }
}

LRESULT CALLBACK BrowserWindowProc(HWND window, UINT message,
                                   WPARAM wparam, LPARAM lparam) {
  if (message == WM_NCCREATE) {
    const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lparam);
    SetWindowLongPtrW(window, GWLP_USERDATA,
                      reinterpret_cast<LONG_PTR>(create->lpCreateParams));
  }
  SimpleHandler* handler = GetWindowHandler(window);

  switch (message) {
    case WM_CREATE:
      CreateBrowserControls(window);
      return 0;

    case kShellChangedMessage: {
      LayoutControls(window, handler);
      if (handler && g_address) {
        auto browser = handler->GetBrowser();
        if (browser && browser->GetMainFrame()) {
          const std::wstring url = browser->GetMainFrame()->GetURL().ToWString();
          SetWindowTextW(g_address, url.c_str());
        }
      }
      return 0;
    }

    case kAddressSubmitMessage:
      SubmitAddress(window, handler);
      return 0;

    case WM_SIZE:
      LayoutControls(window, handler);
      return 0;

    case WM_COMMAND: {
      const int id = LOWORD(wparam);
      if (!handler) break;
      switch (id) {
        case kBack: handler->GoBack(); return 0;
        case kForward: handler->GoForward(); return 0;
        case kReload: handler->Reload(); return 0;
        case kStop: handler->StopLoad(); return 0;
        case kHome: handler->NavigateActive(BuiltInHomePage()); return 0;
        case kNewTab: handler->CreateBrowserTab(BuiltInHomePage()); return 0;
        case kCloseTab: handler->CloseActiveTab(); return 0;
        case kDownloads:
        case kHistory:
        case kBookmarks:
        case kTools:
        case kSettings:
          MessageBoxW(window,
              L"This feature is part of the approved browser scope and is being implemented in a later stage.",
              L"Desktop Browser", MB_OK | MB_ICONINFORMATION);
          return 0;
        default:
          if (lparam && id >= 5000 && id < 5128) {
            const int browser_id = static_cast<int>(GetWindowLongPtrW(
                reinterpret_cast<HWND>(lparam), GWLP_USERDATA));
            handler->SetActiveBrowser(browser_id);
            return 0;
          }
          break;
      }
      break;
    }

    case WM_KEYDOWN:
      if (GetKeyState(VK_CONTROL) & 0x8000) {
        switch (wparam) {
          case 'L':
            SetFocus(g_address);
            SendMessageW(g_address, EM_SETSEL, 0, -1);
            return 0;
          case 'T': handler->CreateBrowserTab(BuiltInHomePage()); return 0;
          case 'W': handler->CloseActiveTab(); return 0;
          case 'R': handler->Reload(); return 0;
          case VK_TAB:
            handler->ActivateNextTab((GetKeyState(VK_SHIFT) & 0x8000) != 0);
            return 0;
        }
      }
      if (wparam == VK_F5) { handler->Reload(); return 0; }
      break;

    case WM_TIMER:
      if (wparam == kResourceMonitorTimerId && handler) {
        handler->RecordResourceSample();
        return 0;
      }
      break;

    case WM_CLOSE:
      if (handler && !handler->TryCloseAllBrowsers()) return 0;
      DestroyWindow(window);
      return 0;

    case WM_DESTROY:
      KillTimer(window, kResourceMonitorTimerId);
      return 0;
  }
  return DefWindowProcW(window, message, wparam, lparam);
}

bool RegisterBrowserWindowClass(HINSTANCE instance) {
  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(wc);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = BrowserWindowProc;
  wc.hInstance = instance;
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
  wc.lpszClassName = kWindowClassName;
  return RegisterClassExW(&wc) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
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
      0, kWindowClassName, L"Desktop Browser",
      WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1200, 820,
      nullptr, nullptr, instance, handler_.get());
  if (!window) {
    CefQuitMessageLoop();
    return;
  }

  handler_->SetMainWindow(window);
  SetTimer(window, kResourceMonitorTimerId, kResourceMonitorIntervalMs, nullptr);
  ShowWindow(window, SW_SHOWNORMAL);
  UpdateWindow(window);
  // Honor an explicit startup URL (used by automated runtime tests and
  // useful for command-line launching). Fall back to the built-in home page.
  CefRefPtr<CefCommandLine> command_line =
      CefCommandLine::GetGlobalCommandLine();
  const CefString startup_url =
      command_line ? command_line->GetSwitchValue("url") : CefString();
  const std::string initial_url =
      startup_url.empty() ? BuiltInHomePage() : startup_url.ToString();
  handler_->CreateBrowserTab(initial_url);
}
