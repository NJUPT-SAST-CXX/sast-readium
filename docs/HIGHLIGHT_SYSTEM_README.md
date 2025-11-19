# SAST Readium PDF 高亮系统

## 概述

完整的 PDF 文本高亮功能实现，支持文本选择、高亮创建、编辑、持久化和导出。

## 快速开始

### 1. 初始化（在 MainWindow 中）

```cpp
#include "managers/HighlightManager.h"

// 初始化
auto& manager = HighlightManager::instance();
manager.setUndoStack(m_undoStack);
manager.setAutoSaveEnabled(true);
```

### 2. 打开文档时

```cpp
// 设置文档并自动加载高亮
manager.setDocument(document, filePath);
```

### 3. 创建高亮

```cpp
// 从文本选择创建高亮
if (m_textSelectionManager->hasSelection()) {
    TextSelection selection = m_textSelectionManager->getSelection();
    manager.addHighlightFromSelection(selection, pageNumber);
}
```

### 4. 渲染高亮

```cpp
// 在 paintEvent 中
auto highlights = manager.getHighlightsForPage(pageNumber);
HighlightRenderer::renderHighlights(painter, highlights, scaleFactor);
```

## 核心功能

### ✅ 已实现

- **数据模型** - 完整的高亮数据结构和 CRUD 操作
- **命令系统** - 11 种命令类型，支持撤销/重做
- **颜色系统** - 7 种预设颜色 + 自定义颜色
- **持久化** - JSON 格式自动保存和加载
- **导出** - Markdown、JSON、PlainText 格式
- **渲染** - 高亮渲染器，支持透明度和缩放
- **事件系统** - EventBus 集成
- **日志记录** - 完整的操作日志

### ⏳ 待集成

- MainWindow UI 集成
- PDF 渲染流程集成
- 右键菜单实现
- 高亮列表侧边栏
- TextSelectionManager 增强
- HighlightCache 实现

## 文件结构

```
app/
├── model/
│   ├── HighlightModel.h          ✅ 数据模型
│   └── HighlightModel.cpp        ✅ (957 行)
├── command/
│   ├── HighlightCommands.h       ✅ 命令系统
│   └── HighlightCommands.cpp     ✅ (717 行)
└── managers/
    ├── HighlightManager.h        ✅ 管理器接口
    └── HighlightManager.cpp      ✅ (630 行)

docs/
├── highlight-system.md                      ✅ 架构文档
├── highlight-usage-examples.md              ✅ 使用示例
├── highlight-implementation-summary.md      ✅ 实现总结
├── highlight-integration-guide.md           ✅ 集成指南
└── HIGHLIGHT_SYSTEM_README.md               ✅ 本文档

tests/
├── test_highlight_model.cpp                 ✅ 单元测试
└── test_highlight_commands.cpp              ✅ 命令测试
```

## API 参考

### HighlightManager

```cpp
// 单例访问
HighlightManager& manager = HighlightManager::instance();

// 文档管理
manager.setDocument(document, filePath);
manager.onDocumentClosed();

// 创建高亮
manager.addHighlight(highlight);
manager.addHighlightFromSelection(selection, pageNumber, color, opacity);
manager.addHighlightWithNote(selection, pageNumber, note, color);

// 编辑高亮
manager.removeHighlight(highlightId);
manager.editHighlightNote(highlightId, newNote);
manager.changeHighlightColor(highlightId, newColor);
manager.changeHighlightOpacity(highlightId, opacity);

// 查询
manager.getHighlight(highlightId);
manager.getAllHighlights();
manager.getHighlightsForPage(pageNumber);
manager.searchHighlights(query);
manager.findHighlightAtPoint(pageNumber, point);

// 持久化
manager.saveHighlights();
manager.loadHighlights();
manager.setAutoSaveEnabled(true);

// 导出
manager.exportToMarkdown();
manager.exportToPlainText();
manager.exportToJson();
manager.exportToFile(filePath, format);

// 统计
manager.getTotalHighlightCount();
manager.getStatistics();
manager.getColorDistribution();
```

### HighlightRenderer

```cpp
// 渲染高亮
HighlightRenderer::renderHighlight(painter, highlight, scaleFactor);
HighlightRenderer::renderHighlights(painter, highlights, scaleFactor);
HighlightRenderer::renderHighlightBorder(painter, highlight, scaleFactor);
HighlightRenderer::renderHighlightNote(painter, highlight, position, scaleFactor);
```

### HighlightColorManager

```cpp
// 颜色管理
QColor color = HighlightColorManager::getDefaultColor(HighlightColor::Yellow);
QString name = HighlightColorManager::getColorName(HighlightColor::Blue);
QList<HighlightColor> presets = HighlightColorManager::getAllPresets();
```

## 使用示例

### 完整的右键菜单

