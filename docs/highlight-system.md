# PDF 高亮系统设计文档

## 概述

本文档描述了 SAST Readium PDF 阅读器中完整的文本高亮功能实现。该系统支持文本选择、高亮创建、编辑、持久化和导出等完整功能。

## 系统架构

### 核心组件

```
┌─────────────────────────────────────────────────────────┐
│                   HighlightManager                       │
│              (统一管理和协调所有高亮操作)                  │
└──────────────┬──────────────────────────────────────────┘
               │
       ┌───────┴───────┐
       │               │
┌──────▼──────┐  ┌────▼─────────┐
│HighlightModel│  │HighlightCache│
│(数据模型)     │  │(持久化存储)   │
└──────┬──────┘  └──────────────┘
       │
┌──────▼──────────────────────────┐
│   HighlightCommands              │
│   (命令模式实现撤销/重做)         │
│   - AddHighlightCommand          │
│   - RemoveHighlightCommand       │
│   - EditHighlightNoteCommand     │
│   - ChangeHighlightColorCommand  │
│   - ...                          │
└──────────────────────────────────┘
```

### 数据流

```
用户交互 → TextSelectionManager → HighlightManager → HighlightCommand
                                                            ↓
                                                     HighlightModel
                                                            ↓
                                                     EventBus (发布事件)
                                                            ↓
                                                     UI 更新 / 渲染
```

## 主要类说明

### 1. HighlightModel (`app/model/HighlightModel.h`)

**职责：** 管理所有高亮数据，提供 CRUD 操作接口

**核心数据结构：**

```cpp
struct TextHighlight {
    QString id;                  // 唯一标识符
    int pageNumber;              // 页码（0-based）
    QList<QRectF> rects;         // 高亮区域矩形列表
    QString text;                // 高亮文本内容
    QColor color;                // 高亮颜色
    double opacity;              // 透明度 (0.0-1.0)
    QString note;                // 备注/注释
    QString author;              // 作者
    QDateTime createdTime;       // 创建时间
    QDateTime modifiedTime;      // 修改时间
    bool isVisible;              // 可见性
    HighlightColor colorPreset;  // 颜色预设
    // ... 其他字段
};
```

**主要方法：**

- `addHighlight()` - 添加高亮
- `removeHighlight()` - 删除高亮
- `updateHighlight()` - 更新高亮
- `getHighlightsForPage()` - 获取指定页面的高亮
- `searchHighlights()` - 搜索高亮
- `exportToMarkdown/JSON/PlainText()` - 导出高亮

### 2. HighlightCommands (`app/command/HighlightCommands.h`)

**职责：** 实现所有高亮操作的命令模式，支持撤销/重做

**命令类型：**

- `AddHighlightCommand` - 添加高亮
- `RemoveHighlightCommand` - 删除高亮
- `EditHighlightNoteCommand` - 编辑备注
- `ChangeHighlightColorCommand` - 修改颜色
- `ChangeHighlightOpacityCommand` - 修改透明度
- `ToggleHighlightVisibilityCommand` - 切换可见性
- `ClearAllHighlightsCommand` - 清除所有高亮
- `BatchAddHighlightsCommand` - 批量添加
- `BatchRemoveHighlightsCommand` - 批量删除
- `UpdateHighlightCommand` - 综合更新

**使用示例：**

```cpp
// 创建添加高亮命令
auto* cmd = HighlightCommandFactory::createAddCommand(model, highlight);
undoStack->push(cmd);  // 执行命令并支持撤销

// 撤销
undoStack->undo();

// 重做
undoStack->redo();
```

### 3. HighlightManager (`app/managers/HighlightManager.h`)

**职责：** 统一管理所有高亮功能，协调各组件交互

**核心功能：**

- 高亮创建和编辑
- 持久化管理（自动保存）
- 导出功能
- 统计信息
- 与 TextSelectionManager 集成

**使用示例：**

```cpp
// 获取单例
auto& manager = HighlightManager::instance();

// 设置文档
manager.setDocument(document, documentPath);

// 从选择创建高亮
manager.addHighlightFromSelection(selection, pageNumber,
                                   HighlightColor::Yellow, 0.4);

// 添加带备注的高亮
manager.addHighlightWithNote(selection, pageNumber,
                              "重要内容", HighlightColor::Red);

// 导出高亮
QString markdown = manager.exportToMarkdown();
manager.exportToFile("highlights.json", "json");
```

### 4. TextSelectionManager (`app/interaction/TextSelectionManager.h`)

**职责：** 管理文本选择交互

**增强功能：**

- 鼠标拖拽选择
- 双击选择单词
- 三击选择整行
- 键盘快捷键选择（Shift + 方向键）
- 跨行、跨段落选择

## 颜色系统

### 预设颜色

```cpp
enum class HighlightColor {
    Yellow,    // 黄色（默认）
    Green,     // 绿色
    Blue,      // 蓝色
    Pink,      // 粉色
    Orange,    // 橙色
    Purple,    // 紫色
    Red,       // 红色
    Custom     // 自定义
};
```

### 颜色管理

```cpp
// 获取预设颜色
QColor color = HighlightColorManager::getDefaultColor(HighlightColor::Yellow);

// 获取所有预设
QList<HighlightColor> presets = HighlightColorManager::getAllPresets();

// 颜色名称
QString name = HighlightColorManager::getColorName(HighlightColor::Blue);
```

## 持久化

### 存储格式

高亮数据以 JSON 格式存储，文件命名规则：

```
<document_name>_highlights.json
```

