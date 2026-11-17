// Copyright (c) 2026 Desktop Browser project.
// CEF process entry point for the Windows x64 host.

#include <windows.h>

#include "include/cef_sandbox_win.h"
#include "include/cef_version.h"
#include "simple_app.h"

namespace {

int RunMain(HINSTANCE instance, void* sandbox_info) {
  CefMainArgs main_args(instance);

  // The same executable is launched for the browser and Chromium child
  // processes. Child processes must be dispatched before browser startup.
  const int exit_code = CefExecuteProcess(main_args, nullptr, sandbox_info);
  if (exit_code >= 0) {
    return exit_code;
  }

  CefSettings settings;
  settings.no_sandbox = false;
  settings.multi_threaded_message_loop = false;

  CefRefPtr<SimpleApp> app(new SimpleApp());
  if (!CefInitialize(main_args, settings, app.get(), sandbox_info)) {
    return CefGetExitCode();
  }

  CefRunMessageLoop();
  CefShutdown();
  return 0;
}

}  // namespace

int APIENTRY wWinMain(HINSTANCE instance,
                      HINSTANCE previous_instance,
                      LPWSTR command_line,
                      int show_command) {
  UNREFERENCED_PARAMETER(previous_instance);
  UNREFERENCED_PARAMETER(command_line);
  UNREFERENCED_PARAMETER(show_command);

  void* sandbox_info = nullptr;
#if defined(CEF_USE_SANDBOX)
  CefScopedSandboxInfo scoped_sandbox;
  sandbox_info = scoped_sandbox.sandbox_info();
#endif

  return RunMain(instance, sandbox_info);
}
