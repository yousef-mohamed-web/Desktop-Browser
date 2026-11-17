# Desktop Browser — Build Notes

## Current implementation stage

The first CI milestone is a **CEF integration baseline**, not the finished browser UI.

GitHub Actions downloads the official CEF Windows x64 standard binary distribution, verifies its SHA-1 checksum against the official sidecar file, and builds the upstream `cefsimple` sample with Visual Studio 2022 / CMake for x64.

This validates the selected CEF distribution and the Windows build pipeline before project-specific browser code is introduced. The sample is intentionally not presented as the completed Desktop Browser.

## Pinned CEF baseline

- CEF: `156.0.14+g4fa4ab5+chromium-156.0.8078.48`
- Chromium: `156.0.8078.48`
- Distribution: official Windows x64 standard binary distribution
- Build toolchain: Visual Studio 2022, CMake, x64
- CI runner: GitHub Actions `windows-2022`

CEF release binaries are tied to Chromium releases. Keep this version pinned during the initial integration milestone. Engine upgrades must be reviewed and tested deliberately; they must never become an automatic end-user browser update.

## Run the build

1. Open the repository's **Actions** tab.
2. Select **CEF Windows x64 Baseline**.
3. Open the latest run for branch `build/cef-windows-x64-baseline`.
4. When the job succeeds, download the `cef-windows-x64-baseline` artifact.

The artifact contains the upstream `cefsimple` executable and its runtime files. It is a technical smoke-test artifact, not yet the branded browser application.

## Next implementation milestone

After the baseline succeeds, replace the upstream sample entry point with the project's own minimal Win32 + CEF host. Validate CEF initialization and shutdown, browser rendering, subprocess startup, sandbox behavior, and navigation before implementing the full browser shell.

## Important constraints

- Windows 10 x64 is the target deployment platform.
- The application itself is built as x64; Win32 refers to the Windows API, not a 32-bit executable.
- Do not disable the Chromium sandbox or collapse Chromium into a single process to make CI pass.
- Do not add CEF binary archives or generated build outputs to Git.
- The end-user browser update policy remains manual-only as specified in `Technical_Specification.txt`.
