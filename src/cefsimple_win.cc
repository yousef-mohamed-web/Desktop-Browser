// Copyright (c) 2026 Desktop Browser project.
// Windows entry point for the CEF Chrome Bootstrap integration.

#include <windows.h>

#include "include/cef_app.h"
#include "include/cef_sandbox_win.h"
#include "include/cef_version_info.h"
#include "simple_app.h"

#if !defined(CEF_USE_BOOTSTRAP) && !defined(CEF_USE_SANDBOX)
#error "Build with CEF sandbox support enabled. Unsandboxed builds are not accepted."
#endif

namespace {

int RunMain(HINSTANCE instance, LPWSTR command_line, int show_command,
            void* sandbox_info) {
  UNREFERENCED_PARAMETER(command_line);
  UNREFERENCED_PARAMETER(show_command);

  CefMainArgs main_args(instance);

  // The same executable entry is used for the browser and Chromium child
  // processes. Dispatch child processes before initializing the browser.
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

#if defined(CEF_USE_BOOTSTRAP)

// The official CEF bootstrap executable loads this function from the
// application DLL and supplies the sandbox information.
CEF_BOOTSTRAP_EXPORT int RunWinMain(HINSTANCE instance,
                                    LPWSTR command_line,
                                    int show_command,
                                    void* sandbox_info,
                                    cef_version_info_t* version_info) {
  UNREFERENCED_PARAMETER(version_info);
  return RunMain(instance, command_line, show_command, sandbox_info);
}

#else

int APIENTRY wWinMain(HINSTANCE instance,
                      HINSTANCE previous_instance,
                      LPWSTR command_line,
                      int show_command) {
  UNREFERENCED_PARAMETER(previous_instance);

  void* sandbox_info = nullptr;
#if defined(CEF_USE_SANDBOX)
  CefScopedSandboxInfo scoped_sandbox;
  sandbox_info = scoped_sandbox.sandbox_info();
#endif

  return RunMain(instance, command_line, show_command, sandbox_info);
}

#endif
