# Desktop Browser — Build Notes

## Current implementation stage

The first CI milestone was a **CEF integration baseline**. It successfully built the official CEF `cefsimple` sample for Windows x64 using the pinned CEF distribution.

The second milestone introduces the project's own CEF host source code. The phase-two workflow places our entry/application/handler source files into the temporary CEF sample build target, then builds it with the Windows sandbox and CEF Chrome Bootstrap enabled. The upstream CEF source tree is modified only inside the disposable GitHub Actions runner; no third-party CEF source is committed to this repository.

This is an integration prototype, not the finished browser interface. It creates a native Win32 top-level window and embeds one CEF browser child using Alloy runtime style, which is appropriate for a project-owned native browser shell. The CEF Chrome Bootstrap process model and Chromium sandbox remain enabled. It opens Google as the initial test page. The address bar, tabs, downloads manager, Split Screen, and the other approved features remain subsequent implementation work; none are cancelled.

## Pinned CEF baseline

- CEF: `156.0.14+g4fa4ab5+chromium-156.0.8078.48`
- Chromium: `156.0.8078.48`
- Distribution: official Windows x64 standard binary distribution
- Build toolchain: Visual Studio 2022, CMake, x64
- CI runner: GitHub Actions `windows-2022`

Keep this version pinned during initial integration. Engine upgrades must be reviewed and tested deliberately; end-user browser updates remain manual-only.

## Workflows

### CEF baseline

`.github/workflows/cef-baseline.yml` builds the upstream `cefsimple` sample. This validates the selected CEF package and Windows build toolchain.

### Project CEF host

`.github/workflows/cef-host.yml` builds the project's own host sources with:

- Windows x64 / Release
- CEF Chrome Bootstrap
- Windows sandbox enabled (`USE_SANDBOX=ON`)
- Chromium child-process dispatch through the same application entry point
- One initial browser view and a controlled CEF message loop

The output artifact is `desktop-browser-cef-host-x64`. It is a technical prototype for CI inspection, not a release installer.

## Run the phase-two build

1. Open the repository's **Actions** tab.
2. Select **Desktop Browser - CEF Windows x64 Host**.
3. Select branch `implementation/cef-win32-host`.
4. Run the workflow and wait for all steps to finish.
5. Inspect the `desktop-browser-cef-host-x64` artifact only after a successful run.

The workflow verifies the official CEF archive checksum before extraction and checks that the sandboxed bootstrap executable, application DLL, CEF runtime DLL, resources, and locales are staged.

## Important constraints

- Windows 10 x64 is the target deployment platform.
- Win32 refers to the Windows API; the executable is x64.
- Do not disable the Chromium sandbox or use single-process mode to make CI pass.
- Do not add CEF binary archives or generated build outputs to Git.
- Do not remove or reduce any approved browser feature without the user's explicit approval.
- Normal startup will eventually open the Home Page only; session restore remains user-initiated.
- The end-user browser update policy remains manual-only as specified in `Technical_Specification.txt`.


## Resource-management foundation (phase three)

The project host now includes a UI-thread-owned `TabManager` registry for multiple CEF browser instances. The registry tracks browser identifiers, the active browser, and lifecycle callbacks without defining or changing the browser's tab UI.

A read-only `ResourceMonitor` samples the host process working set/private bytes, system available physical memory, memory load, and host-process CPU usage every three seconds. When `DESKTOP_BROWSER_METRICS_FILE` is set, these samples are appended to that metrics file.

`AdaptiveMemoryManager` currently classifies system memory pressure (normal/moderate/high/critical) only. It deliberately performs no tab suspension, discard, reload, process-priority change, or other disruptive action. The monitor currently measures the browser host process, not the aggregate CEF child-process tree; aggregate measurement remains part of the external performance harness.

The CEF host workflow copies and compiles the new runtime components inside the disposable official CEF sample build. No CEF binaries or upstream CEF source are committed to this repository.


## Browser shell implementation status

The native Win32 shell now provides the first navigation and tab-management slice:
- Built-in lightweight Home Page with a search field and common website shortcuts.
- Address/search entry, Back, Forward, Reload, Stop, and Home actions.
- Multiple CEF browser instances represented as rectangular tabs, with title updates and tab switching.
- New-tab and close-tab actions; closing the last tab returns to the Home Page.
- Ctrl+L, Ctrl+T, Ctrl+W, Ctrl+R, Ctrl+Tab, and F5 shortcuts.
- Hidden browser views receive the supported WasHidden notification; this is visibility scheduling, not destructive tab suspension.

This is an implementation stage, not a claim that the first-release browser is complete. Downloads, bookmarks, history, settings, favicon retrieval, window management, split screen, privacy/profile isolation, and the remaining requirements in Technical_Specification.txt remain required follow-on work. The visible placeholder entries are not implemented feature functionality.