### JSON 结构

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
      "createdTime": "2024-01-01T12:00:00",
      "modifiedTime": "2024-01-01T12:00:00",
      "isVisible": true,
      "colorPreset": 0,
      "startCharIndex": 100,
      "endCharIndex": 150,
      "startPoint": {"x": 10.0, "y": 20.0},
      "endPoint": {"x": 100.0, "y": 20.0},
      "rects": [
        {"x": 10.0, "y": 20.0, "width": 90.0, "height": 15.0}
      ]
    }
  ]
}
```

### 自动保存

```cpp
// 启用自动保存
manager.setAutoSaveEnabled(true);

// 手动保存
manager.saveHighlights();

// 加载高亮
manager.loadHighlights();
```

## 导出功能

### 支持的格式

1. **Markdown**

   ```markdown
   # Highlights for document.pdf

   ## Page 1

   - **highlighted text** (Yellow)
     > my note
   ```

2. **Plain Text**

   ```
   Highlights for: document.pdf
   Total: 5 highlights

   [Page 1] highlighted text
   Note: my note
   ```

3. **JSON**

   ```json
   [
     {
       "id": "...",
       "text": "...",
       ...
     }
   ]
   ```

### 导出示例

```cpp
// 导出为 Markdown
QString md = manager.exportToMarkdown();

// 导出为文件
manager.exportToFile("highlights.md", "markdown");
manager.exportToFile("highlights.txt", "text");
manager.exportToFile("highlights.json", "json");
```

## 事件系统

### 发布的事件

```cpp
// 高亮添加
EventBus::instance().publish("highlight_added", highlightData);

// 高亮删除
EventBus::instance().publish("highlight_removed", highlightId);

// 高亮更新
EventBus::instance().publish("highlight_updated", highlightData);

// 高亮可见性变化
EventBus::instance().publish("highlight_visibility_changed", data);
```

### 订阅事件

```cpp
EventBus::instance().subscribe("highlight_added", this,
    [](const QVariant& data) {
        // 处理高亮添加事件
        auto highlight = data.value<TextHighlight>();
        // 更新 UI...
    });
```

## 渲染集成

### 在 RenderModel 中渲染高亮

```cpp
void RenderModel::renderPage(int pageNumber, QPainter& painter) {
    // 渲染 PDF 页面
    renderPDFPage(pageNumber, painter);

    // 渲染高亮
    auto highlights = HighlightManager::instance()
                        .getHighlightsForPage(pageNumber);
    HighlightRenderer::renderHighlights(painter, highlights, scaleFactor);
}
```

### 高亮渲染器

```cpp
// 渲染单个高亮
HighlightRenderer::renderHighlight(painter, highlight, scaleFactor);

// 渲染高亮边框
HighlightRenderer::renderHighlightBorder(painter, highlight, scaleFactor);

// 渲染高亮备注
HighlightRenderer::renderHighlightNote(painter, highlight, position, scaleFactor);
```

## 用户交互

### 右键菜单

```cpp
QMenu* contextMenu = new QMenu();

if (selectionManager->hasSelection()) {
    // 添加高亮选项
    QMenu* highlightMenu = contextMenu->addMenu("Highlight");
    for (auto color : HighlightColorManager::getAllPresets()) {
        QString colorName = HighlightColorManager::getColorName(color);
        highlightMenu->addAction(colorName, [=]() {
            manager.addHighlightFromSelection(selection, pageNumber, color);
        });
    }

    // 添加备注
    contextMenu->addAction("Add Note", [=]() {
        QString note = QInputDialog::getText(...);
        manager.addHighlightWithNote(selection, pageNumber, note);
    });

    // 复制文本
    contextMenu->addAction("Copy", [=]() {
        selectionManager->copySelectionToClipboard();
    });
}

// 如果点击在高亮上
TextHighlight highlight = manager.findHighlightAtPoint(pageNumber, point);
if (!highlight.isEmpty()) {
    contextMenu->addAction("Edit Note", [=]() {
        // 编辑备注...
    });
    contextMenu->addAction("Change Color", [=]() {
        // 修改颜色...
    });
    contextMenu->addAction("Delete Highlight", [=]() {
        manager.removeHighlight(highlight.id);
    });
}

contextMenu->exec(globalPos);
```

## 测试

### 单元测试

```cpp
// tests/test_highlight_model.cpp
TEST(HighlightModelTest, AddHighlight) {
    HighlightModel model;
    TextHighlight highlight;
    highlight.text = "test";
    highlight.pageNumber = 0;

    EXPECT_TRUE(model.addHighlight(highlight));
    EXPECT_EQ(model.getTotalHighlightCount(), 1);
}

// tests/test_highlight_commands.cpp
TEST(HighlightCommandsTest, UndoRedo) {
    HighlightModel model;
    QUndoStack stack;

    auto* cmd = new AddHighlightCommand(&model, highlight);
    stack.push(cmd);

    EXPECT_EQ(model.getTotalHighlightCount(), 1);

    stack.undo();
    EXPECT_EQ(model.getTotalHighlightCount(), 0);

    stack.redo();
    EXPECT_EQ(model.getTotalHighlightCount(), 1);
}
```

## 性能优化

1. **延迟加载** - 仅加载当前页面的高亮
2. **缓存** - 使用 HighlightCache 缓存高亮数据
3. **批量操作** - 使用批量命令减少信号发送
4. **索引** - 按页码索引高亮，加速查询

## 未来扩展

- [ ] 高亮合并功能
- [ ] 高亮分割功能
- [ ] 跨页面高亮支持
- [ ] 高亮同步（云端）
- [ ] 高亮分享功能
- [ ] 高亮模板
- [ ] AI 辅助高亮建议
