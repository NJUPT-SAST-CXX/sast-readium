# SAST Readium Stylesheet Organization

This directory contains all Qt StyleSheet (QSS) files for the SAST Readium application.

## 📁 File Structure

### Core Theme Files

- **`light.qss`** (1,111 lines, ~28 KB)
  - Light theme stylesheet for the entire application
  - Loaded by `StyleManager` when light theme is active
  - Contains styles for all main UI components

- **`dark.qss`** (1,110 lines, ~28 KB)
  - Dark theme stylesheet for the entire application
  - Loaded by `StyleManager` when dark theme is active
  - Mirrors the structure of `light.qss` with dark color palette

### Supplementary Files

- **`common.qss`** (30 lines, ~662 bytes)
  - Theme-independent shared styles
  - Applied to both light and dark themes
  - Contains focus states, menu interactions, and utility classes

- **`thumbnails.qss`** (298 lines, ~6.6 KB)
  - Chrome-style thumbnail grid styling
  - Theme-independent (works with both light and dark)
  - Specialized styles for PDF thumbnail view

- **`metadata_dialog.qss`** (447 lines, ~9.7 KB)
  - **Optional** stylesheet for DocumentMetadataDialog
  - Contains both light and dark theme variants using `[theme="dark"]` selectors
  - Currently **NOT LOADED** by default (dialog uses global stylesheet)
  - Available for future use if specialized metadata dialog styling is needed

## 🔄 Loading Order

The `StyleManager` loads QSS files in the following order:

1. **Theme-specific stylesheet** (`light.qss` OR `dark.qss`)
2. **Common stylesheet** (`common.qss`)
3. **Thumbnails stylesheet** (`thumbnails.qss`)

**Final stylesheet** = Theme + Common + Thumbnails

This order ensures:

- Theme-specific styles are applied first
- Common styles can override theme defaults
- Thumbnail styles are applied last for specificity

## 📝 File Headers

Each QSS file contains a header comment block with:

- **Purpose**: What the file styles
- **Theme**: Light/Dark/Independent
- **Components**: List of styled widgets
- **Loading**: How and when it's loaded
- **Last Updated**: Date of last major changes

## 🎨 Styling Guidelines

### Color Palette

**Light Theme:**

- Primary: `#0078d4` (Microsoft Blue)
- Background: `#f6f7f9` (Light Gray)
- Text: `#333333` (Dark Gray)
- Border: `#c0c0c0` (Medium Gray)

**Dark Theme:**

- Primary: `#60cdff` (Light Blue)
- Background: `#1f1f22` (Dark Gray)
- Text: `#e5e5e7` (Light Gray)
- Border: `#5a5a62` (Medium Gray)

### Component Organization

Both `light.qss` and `dark.qss` are organized into sections:

1. **Main Window & Layout** - QMainWindow, QWidget, QStackedWidget
2. **Toolbars** - QToolBar, QToolButton
3. **Menus** - QMenuBar, QMenu, QAction
4. **Buttons** - QPushButton, QToolButton states
5. **Input Widgets** - QLineEdit, QTextEdit, QPlainTextEdit, QComboBox
6. **Lists & Trees** - QListView, QTreeView, QTableView
7. **Tabs** - QTabWidget, QTabBar
8. **Scrollbars** - QScrollBar (vertical/horizontal)
9. **Dialogs** - QDialog, QMessageBox
10. **Progress & Status** - QProgressBar, QStatusBar
11. **Specialized Widgets** - Custom application widgets

### Removed Properties

The following CSS3 properties are **NOT supported** by Qt StyleSheets and have been removed:

- ❌ `transition` - CSS3 transitions
- ❌ `box-shadow` - CSS3 shadows
- ❌ `transform` - CSS3 transforms

Use Qt's `QPropertyAnimation` for animations instead.

## 🔧 Maintenance

### Adding New Styles

1. Determine if the style is theme-specific or common
2. Add to appropriate file (`light.qss`/`dark.qss` or `common.qss`)
3. If theme-specific, add to **BOTH** light and dark files
4. Test with both themes to ensure consistency

### Modifying Existing Styles

1. Search for the selector in the appropriate file
2. Make changes to both light and dark versions if theme-specific
3. Rebuild the application to update embedded resources
4. Test visual changes in both themes

### Extracting Common Styles

If you find duplicate rules between `light.qss` and `dark.qss`:

1. Extract the common parts to `common.qss`
2. Keep only color/theme-specific properties in theme files
3. Document the extraction in commit message

## 🚀 Build Integration

QSS files are embedded into the application via Qt Resource System:

- **Resource file**: `app/app.qrc`
- **Prefix**: `:/styles/`
- **Access**: `QFile(":/styles/light.qss")`

Changes to QSS files require rebuilding the application:

```bash
cmake --build build/Debug-MSYS2 --target app -j2
```

## 📊 Statistics

| File | Lines | Size | Theme | Status |
|------|-------|------|-------|--------|
| light.qss | 1,111 | 27.8 KB | Light | ✅ Active |
| dark.qss | 1,110 | 28.0 KB | Dark | ✅ Active |
| common.qss | 30 | 662 B | Both | ✅ Active |
| thumbnails.qss | 298 | 6.6 KB | Both | ✅ Active |
| metadata_dialog.qss | 447 | 9.7 KB | Both | ⚠️ Optional |

**Total**: 2,996 lines, ~73 KB of stylesheets

## 🔍 Related Files

- **StyleManager**: `app/managers/StyleManager.cpp` - Loads and applies QSS
- **Resource File**: `app/app.qrc` - Embeds QSS into application
- **Theme Enum**: `app/managers/StyleManager.h` - Theme::Light, Theme::Dark

## 📚 References

- [Qt Style Sheets Documentation](https://doc.qt.io/qt-6/stylesheet.html)
- [Qt Style Sheets Reference](https://doc.qt.io/qt-6/stylesheet-reference.html)
- [Qt Style Sheets Examples](https://doc.qt.io/qt-6/stylesheet-examples.html)

---

**Last Updated**: 2025-10-29
**Maintained By**: SAST Readium Development Team
