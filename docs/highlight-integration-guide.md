# 高亮系统集成指南

本文档提供将高亮系统集成到 SAST Readium 项目的详细步骤和代码示例。

## 1. MainWindow 集成

### 1.1 在 MainWindow.h 中添加成员变量

```cpp
// MainWindow.h
#include "managers/HighlightManager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

private:
    // ... 其他成员变量 ...

    // 高亮系统相关
    QUndoStack* m_undoStack;

    // 菜单和工具栏
    QMenu* m_highlightMenu;
    QToolBar* m_highlightToolBar;

private slots:
    // 高亮相关槽函数
    void onHighlightAdded(const TextHighlight& highlight);
    void onHighlightRemoved(const QString& highlightId);
    void onHighlightUpdated(const TextHighlight& highlight);
    void onHighlightText();
    void onHighlightWithNote();
    void onExportHighlights();
    void onShowHighlightList();
};
```

### 1.2 在 MainWindow.cpp 中初始化

```cpp
// MainWindow.cpp
void MainWindow::initializeApplication() {
    // ... 其他初始化代码 ...

    // 创建撤销栈
    m_undoStack = new QUndoStack(this);

    // 初始化高亮管理器
    auto& highlightManager = HighlightManager::instance();
    highlightManager.setUndoStack(m_undoStack);
    highlightManager.setAutoSaveEnabled(true);
    highlightManager.setDefaultColor(HighlightColor::Yellow);
    highlightManager.setDefaultOpacity(0.4);

    // 连接信号
    connect(&highlightManager, &HighlightManager::highlightAdded,
            this, &MainWindow::onHighlightAdded);
    connect(&highlightManager, &HighlightManager::highlightRemoved,
            this, &MainWindow::onHighlightRemoved);
    connect(&highlightManager, &HighlightManager::highlightUpdated,
            this, &MainWindow::onHighlightUpdated);

    // 订阅 EventBus 事件
    EventBus::instance().subscribe("document_opened", this,
        [this](const QVariant& data) {
            QString filePath = data.toString();
            SLOG_INFO("Document opened: %1").arg(filePath);
        });

    EventBus::instance().subscribe("document_closed", this,
        [&highlightManager](const QVariant&) {
            highlightManager.onDocumentClosed();
        });

    // 创建菜单和工具栏
    createHighlightMenu();
    createHighlightToolBar();

    SLOG_INFO("Highlight system initialized");
}

void MainWindow::createHighlightMenu() {
    // 创建高亮菜单
    m_highlightMenu = menuBar()->addMenu(tr("&Highlight"));

    // 快速高亮（默认颜色）
    QAction* quickHighlightAction = m_highlightMenu->addAction(
        QIcon(":/icons/highlight"), tr("Quick Highlight"));
    quickHighlightAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_H));
    connect(quickHighlightAction, &QAction::triggered,
            this, &MainWindow::onHighlightText);

    // 高亮并添加备注
    QAction* highlightWithNoteAction = m_highlightMenu->addAction(
        QIcon(":/icons/highlight_note"), tr("Highlight with Note..."));
    highlightWithNoteAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_H));
    connect(highlightWithNoteAction, &QAction::triggered,
            this, &MainWindow::onHighlightWithNote);

    m_highlightMenu->addSeparator();

    // 颜色子菜单
    QMenu* colorMenu = m_highlightMenu->addMenu(tr("Highlight Color"));
    for (auto color : HighlightColorManager::getAllPresets()) {
        QString colorName = HighlightColorManager::getColorName(color);
        QAction* colorAction = colorMenu->addAction(colorName);

        // 设置颜色图标
        QPixmap pixmap(16, 16);
        pixmap.fill(HighlightColorManager::getDefaultColor(color));
        colorAction->setIcon(QIcon(pixmap));

        connect(colorAction, &QAction::triggered, [this, color]() {
            highlightSelectedText(color);
        });
    }

    m_highlightMenu->addSeparator();

    // 显示高亮列表
    QAction* showListAction = m_highlightMenu->addAction(
        QIcon(":/icons/list"), tr("Show Highlight List"));
    showListAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_L));
    connect(showListAction, &QAction::triggered,
            this, &MainWindow::onShowHighlightList);

    // 导出高亮
    QAction* exportAction = m_highlightMenu->addAction(
        QIcon(":/icons/export"), tr("Export Highlights..."));
    connect(exportAction, &QAction::triggered,
            this, &MainWindow::onExportHighlights);

    m_highlightMenu->addSeparator();

    // 撤销/重做
    QAction* undoAction = m_highlightMenu->addAction(tr("Undo"));
    undoAction->setShortcut(QKeySequence::Undo);
    connect(undoAction, &QAction::triggered, m_undoStack, &QUndoStack::undo);

    QAction* redoAction = m_highlightMenu->addAction(tr("Redo"));
    redoAction->setShortcut(QKeySequence::Redo);
    connect(redoAction, &QAction::triggered, m_undoStack, &QUndoStack::redo);
}

void MainWindow::createHighlightToolBar() {
    m_highlightToolBar = addToolBar(tr("Highlight"));

    // 快速高亮按钮
    QAction* highlightAction = m_highlightToolBar->addAction(
        QIcon(":/icons/highlight"), tr("Highlight"));
    connect(highlightAction, &QAction::triggered,
            this, &MainWindow::onHighlightText);

    // 颜色选择器
    QComboBox* colorCombo = new QComboBox(this);
    for (auto color : HighlightColorManager::getAllPresets()) {
        QString colorName = HighlightColorManager::getColorName(color);
        QPixmap pixmap(16, 16);
        pixmap.fill(HighlightColorManager::getDefaultColor(color));
        colorCombo->addItem(QIcon(pixmap), colorName, static_cast<int>(color));
    }
    m_highlightToolBar->addWidget(colorCombo);

    connect(colorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this, colorCombo](int index) {
                HighlightColor color = static_cast<HighlightColor>(
                    colorCombo->itemData(index).toInt());
                HighlightManager::instance().setDefaultColor(color);
            });

    // 撤销/重做按钮
    m_highlightToolBar->addSeparator();
    m_highlightToolBar->addAction(QIcon(":/icons/undo"), tr("Undo"),
                                    m_undoStack, &QUndoStack::undo);
    m_highlightToolBar->addAction(QIcon(":/icons/redo"), tr("Redo"),
                                    m_undoStack, &QUndoStack::redo);
}

// 槽函数实现
void MainWindow::onHighlightAdded(const TextHighlight& highlight) {
    SLOG_INFO("Highlight added on page %1").arg(highlight.pageNumber);
    // 触发 PDF 视图重绘
    if (m_pdfViewer) {
        m_pdfViewer->update();
    }
    // 更新高亮列表
    if (m_highlightListWidget) {
        m_highlightListWidget->refresh();
    }
}

void MainWindow::onHighlightRemoved(const QString& highlightId) {
    SLOG_INFO("Highlight removed: %1").arg(highlightId);
    if (m_pdfViewer) {
        m_pdfViewer->update();
    }
    if (m_highlightListWidget) {
        m_highlightListWidget->refresh();
    }
}

void MainWindow::onHighlightUpdated(const TextHighlight& highlight) {
    SLOG_INFO("Highlight updated: %1").arg(highlight.id);
    if (m_pdfViewer) {
        m_pdfViewer->update();
    }
}

void MainWindow::onHighlightText() {
    if (!m_textSelectionManager || !m_textSelectionManager->hasSelection()) {
        QMessageBox::information(this, tr("No Selection"),
            tr("Please select text first."));
        return;
    }

    TextSelection selection = m_textSelectionManager->getSelection();
    int currentPage = m_pdfViewer->getCurrentPageNumber();

    auto& manager = HighlightManager::instance();
    if (manager.addHighlightFromSelection(selection, currentPage)) {
        m_textSelectionManager->clearSelection();
    }
}

void MainWindow::onHighlightWithNote() {
    if (!m_textSelectionManager || !m_textSelectionManager->hasSelection()) {
        QMessageBox::information(this, tr("No Selection"),
            tr("Please select text first."));
        return;
    }

    bool ok;
    QString note = QInputDialog::getText(this, tr("Add Note"),
        tr("Enter your note:"), QLineEdit::Normal, QString(), &ok);

    if (ok && !note.isEmpty()) {
        TextSelection selection = m_textSelectionManager->getSelection();
        int currentPage = m_pdfViewer->getCurrentPageNumber();

        auto& manager = HighlightManager::instance();
        if (manager.addHighlightWithNote(selection, currentPage, note)) {
            m_textSelectionManager->clearSelection();
        }
    }
}

void MainWindow::onExportHighlights() {
    QStringList formats;
    formats << "Markdown (*.md)"
            << "Plain Text (*.txt)"
            << "JSON (*.json)";

    QString filter = formats.join(";;");
    QString filePath = QFileDialog::getSaveFileName(
        this, tr("Export Highlights"), QString(), filter);

    if (filePath.isEmpty()) {
        return;
    }

    // 确定格式
    QString format = "json";
    if (filePath.endsWith(".md")) {
        format = "markdown";
    } else if (filePath.endsWith(".txt")) {
        format = "text";
    }

    auto& manager = HighlightManager::instance();
    if (manager.exportToFile(filePath, format)) {
        QMessageBox::information(this, tr("Export Successful"),
            tr("Highlights exported to %1").arg(filePath));
    } else {
        QMessageBox::warning(this, tr("Export Failed"),
            tr("Failed to export highlights"));
    }
}

void MainWindow::onShowHighlightList() {
    // 显示或切换高亮列表侧边栏
    if (m_highlightListWidget) {
        m_highlightListWidget->setVisible(!m_highlightListWidget->isVisible());
    }
}

void MainWindow::highlightSelectedText(HighlightColor color) {
    if (!m_textSelectionManager || !m_textSelectionManager->hasSelection()) {
        return;
    }

    TextSelection selection = m_textSelectionManager->getSelection();
    int currentPage = m_pdfViewer->getCurrentPageNumber();

    auto& manager = HighlightManager::instance();
    if (manager.addHighlightFromSelection(selection, currentPage, color, 0.4)) {
        m_textSelectionManager->clearSelection();
    }
}

// 文档打开时
void MainWindow::openDocument(const QString& filePath) {
    // ... 加载 PDF 文档 ...

    // 设置文档到高亮管理器
    auto& highlightManager = HighlightManager::instance();
    highlightManager.setDocument(m_document, filePath);

    // 发布文档打开事件
    EventBus::instance().publish("document_opened", filePath);

    SLOG_INFO("Document opened: %1").arg(filePath);
}

// 文档关闭时
void MainWindow::closeDocument() {
    // 发布文档关闭事件（会触发高亮自动保存）
    EventBus::instance().publish("document_closed", QVariant());

    // ... 关闭 PDF 文档 ...

    SLOG_INFO("Document closed");
}
```