```cpp
void PDFViewer::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu;
    auto& manager = HighlightManager::instance();

    // 文本选择 -> 高亮选项
    if (hasSelection()) {
        QMenu* highlightMenu = menu.addMenu("Highlight");
        for (auto color : HighlightColorManager::getAllPresets()) {
            QString name = HighlightColorManager::getColorName(color);
            highlightMenu->addAction(name, [=]() {
                manager.addHighlightFromSelection(selection, page, color);
            });
        }
    }

    // 点击高亮 -> 编辑选项
    TextHighlight highlight = manager.findHighlightAtPoint(page, pos);
    if (!highlight.isEmpty()) {
        menu.addAction("Edit Note...", [=]() { /* ... */ });
        menu.addAction("Delete", [=]() {
            manager.removeHighlight(highlight.id);
        });
    }

    menu.exec(event->globalPos());
}
```

### 键盘快捷键

```cpp
// Ctrl+H - 快速高亮
QShortcut* highlightShortcut = new QShortcut(
    QKeySequence(Qt::CTRL | Qt::Key_H), this);
connect(highlightShortcut, &QShortcut::activated, [=]() {
    if (hasSelection()) {
        manager.addHighlightFromSelection(selection, page);
    }
});

// Ctrl+Shift+H - 高亮并添加备注
QShortcut* noteShortcut = new QShortcut(
    QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_H), this);
connect(noteShortcut, &QShortcut::activated, [=]() {
    QString note = QInputDialog::getText(this, "Add Note", "Enter note:");
    if (!note.isEmpty()) {
        manager.addHighlightWithNote(selection, page, note);
    }
});
```

### EventBus 集成

```cpp
// 订阅高亮事件
EventBus::instance().subscribe("highlight_added", this,
    [](const QVariant& data) {
        QVariantMap map = data.toMap();
        SLOG_INFO("New highlight: %1").arg(map["text"].toString());
    });

// 发布文档事件
EventBus::instance().publish("document_opened", filePath);
EventBus::instance().publish("document_closed", QVariant());
```

## 数据格式

### JSON 存储格式

```json
{
  "version": "1.0",
  "documentPath": "/path/to/document.pdf",
  "totalHighlights": 5,
  "highlights": [
    {
      "id": "uuid-string",
      "pageNumber": 0,
      "text": "highlighted text",
      "color": "#FFFF0066",
      "opacity": 0.4,
      "note": "my note",
      "author": "username",
      "createdTime": "2024-01-17T12:00:00",
      "isVisible": true,
      "colorPreset": 0,
      "rects": [
        {"x": 10.0, "y": 20.0, "width": 90.0, "height": 15.0}
      ]
    }
  ]
}
```

### Markdown 导出格式

```markdown
# Highlights for document.pdf

## Page 1

- **highlighted text** (Yellow)
  > my note

- **another highlight** (Green)
```

## 性能特性

- ✅ 命令合并减少内存使用
- ✅ 延迟加载设计
- ✅ 按页码索引查询
- ✅ 自动保存防抖动（30秒）
- ⏳ 只渲染可见页面（待实现）
- ⏳ 高亮缓存系统（待实现）

## 测试

### 运行测试

```bash
# 单元测试
ctest --test-dir build -R test_highlight

# 或使用测试脚本
./scripts/run_tests.ps1 -TestType Unit
```

### 测试覆盖

- ✅ HighlightModel - 所有 CRUD 操作
- ✅ HighlightCommands - 所有命令的撤销/重做
- ⏳ HighlightManager - 集成测试
- ⏳ UI 交互 - 端到端测试

## 故障排除

### 常见问题

**Q: 高亮没有显示？**
A: 确保在 paintEvent 中调用了 HighlightRenderer::renderHighlights()

**Q: 高亮没有保存？**
A: 检查 setAutoSaveEnabled(true) 是否调用，或手动调用 saveHighlights()

**Q: 撤销/重做不工作？**
A: 确保 setUndoStack() 已调用并传入有效的 QUndoStack

**Q: 文档切换时高亮混乱？**
A: 确保在文档关闭时调用 onDocumentClosed()

## 贡献指南

### 添加新功能

1. 在 HighlightModel 中添加数据操作
2. 在 HighlightCommands 中创建对应命令
3. 在 HighlightManager 中暴露接口
4. 添加单元测试
5. 更新文档

### 代码规范

- 遵循项目的 C++20 标准
- 使用 Qt 编码规范
- 添加完整的文档注释
- 使用 SLOG_* 宏记录日志
- 所有修改操作使用命令模式

## 文档索引

- **架构设计** - `docs/highlight-system.md`
- **使用示例** - `docs/highlight-usage-examples.md`
- **实现总结** - `docs/highlight-implementation-summary.md`
- **集成指南** - `docs/highlight-integration-guide.md`
- **完成报告** - `HIGHLIGHT_SYSTEM_COMPLETION_REPORT.md`
- **集成报告** - `HIGHLIGHT_INTEGRATION_COMPLETION_REPORT.md`

## 许可证

Copyright (c) 2024 SAST Team. All rights reserved.

---

**版本：** 1.0
**最后更新：** 2024-01-17
**状态：** 核心功能完成，待 UI 集成
**维护者：** SAST Team
