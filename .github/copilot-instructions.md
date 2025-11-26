# Copilot instructions for SAST Readium

Purpose: give AI coding agents just enough context to be productive in this C++/Qt6 PDF reader codebase.

## Big picture

- App: Qt6 desktop PDF reader with Poppler-Qt6; logging via spdlog; extensive tests.
- Architecture: MVC + Command pattern + Service Locator + Event Bus + Factories + Plugin hooks.
- Key dirs: `app/` (code), `tests/` (Qt Test), `cmake/` (build utilities), `scripts/` (DX), `docs/` (design & setup).

## Core patterns to follow

- Commands (undo/redo): add under `app/command/` and register in `CommandManager`.
  - Example: `DocumentCommands.cpp`, `NavigationCommands.cpp`, `InitializationCommand.cpp`.
- Service Locator (DI): central access to services in `app/controller/ServiceLocator.*`.
  - Retrieve: `auto* doc = ServiceLocator::instance().getService<DocumentController>();`
  - Register (typically during init): `registerService<MyService>([] { return new MyService(); });`
- Event Bus (decoupled comms): `app/controller/EventBus.*`.
  - Subscribe: `EventBus::instance().subscribe("document_opened", this, handler);`
  - Publish: `EventBus::instance().publish("document_opened", filename);`
- Logging (spdlog + Qt): prefer `app/logging/SimpleLogging.h`.
  - Init: `SastLogging::init();`  Use: `SLOG_INFO("opened: %1").arg(path);`  or `SastLogging::info(...)`.
- Caching: see `app/cache/` (`CacheManager`, `PDFCacheManager`, `PageTextCache`, `SearchResultCache`).
- UI & Controllers: `MainWindow.*` and controllers in `app/controller/` coordinate models and views.

## Build, run, debug

- CMake presets recommended (see `CMakePresets.json`, README):
  - Linux/macOS (system pkgs): `cmake --preset=Debug-Unix; cmake --build --preset=Debug-Unix`.
  - Windows vcpkg: `cmake --preset=Debug-Windows; cmake --build --preset=Debug-Windows`.
  - Windows MSYS2 (recommended on Windows): `.\scripts\build-msys2.sh -d` (run in MSYS2).
- VS Code tasks available (Windows):
  - Configure/Build Debug (MSVC) → builds `build/Debug-Windows` target `app`.
  - Configure/Build Debug (MinGW) → builds `build/Debug-MinGW` target `app`.
- Clangd config is auto-managed by CMake; see `docs/setup/clangd-setup.md`.

## Tests

- Framework: Qt Test with utilities in `tests/TestUtilities.*`; see `tests/README.md`.
- Patterns: test files live under `tests/**`, commonly named `test_*.cpp`; CMake auto-creates targets.
- Quick runs: CTest (`ctest --test-dir <build>`), or PowerShell script `.\scripts\run_tests.ps1` (Windows).

## Conventions and helpers

- Source discovery: targets are composed via helpers in `cmake/TargetUtils.cmake` (e.g., `discover_app_sources()`), and code builds into a library usable by tests.
- Dependency selection: system packages preferred; vcpkg as alternative; MSYS2 recommended for Windows. See `cmake/Dependencies.cmake` and README.
- Logging config: runtime JSON in `config/logging*.json`; use SimpleLogging API instead of direct spdlog in app code.
- Internationalization, theming, recent files, etc. live under `app/managers/`.

## Common extension recipes

- Add a new command:
  1) Create command in `app/command/` (see `DocumentCommands.h`); 2) register in `CommandManager`; 3) bind to UI/shortcut.
- Add a new service:
  1) Define interface/impl; 2) register with `ServiceLocator` during init; 3) retrieve via `getService<T>()` where needed.
- Send cross-component notifications: publish/subscribe via `EventBus` instead of tight coupling.
- Log work: call `SastLogging::init()` early (see `app/main.cpp`), then use `SLOG_*` or `SastLogging::*` helpers.

## Files worth skimming first

- Entry: `app/main.cpp`, `app/MainWindow.*`.
- Patterns: `app/command/CommandManager.*`, `app/controller/ServiceLocator.*`, `app/controller/EventBus.*`.
- Subsystems: `app/cache/*`, `app/search/*`, `app/logging/*`.
- Docs: `docs/architecture.md`, `docs/debugging/index.md`, `docs/logging-system.md`.

If any of the above feels off (e.g., missing registration points or build targets), check `cmake/` and the docs listed in README; this project centralizes those details there.
