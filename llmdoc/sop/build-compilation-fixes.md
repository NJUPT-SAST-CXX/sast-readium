# Build System Compilation Fixes

## 1. Purpose

This SOP documents critical build system fixes applied to resolve 5 distinct compilation errors that prevented the project from compiling. These fixes address enum completeness, type casting, field naming consistency, macro conflicts with Qt internals, and missing includes. This document serves as a reference for maintaining build integrity and preventing regression.

## 2. Step-by-Step Guide

### Fix 1: ActionMap Enum Extensions (app/controller/tool.hpp)

**Problem**: KeyboardShortcutManager.cpp referenced actions `quit` and `showHelp` that were missing from ActionMap enum definition.

**Solution**:
1. Open `/app/controller/tool.hpp`
2. Locate the ActionMap enum (lines 3-67)
3. Add missing enum values at end of enum before closing brace:
   - `quit` (application termination)
   - `showHelp` (display help dialog)
4. Rebuild project

**Files Modified**: `/app/controller/tool.hpp` (2 enum values added)

### Fix 2: KeyboardShortcutManager QApplication Type Cast (app/managers/KeyboardShortcutManager.cpp)

**Problem**: Invalid conversion from `QCoreApplication*` to `const QApplication*` when connecting to `focusChanged` signal.

**Solution**:
1. Open `/app/managers/KeyboardShortcutManager.cpp` (around line 35)
2. Replace direct `QApplication::instance()` with type-safe cast:
   ```cpp
   // OLD: QApplication::instance()
   // NEW: qobject_cast<QApplication*>(QApplication::instance())
   if (auto* app = qobject_cast<QApplication*>(QApplication::instance())) {
       connect(app, &QApplication::focusChanged,
               this, &KeyboardShortcutManager::onFocusChanged);
   }
   ```
3. Explanation: `focusChanged` signal exists only on QApplication, not base QCoreApplication
4. Rebuild project

**Files Modified**: `/app/managers/KeyboardShortcutManager.cpp` (1 cast added, conditional wrapper)

### Fix 3: AnnotationModel Field Name Consistency (app/model/AnnotationModel.cpp)

**Problem**: Field names in PDFAnnotation struct definition didn't match usage in implementation. Three distinct field mismatches caused runtime errors.

**Solution**:
1. Open `/app/model/AnnotationModel.cpp` (implementation file)
2. Cross-reference with `/app/model/AnnotationModel.h` (header file)
3. Header defines: `boundingRect`, `createdTime`, `modifiedTime`
4. In AnnotationModel.cpp, replace all incorrect usages:
   - `boundary` → `boundingRect` (3 occurrences in toJson/fromJson)
   - `creationDate` → `createdTime` (2 occurrences)
   - `modificationDate` → `modifiedTime` (1 occurrence)
5. Example fix:
   ```cpp
   // OLD: annotation.boundingRect = rectObj["boundary"].toObject();
   // NEW: annotation.boundingRect = rectObj["boundingRect"].toObject();
   ```
6. Rebuild project

**Files Modified**: `/app/model/AnnotationModel.cpp` (6 field name corrections)

### Fix 4: LoggingMacros Qt Compatibility (app/logging/LoggingMacros.h)

**Problem**: Problematic `#undef` and `#define` of Qt's `qDebug`, `qWarning`, `qCritical` macros broke Qt's internal template code in `qrangemodel_impl.h`.

**Solution**:
1. Open `/app/logging/LoggingMacros.h`
2. **Remove** all `#undef qDebug`, `#undef qWarning`, `#undef qCritical` lines (if present)
3. **Remove** any redefinition macros that override Qt's logging
4. Add new spd_q* prefixed alternatives instead (lines 319-326):
   ```cpp
   #define spd_qDebug() spdlogDebug()
   #define spd_qInfo() spdlogInfo()
   #define spd_qWarning() spdlogWarning()
   #define spd_qCritical() spdlogCritical()
   ```
5. Add documentation comment (lines 314-317):
   ```cpp
   // NOTE: We do NOT undef or redefine Qt's qDebug/qWarning/qCritical macros
   // because it breaks Qt's internal usage in template headers like qrangemodel_impl.h
   // The original Qt logging macros remain available for backward compatibility.
   // For spdlog integration, use the spd_q* alternatives or LOG_* macros.
   ```
6. Rebuild project

**Files Modified**: `/app/logging/LoggingMacros.h` (removed harmful macros, added spd_q* alternatives)

**Important**: Qt's original macros must be preserved for backward compatibility with Qt template headers.

### Fix 5: Test Missing Include (tests/factory/test_command_prototype_registry.cpp)

**Problem**: Missing `#include <QElapsedTimer>` caused compilation error in performance measurement tests.

**Solution**:
1. Open `/tests/factory/test_command_prototype_registry.cpp`
2. Add missing include at top of file with other Qt headers:
   ```cpp
   #include <QElapsedTimer>
   ```
3. Rebuild project

**Files Modified**: `/tests/factory/test_command_prototype_registry.cpp` (1 include added)

## 3. Relevant Code Modules

- `/app/controller/tool.hpp` - ActionMap enum definition
- `/app/managers/KeyboardShortcutManager.h` - Header for KeyboardShortcutManager
- `/app/managers/KeyboardShortcutManager.cpp` - Implementation with focus change handling
- `/app/model/AnnotationModel.h` - Header with PDFAnnotation struct definition
- `/app/model/AnnotationModel.cpp` - Implementation with serialization logic
- `/app/logging/LoggingMacros.h` - Comprehensive logging macro definitions
- `/tests/factory/test_command_prototype_registry.cpp` - Performance measurement tests

## 4. Attention

- **Critical**: Do NOT redefine Qt's qDebug/qWarning/qCritical macros as they break Qt internal templates
- Always validate field names match exactly between struct definition (.h) and implementation (.cpp)
- Use `qobject_cast<QApplication*>()` when accessing QApplication-specific features from base QCoreApplication
- When adding enum values, ensure no duplicate enumerations exist
- Test all compilation target types: main application (app.exe) and all 73 test executables
- Build validation: 74 total executables must compile without errors
