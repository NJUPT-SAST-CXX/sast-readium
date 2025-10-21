# Cursor Rules for SAST Readium

This directory contains Cursor Rules that help guide AI-assisted development for the SAST Readium project.

## Available Rules

### 1. project-architecture.mdc (Always Applied)

**Applies to:** All files

Provides comprehensive overview of the project architecture including:

- Core architecture patterns (MVC, Command, Service Locator, Event Bus, Factory, Plugin)
- Key subsystems (Search, PDF Rendering, Caching, Logging)
- Directory structure and organization
- Extension points for adding new features
- Communication guidelines between components

This rule is **always applied** to help the AI understand the overall project structure and design patterns.

### 2. cpp-coding-standards.mdc

**Applies to:** `*.cpp`, `*.h`, `*.hpp`

Defines C++ coding standards including:

- Naming conventions (PascalCase, camelCase, prefixes)
- Modern C++ practices (C++20, smart pointers, RAII)
- Const correctness and auto keyword usage
- Qt-specific practices (QObject, signals/slots)
- Thread safety guidelines
- Error handling patterns
- Documentation standards
- Performance considerations

### 3. qt-patterns.mdc

**Applies to:** `*.cpp`, `*.h`, `*.ui`, `*.qrc`

Qt framework-specific patterns and best practices:

- Object model and QObject hierarchy
- Signals and slots (modern syntax)
- Widget patterns and lifecycle
- Model/View framework usage
- Resource management (.qrc files)
- Threading with QThread and QtConcurrent
- Event system and custom events
- Internationalization (i18n)
- Performance best practices for Qt

### 4. cmake-conventions.mdc

**Applies to:** `CMakeLists.txt`, `*.cmake`

CMake build system conventions:

- Project structure and module organization
- Build presets for different platforms
- Dependency management (system packages vs vcpkg)
- Target configuration (modern CMake)
- Qt integration (qt_standard_project_setup)
- Compiler settings and optimization
- Testing integration
- Platform detection and cross-compilation
- IDE integration (clangd, compile_commands.json)

### 5. testing-conventions.mdc

**Applies to:** `tests/**/*.cpp`, `tests/**/*.h`

Testing framework and conventions:

- Qt Test framework with custom TestUtilities
- Test categories (unit, integration, performance)
- Standard test structure and patterns
- Assertions and custom test macros
- Testing asynchronous operations
- Mock objects and dependency injection
- Performance testing and benchmarking
- Best practices for test independence and coverage

### 6. logging-standards.mdc

**Applies when:** Manually referenced or when working with logging

Logging system standards:

- spdlog-based logging architecture
- Using category loggers vs global logger
- Log levels and when to use them (TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL)
- Format string syntax and Qt type conversion
- Performance considerations
- Configuration (JSON files)
- Qt integration (message handler)
- Best practices and security (no sensitive data)

## Rule Metadata

Each rule file contains YAML frontmatter that controls when it's applied:

```yaml
---
alwaysApply: true        # Applied to every request
globs: *.cpp,*.h         # Applied to matching file patterns
description: "..."       # Allows manual selection by AI
---
```

## Usage

### Automatic Application

- Rules with `alwaysApply: true` are always included
- Rules with `globs` are applied when working with matching files
- Rules with `description` can be fetched by the AI when relevant

### Manual Application

You can manually apply a rule by mentioning it in your request:

```
"Follow the testing-conventions rule to create a new test"
"Use the cmake-conventions for this CMakeLists.txt change"
```

## Maintenance

When updating rules:

1. Keep rules focused and relevant
2. Reference actual project files using `[filename](mdc:path/to/file)`
3. Update examples to match current project practices
4. Test that rules provide helpful guidance without being overly restrictive

## Related Documentation

These rules complement the project documentation in the `docs/` directory:

- [docs/architecture.md](../../docs/architecture.md) - Detailed architecture documentation
- [docs/logging-system.md](../../docs/logging-system.md) - Logging system guide
- [tests/README.md](../../tests/README.md) - Testing framework documentation
- [cmake/README.md](../../cmake/README.md) - CMake modules documentation

## Contributing

When adding new rules:

1. Use descriptive filenames ending in `.mdc`
2. Include appropriate metadata in frontmatter
3. Reference actual project files for examples
4. Keep rules concise and actionable
5. Update this README with the new rule
