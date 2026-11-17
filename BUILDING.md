# Desktop Browser — Build Notes

## Current implementation stage

The first CI milestone was a **CEF integration baseline**. It successfully built the official CEF `cefsimple` sample for Windows x64 using the pinned CEF distribution.

The second milestone introduces the project's own CEF host source code. The phase-two workflow replaces the upstream sample's entry/application/handler source files inside the temporary CEF build tree, then builds the official CEF target with the Windows sandbox and CEF Chrome Bootstrap enabled. The upstream CEF source tree is modified only inside the disposable GitHub Actions runner; no third-party CEF source is committed to this repository.

This is an integration prototype, not the finished browser interface. It currently creates one CEF browser window using CEF's default Chrome runtime style and opens Google as the initial page. The custom Win32 browser shell, tabs, address bar, downloads manager, Split Screen, and the other approved features remain subsequent implementation work; none are cancelled.

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
