# SAST Readium - Feature Testing Checklist

This checklist should be used to verify that all application features work correctly after packaging and installation.

## Test Environment

- [ ] **Clean Windows Installation**: Test on a system without Qt6 or development tools installed
- [ ] **Windows Version**: Test on target Windows versions (Windows 10/11)
- [ ] **Architecture**: Verify correct architecture (x64)
- [ ] **User Permissions**: Test with standard user account (non-administrator)

## Installation Testing

### MSI Installer (MSVC Build)

- [ ] Installer launches without errors
- [ ] License agreement displays correctly
- [ ] Installation directory can be customized
- [ ] Installation completes successfully
- [ ] Start Menu shortcuts are created
- [ ] Desktop shortcut is created (if selected)
- [ ] Uninstaller is registered in Add/Remove Programs
- [ ] Application metadata is correct (version, publisher, icon)

### NSIS Installer (MSYS2 Build)

- [ ] Installer launches without errors
- [ ] Installation directory can be customized
- [ ] Installation completes successfully
- [ ] Start Menu shortcuts are created
- [ ] Desktop shortcut is created
- [ ] Uninstaller is created
- [ ] Application can be uninstalled cleanly

### Portable Package (ZIP)

- [ ] Archive extracts without errors
- [ ] Application runs from extracted directory
- [ ] No installation required
- [ ] All features work from portable location

## Application Launch

- [ ] **Application starts successfully** from Start Menu shortcut
- [ ] **Application starts successfully** from Desktop shortcut
- [ ] **Application starts successfully** from executable directly
- [ ] **No missing DLL errors** on launch
- [ ] **Main window appears** within 5 seconds
- [ ] **Application icon** displays correctly in taskbar
- [ ] **Window title** is correct ("SAST Readium")

## Core Functionality

### PDF Rendering

- [ ] **Open PDF file** via File → Open menu
- [ ] **Open PDF file** via drag-and-drop
- [ ] **Open PDF file** via command-line argument
- [ ] **PDF content renders correctly** (text, images, vector graphics)
- [ ] **Page navigation works** (next/previous buttons)
- [ ] **Page navigation works** (page number input)
- [ ] **Zoom in/out works** correctly
- [ ] **Fit to width** zoom mode works
- [ ] **Fit to page** zoom mode works
- [ ] **Scrolling** is smooth and responsive
- [ ] **Multi-page PDFs** display correctly
- [ ] **Large PDFs** (100+ pages) load without crashes

### Search Functionality

- [ ] **Search dialog opens** (Ctrl+F or menu)
- [ ] **Text search works** for simple queries
- [ ] **Case-sensitive search** works correctly
- [ ] **Whole word search** works correctly
- [ ] **Search highlights** appear on page
- [ ] **Next/Previous result** navigation works
- [ ] **Search results count** displays correctly
- [ ] **Incremental search** updates in real-time
- [ ] **Search across multiple pages** works
- [ ] **Search performance** is acceptable (< 2s for typical PDF)
- [ ] **Background search** doesn't freeze UI

### Thumbnail View

- [ ] **Thumbnail panel** can be opened/closed
- [ ] **Thumbnails generate** for all pages
- [ ] **Thumbnail quality** is acceptable
- [ ] **Clicking thumbnail** navigates to page
- [ ] **Current page** is highlighted in thumbnails
- [ ] **Thumbnail scrolling** is smooth
- [ ] **Large PDFs** generate thumbnails without crashes

### Document Navigation

- [ ] **Table of Contents** (if PDF has bookmarks) displays
- [ ] **Bookmark navigation** works correctly
- [ ] **Go to page** dialog works
- [ ] **First/Last page** buttons work
- [ ] **Page history** (back/forward) works

## User Interface

### Menus and Toolbars

- [ ] **File menu** displays correctly
- [ ] **Edit menu** displays correctly
- [ ] **View menu** displays correctly
- [ ] **Help menu** displays correctly
- [ ] **Toolbar buttons** have correct icons
- [ ] **Toolbar tooltips** display on hover
- [ ] **Menu shortcuts** (Ctrl+O, Ctrl+F, etc.) work

### Window Management

- [ ] **Window can be resized** smoothly
- [ ] **Window can be maximized**
- [ ] **Window can be minimized**
- [ ] **Window can be restored** from minimized state
- [ ] **Window position** is remembered between sessions
- [ ] **Window size** is remembered between sessions

### Themes and Styling