## 2. PDF 渲染集成

### 2.1 在 RenderModel 或 PDF 视图中集成高亮渲染

```cpp
// PDFPageWidget.cpp 或 RenderModel.cpp
void PDFPageWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);

    // 1. 渲染 PDF 页面内容
    if (m_pageImage) {
        painter.drawImage(0, 0, *m_pageImage);
    }

    // 2. 渲染高亮（在 PDF 内容之上）
    auto& highlightManager = HighlightManager::instance();
    auto highlights = highlightManager.getHighlightsForPage(m_pageNumber);

    if (!highlights.isEmpty()) {
        HighlightRenderer::renderHighlights(painter, highlights, m_scaleFactor);
    }

    // 3. 渲染文本选择（在高亮之上）
    if (m_textSelectionManager && m_textSelectionManager->hasSelection()) {
        m_textSelectionManager->renderSelection(painter, m_scaleFactor);
    }

    // 4. 渲染其他 UI 元素...
}
```

## 3. 右键菜单集成

```cpp
// PDFPageWidget.cpp
void PDFPageWidget::contextMenuEvent(QContextMenuEvent* event) {
    QMenu contextMenu(this);
    QPoint globalPos = event->globalPos();
    QPointF pagePos = mapToPage(event->pos());

    auto& highlightManager = HighlightManager::instance();

    // 检查是否有文本选择
    if (m_textSelectionManager && m_textSelectionManager->hasSelection()) {
        TextSelection selection = m_textSelectionManager->getSelection();

        // 添加高亮子菜单
        QMenu* highlightMenu = contextMenu.addMenu(
            QIcon(":/icons/highlight"), tr("Highlight"));

        for (auto color : HighlightColorManager::getAllPresets()) {
            QString colorName = HighlightColorManager::getColorName(color);
            QAction* action = highlightMenu->addAction(colorName);

            // 设置颜色图标
            QPixmap pixmap(16, 16);
            pixmap.fill(HighlightColorManager::getDefaultColor(color));
            action->setIcon(QIcon(pixmap));

            connect(action, &QAction::triggered, [=]() {
                highlightManager.addHighlightFromSelection(
                    selection, m_pageNumber, color, 0.4);
                m_textSelectionManager->clearSelection();
                update();
            });
        }

        contextMenu.addSeparator();

        // 添加备注
        QAction* noteAction = contextMenu.addAction(
            QIcon(":/icons/note"), tr("Highlight with Note..."));
        connect(noteAction, &QAction::triggered, [=]() {
            bool ok;
            QString note = QInputDialog::getText(
                this, tr("Add Note"), tr("Enter your note:"),
                QLineEdit::Normal, QString(), &ok);
            if (ok && !note.isEmpty()) {
                highlightManager.addHighlightWithNote(
                    selection, m_pageNumber, note);
                m_textSelectionManager->clearSelection();
                update();
            }
        });

        // 复制文本
        contextMenu.addAction(QIcon(":/icons/copy"), tr("Copy"),
            m_textSelectionManager, &TextSelectionManager::copySelectionToClipboard);
    }

    // 检查是否点击在高亮上
    TextHighlight highlight = highlightManager.findHighlightAtPoint(
        m_pageNumber, pagePos);

    if (!highlight.isEmpty()) {
        if (contextMenu.actions().size() > 0) {
            contextMenu.addSeparator();
        }

        // 编辑备注
        QAction* editNoteAction = contextMenu.addAction(
            QIcon(":/icons/edit"), tr("Edit Note..."));
        connect(editNoteAction, &QAction::triggered, [=]() {
            bool ok;
            QString note = QInputDialog::getText(
                this, tr("Edit Note"), tr("Enter your note:"),
                QLineEdit::Normal, highlight.note, &ok);
            if (ok) {
                highlightManager.editHighlightNote(highlight.id, note);
                update();
            }
        });

        // 修改颜色
        QMenu* changeColorMenu = contextMenu.addMenu(
            QIcon(":/icons/color"), tr("Change Color"));
        for (auto color : HighlightColorManager::getAllPresets()) {
            QString colorName = HighlightColorManager::getColorName(color);
            QAction* action = changeColorMenu->addAction(colorName);

            QPixmap pixmap(16, 16);
            pixmap.fill(HighlightColorManager::getDefaultColor(color));
            action->setIcon(QIcon(pixmap));

            connect(action, &QAction::triggered, [=]() {
                QColor qcolor = HighlightColorManager::getDefaultColor(color);
                highlightManager.changeHighlightColor(highlight.id, qcolor);
                update();
            });
        }

        // 删除高亮
        QAction* deleteAction = contextMenu.addAction(
            QIcon(":/icons/delete"), tr("Delete Highlight"));
        connect(deleteAction, &QAction::triggered, [=]() {
            highlightManager.removeHighlight(highlight.id);
            update();
        });
    }

    contextMenu.exec(globalPos);
}
```

