# SAST Readium Documentation

Welcome to the comprehensive documentation for SAST Readium, a modern Qt6-based PDF reader application with advanced architecture and extensibility.

## Quick Navigation

### 🚀 [Getting Started](getting-started/)

Essential information to get you up and running with SAST Readium:

- [Dependency Management](getting-started/dependency-management.md) - Comprehensive dependency strategy
- [Platform Support](getting-started/platform-support.md) - Supported platforms and architectures

### 🏗️ [Architecture](architecture.md)

Comprehensive architecture documentation:

- [Architecture Overview](architecture.md) - Design patterns and component organization
- [API Reference](api-reference.md) - Detailed API documentation for core components
- [Thread Safety Guidelines](thread-safety-guidelines.md) - Thread safety best practices

### 🔧 [Build Systems](build-systems/)

Build system documentation and guides:

- [Build System Comparison](build-systems/build-system-comparison.md) - CMake vs Xmake comparison
- [CMake Modules](../cmake/README.md) - Detailed CMake module documentation
- [Migration Guide](MIGRATION-GUIDE.md) - Guide for migrating to the new build system

### ⚙️ [Setup & Configuration](setup/)

Development environment setup guides:

- [MSYS2 Build Setup](setup/msys2-build.md) - Windows MSYS2 environment setup
- [clangd IDE Setup](setup/clangd-setup.md) - IDE integration and language server configuration
- [Build Artifact Management](setup/build-artifact-management.md) - Keep build outputs organized and IDE-aware

### ✨ [Features](features/)

Application features and implementation details:

- [Thumbnail System](features/thumbnail-system.md) - Chrome-style PDF thumbnail system
- [Highlight System](features/highlight-system.md) - Comprehensive text highlighting functionality
- [Viewing Experience](features/viewing-experience.md) - Themes, links, and navigation
- [Security & Protection](features/security-and-protection.md) - Passwords and watermarks
- [Text-to-Speech](features/text-to-speech.md) - Accessibility features

### [Plugin System](plugins/index.md)

Extensible plugin architecture:

- [Development Guide](plugins/development-guide.md) - Create your first plugin
- [Architecture](plugins/architecture.md) - Internal design and components
- [UI Integration](plugins/ui-integration.md) - Extending the user interface
- [Specialized Plugins](plugins/specialized-plugins.md) - Deep integration plugins

### [Technical Documentation](architecture.md)

In-depth technical documentation:

- [Architecture](architecture.md) - System architecture and design patterns
- [API Reference](api-reference.md) - API documentation
- [Logging System](logging-system.md) - Logging infrastructure
- [Packaging Guide](packaging/index.md) - Packaging instructions
- [Thread Safety](thread-safety-guidelines.md) - Thread safety guidelines

### 🛠️ [Debugging](debugging/)

Troubleshooting and diagnostics:

- [Debugging Setup](debugging/index.md) - Cross-platform debugger and IDE setup
- [Freeze & Deadlock Debugging](debugging/freeze-debugging.md) - GDB workflow for freeze investigations

## About SAST Readium

SAST Readium is a Qt6-based PDF reader application that provides:

- **Modern Architecture**: Built with proven design patterns (Command, Service Locator, Event Bus, Factory, Plugin)
- **High Performance**: Optimized rendering with Poppler-Qt6, smart caching, and asynchronous operations
- **Extensibility**: Plugin system and well-defined extension points
- **Cross-Platform**: Support for Windows, Linux, and macOS with cross-compilation toolchains
- **Developer-Friendly**: Comprehensive documentation, IDE integration, and 100+ tests
- **Modern UI**: Clean, intuitive interface with Chrome-style thumbnails and theme support

## Key Features

### Architecture Patterns

- **Command Pattern**: Full undo/redo support with `CommandManager`
- **Service Locator**: Dependency injection for loose coupling
- **Event Bus**: Decoupled publish-subscribe communication
- **Factory Pattern**: Standardized object creation with `ModelFactory` and `WidgetFactory`
- **Plugin System**: Extensibility through `PluginInterface` and `PluginManager`

### Core Functionality

- PDF viewing with zoom, rotation, and navigation
- Advanced search with incremental search and error recovery
- Chrome-style thumbnails with virtual scrolling
- Multi-document support with recent files tracking
- Bookmarks and annotations
- Internationalization (English/Chinese)
- Light and dark themes

### Developer Experience

- Comprehensive test suite (100+ tests)
- Automatic clangd configuration
- Cross-compilation support
- Clang compiler support
- High-performance logging system
- Detailed documentation

## Quick Start

For the fastest way to get started, see our [Getting Started Guide](getting-started/).

For architecture and design information, see the [Architecture Guide](architecture.md).

For API documentation, see the [API Reference](api-reference.md).

## Contributing

This documentation is part of the SAST Readium project. For contribution guidelines and development information, please refer to the main project repository.

## Changelog

See [CHANGELOG.md](../CHANGELOG.md) for a detailed list of changes, new features, and migration notes.