- [ ] **Light theme** applies correctly
- [ ] **Dark theme** applies correctly
- [ ] **Theme switching** works without restart
- [ ] **Custom styles** load from styles directory
- [ ] **UI elements** are readable in both themes
- [ ] **Icons** are visible in both themes

## Internationalization (i18n)

- [ ] **Language selection** menu is available
- [ ] **English translation** works correctly
- [ ] **Chinese translation** works correctly
- [ ] **Language switching** works without restart
- [ ] **All UI elements** are translated
- [ ] **Translation files** (.qm) are present in translations directory

## Advanced Features

### Caching System

- [ ] **Page cache** improves performance on revisit
- [ ] **Thumbnail cache** persists between sessions
- [ ] **Search results cache** works correctly
- [ ] **Cache directory** is created in user profile
- [ ] **Cache doesn't grow unbounded** (size limits work)

### Recent Files

- [ ] **Recent files list** populates when opening PDFs
- [ ] **Recent files menu** displays correctly
- [ ] **Clicking recent file** opens the document
- [ ] **Recent files persist** between sessions
- [ ] **Recent files limit** works (max 10 or configured limit)
- [ ] **Non-existent files** are handled gracefully

### System Tray Integration

- [ ] **System tray icon** appears when minimized (if enabled)
- [ ] **System tray menu** displays correctly
- [ ] **Restore from tray** works
- [ ] **Exit from tray** works
- [ ] **Tray icon tooltip** shows application name

### Printing

- [ ] **Print dialog** opens (File → Print)
- [ ] **Print preview** displays correctly
- [ ] **Page range selection** works
- [ ] **Print to PDF** works (if available)
- [ ] **Print to physical printer** works (if available)

### Plugin System

- [ ] **Plugin directory** exists
- [ ] **Plugins can be loaded** (if any are included)
- [ ] **Plugin manager** displays loaded plugins
- [ ] **Plugin errors** are handled gracefully

## Performance Testing

- [ ] **Application startup time** < 3 seconds
- [ ] **PDF load time** < 2 seconds for typical document
- [ ] **Memory usage** is reasonable (< 500 MB for typical use)
- [ ] **CPU usage** is low when idle (< 5%)
- [ ] **No memory leaks** during extended use (1 hour test)
- [ ] **Smooth scrolling** at 60 FPS
- [ ] **Responsive UI** during background operations

## Error Handling

- [ ] **Invalid PDF files** show error message (don't crash)
- [ ] **Corrupted PDF files** are handled gracefully
- [ ] **Missing files** show appropriate error
- [ ] **Insufficient permissions** are handled correctly
- [ ] **Out of memory** conditions don't crash application
- [ ] **Network errors** (if applicable) are handled
- [ ] **Error messages** are user-friendly and informative

## Logging and Debugging

- [ ] **Log files** are created in appropriate location
- [ ] **Log level** can be configured
- [ ] **Logs contain useful information** for debugging
- [ ] **Logs don't contain sensitive information**
- [ ] **Log rotation** works (if implemented)

## Uninstallation Testing

### MSI Uninstaller

- [ ] **Uninstaller runs** from Add/Remove Programs
- [ ] **All files are removed** from installation directory
- [ ] **Shortcuts are removed** from Start Menu
- [ ] **Desktop shortcut is removed**
- [ ] **Registry entries are cleaned up**
- [ ] **User data is preserved** (or user is prompted)

### NSIS Uninstaller

- [ ] **Uninstaller runs** successfully
- [ ] **All files are removed** from installation directory
- [ ] **Shortcuts are removed**
- [ ] **User data handling** is correct

## Regression Testing

After any packaging changes, verify:

- [ ] **All previous features** still work
- [ ] **No new crashes** introduced
- [ ] **Performance** hasn't degraded
- [ ] **Package size** hasn't increased significantly

## Notes

### Test Results

- **Date**: _______________
- **Tester**: _______________
- **Package Version**: _______________
- **Build Type**: [ ] MSVC MSI  [ ] MSYS2 NSIS  [ ] Portable ZIP
- **Windows Version**: _______________

### Issues Found

(List any issues discovered during testing)

1.
2.
3.

### Overall Assessment

- [ ] **PASS**: All critical features work correctly
- [ ] **PASS WITH WARNINGS**: Minor issues found but package is usable
- [ ] **FAIL**: Critical issues prevent release

### Recommendations

(Any suggestions for improvements)

---

**Note**: This checklist should be completed for each release candidate before final distribution.