## 4. 键盘快捷键

已在 MainWindow 的菜单创建中设置：

- **Ctrl+H** - 快速高亮（使用默认颜色）
- **Ctrl+Shift+H** - 高亮并添加备注
- **Ctrl+Shift+L** - 显示/隐藏高亮列表
- **Ctrl+Z** - 撤销
- **Ctrl+Y** 或 **Ctrl+Shift+Z** - 重做

## 5. EventBus 集成

高亮系统已自动集成到 EventBus，发布以下事件：

- `highlight_added` - 高亮添加时
- `highlight_removed` - 高亮删除时
- `highlight_updated` - 高亮更新时

订阅示例：

```cpp
EventBus::instance().subscribe("highlight_added", this,
    [](const QVariant& data) {
        QVariantMap map = data.toMap();
        QString id = map["id"].toString();
        int pageNumber = map["pageNumber"].toInt();
        QString text = map["text"].toString();

        SLOG_INFO("New highlight on page %1: %2").arg(pageNumber).arg(text);
    });
```

## 6. 编译配置

确保在 CMakeLists.txt 中包含新文件：

```cmake
# app/CMakeLists.txt
set(APP_SOURCES
    # ... 其他源文件 ...
    model/HighlightModel.cpp
    command/HighlightCommands.cpp
    managers/HighlightManager.cpp
)
```

## 7. 测试集成

运行集成测试：

```bash
# 编译项目
cmake --build build

# 运行测试
ctest --test-dir build --output-on-failure

# 或使用测试脚本
./scripts/run_tests.ps1 -TestType Integration
```

## 总结

按照以上步骤，高亮系统将完全集成到 SAST Readium 项目中，提供完整的文本高亮功能。所有功能都遵循项目的架构模式，并与现有系统无缝集成。
