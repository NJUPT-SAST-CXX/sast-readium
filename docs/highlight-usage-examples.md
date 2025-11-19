# 高亮系统使用示例

本文档提供了 SAST Readium 高亮系统的实际使用示例代码。

## 基础使用

### 1. 初始化高亮管理器

```cpp
#include "managers/HighlightManager.h"
#include "model/HighlightModel.h"
#include "command/HighlightCommands.h"

// 在应用初始化时
void MainWindow::initializeHighlightSystem() {
    // 获取高亮管理器单例
    auto& highlightManager = HighlightManager::instance();

    // 设置撤销栈
    highlightManager.setUndoStack(m_undoStack);

    // 设置文本选择管理器
    highlightManager.setTextSelectionManager(m_textSelectionManager);

    // 连接信号
    connect(&highlightManager, &HighlightManager::highlightAdded,
            this, &MainWindow::onHighlightAdded);
    connect(&highlightManager, &HighlightManager::highlightRemoved,
            this, &MainWindow::onHighlightRemoved);

    // 启用自动保存
    highlightManager.setAutoSaveEnabled(true);

    // 设置默认颜色和透明度
    highlightManager.setDefaultColor(HighlightColor::Yellow);
    highlightManager.setDefaultOpacity(0.4);
}
```

### 2. 打开文档时加载高亮

```cpp
void MainWindow::openDocument(const QString& filePath) {
    // 加载 PDF 文档
    m_document = Poppler::Document::load(filePath);

    // 设置文档到高亮管理器
    auto& highlightManager = HighlightManager::instance();
    highlightManager.setDocument(m_document, filePath);

    // 加载已保存的高亮
    if (!highlightManager.loadHighlights()) {
        SLOG_WARN("No saved highlights found for this document");
    }

    // 更新 UI
    updateHighlightList();
}
```

### 3. 从文本选择创建高亮

```cpp
void PDFViewer::onTextSelected() {
    if (!m_textSelectionManager->hasSelection()) {
        return;
    }

    // 获取当前选择
    TextSelection selection = m_textSelectionManager->getSelection();
    int currentPage = getCurrentPageNumber();

    // 创建高亮
    auto& highlightManager = HighlightManager::instance();
    bool success = highlightManager.addHighlightFromSelection(
        selection,
        currentPage,
        HighlightColor::Yellow,
        0.4
    );

    if (success) {
        // 清除选择
        m_textSelectionManager->clearSelection();

        // 触发重绘
        update();
    }
}
```

## 高级功能

### 4. 右键菜单集成

```cpp
void PDFViewer::showContextMenu(const QPoint& pos) {
    QMenu contextMenu(this);
    QPoint globalPos = mapToGlobal(pos);
    QPointF pagePos = mapToPage(pos);
    int pageNumber = getCurrentPageNumber();

    auto& highlightManager = HighlightManager::instance();

    // 检查是否有文本选择
    if (m_textSelectionManager->hasSelection()) {
        TextSelection selection = m_textSelectionManager->getSelection();

        // 添加高亮子菜单
        QMenu* highlightMenu = contextMenu.addMenu(QIcon(":/icons/highlight"),
                                                     tr("Highlight"));

        // 添加各种颜色选项
        addHighlightColorAction(highlightMenu, HighlightColor::Yellow,
                                 "Yellow", selection, pageNumber);
        addHighlightColorAction(highlightMenu, HighlightColor::Green,
                                 "Green", selection, pageNumber);
        addHighlightColorAction(highlightMenu, HighlightColor::Blue,
                                 "Blue", selection, pageNumber);
        addHighlightColorAction(highlightMenu, HighlightColor::Pink,
                                 "Pink", selection, pageNumber);
        addHighlightColorAction(highlightMenu, HighlightColor::Orange,
                                 "Orange", selection, pageNumber);

        contextMenu.addSeparator();

        // 添加带备注的高亮
        QAction* noteAction = contextMenu.addAction(
            QIcon(":/icons/note"), tr("Highlight with Note"));
        connect(noteAction, &QAction::triggered, [=]() {
            QString note = QInputDialog::getText(
                this, tr("Add Note"), tr("Enter your note:"));
            if (!note.isEmpty()) {
                highlightManager.addHighlightWithNote(
                    selection, pageNumber, note, HighlightColor::Yellow);
            }
        });

        // 复制文本
        QAction* copyAction = contextMenu.addAction(
            QIcon(":/icons/copy"), tr("Copy"));
        connect(copyAction, &QAction::triggered, [=]() {
            m_textSelectionManager->copySelectionToClipboard();
        });
    }

    // 检查是否点击在高亮上
    TextHighlight highlight = highlightManager.findHighlightAtPoint(
        pageNumber, pagePos);

    if (!highlight.isEmpty()) {
        if (contextMenu.actions().size() > 0) {
            contextMenu.addSeparator();
        }

        // 编辑备注
        QAction* editNoteAction = contextMenu.addAction(
            QIcon(":/icons/edit"), tr("Edit Note"));
        connect(editNoteAction, &QAction::triggered, [=]() {
            editHighlightNote(highlight.id);
        });

        // 修改颜色
        QMenu* changeColorMenu = contextMenu.addMenu(
            QIcon(":/icons/color"), tr("Change Color"));
        for (auto color : HighlightColorManager::getAllPresets()) {
            QString colorName = HighlightColorManager::getColorName(color);
            QAction* colorAction = changeColorMenu->addAction(colorName);
            connect(colorAction, &QAction::triggered, [=]() {
                QColor qcolor = HighlightColorManager::getDefaultColor(color);
                highlightManager.changeHighlightColor(highlight.id, qcolor);
            });
        }

        // 删除高亮
        QAction* deleteAction = contextMenu.addAction(
            QIcon(":/icons/delete"), tr("Delete Highlight"));
        connect(deleteAction, &QAction::triggered, [=]() {
            highlightManager.removeHighlight(highlight.id);
        });
    }

    contextMenu.exec(globalPos);
}

void PDFViewer::addHighlightColorAction(QMenu* menu, HighlightColor color,
                                         const QString& name,
                                         const TextSelection& selection,
                                         int pageNumber) {
    QAction* action = menu->addAction(name);

    // 设置颜色图标
    QPixmap pixmap(16, 16);
    pixmap.fill(HighlightColorManager::getDefaultColor(color));
    action->setIcon(QIcon(pixmap));

    connect(action, &QAction::triggered, [=]() {
        HighlightManager::instance().addHighlightFromSelection(
            selection, pageNumber, color, 0.4);
    });
}
```

### 5. 高亮列表侧边栏

```cpp
class HighlightListWidget : public QWidget {
    Q_OBJECT

public:
    explicit HighlightListWidget(QWidget* parent = nullptr)
        : QWidget(parent) {
        setupUI();
        connectSignals();
        loadHighlights();
    }

private slots:
    void onHighlightClicked(const QModelIndex& index) {
        QString highlightId = m_model->data(index,
            HighlightModel::IdRole).toString();
        TextHighlight highlight = HighlightManager::instance()
            .getHighlight(highlightId);

        // 跳转到高亮位置
        emit jumpToPage(highlight.pageNumber);
        emit highlightSelected(highlightId);
    }

    void onSearchTextChanged(const QString& text) {
        if (text.isEmpty()) {
            loadHighlights();
        } else {
            auto results = HighlightManager::instance()
                .searchHighlights(text);
            updateList(results);
        }
    }

    void onFilterByColor(HighlightColor color) {
        auto results = HighlightManager::instance()
            .model()->getHighlightsByColor(color);
        updateList(results);
    }

private:
    void setupUI() {
        QVBoxLayout* layout = new QVBoxLayout(this);

        // 搜索框
        m_searchEdit = new QLineEdit(this);
        m_searchEdit->setPlaceholderText(tr("Search highlights..."));
        layout->addWidget(m_searchEdit);

        // 颜色过滤器
        QHBoxLayout* filterLayout = new QHBoxLayout();
        QLabel* filterLabel = new QLabel(tr("Filter by color:"), this);
        m_colorFilter = new QComboBox(this);
        m_colorFilter->addItem(tr("All Colors"));
        for (auto color : HighlightColorManager::getAllPresets()) {
            m_colorFilter->addItem(
                HighlightColorManager::getColorName(color));
        }
        filterLayout->addWidget(filterLabel);
        filterLayout->addWidget(m_colorFilter);
        layout->addLayout(filterLayout);

        // 高亮列表
        m_listView = new QListView(this);
        m_model = HighlightManager::instance().model();
        m_listView->setModel(m_model);
        layout->addWidget(m_listView);

        // 统计信息
        m_statsLabel = new QLabel(this);
        layout->addWidget(m_statsLabel);

        // 导出按钮
        QPushButton* exportBtn = new QPushButton(tr("Export..."), this);
        connect(exportBtn, &QPushButton::clicked,
                this, &HighlightListWidget::onExportClicked);
        layout->addWidget(exportBtn);
    }

    void connectSignals() {
        connect(m_searchEdit, &QLineEdit::textChanged,
                this, &HighlightListWidget::onSearchTextChanged);
        connect(m_listView, &QListView::clicked,
                this, &HighlightListWidget::onHighlightClicked);
        connect(m_colorFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &HighlightListWidget::onColorFilterChanged);

        auto& manager = HighlightManager::instance();
        connect(&manager, &HighlightManager::highlightAdded,
                this, &HighlightListWidget::onHighlightAdded);
        connect(&manager, &HighlightManager::highlightRemoved,
                this, &HighlightListWidget::onHighlightRemoved);
    }

    void loadHighlights() {
        updateStats();
    }

    void updateStats() {
        auto& manager = HighlightManager::instance();
        int total = manager.getTotalHighlightCount();
        auto stats = manager.getStatistics();

        QString text = tr("Total: %1 highlights on %2 pages")
            .arg(total).arg(stats.totalPages);
        m_statsLabel->setText(text);
    }

signals:
    void jumpToPage(int pageNumber);
    void highlightSelected(const QString& highlightId);

private:
    QLineEdit* m_searchEdit;
    QComboBox* m_colorFilter;
    QListView* m_listView;
    QLabel* m_statsLabel;
    HighlightModel* m_model;
};
```

### 6. 导出功能

```cpp
void MainWindow::exportHighlights() {
    QStringList formats;
    formats << "Markdown (*.md)"
            << "Plain Text (*.txt)"
            << "JSON (*.json)"
            << "HTML (*.html)"
            << "CSV (*.csv)";

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
    } else if (filePath.endsWith(".html")) {
        format = "html";
    } else if (filePath.endsWith(".csv")) {
        format = "csv";
    }

    // 导出
    auto& manager = HighlightManager::instance();
    bool success = manager.exportToFile(filePath, format);

    if (success) {
        QMessageBox::information(this, tr("Export Successful"),
            tr("Highlights exported to %1").arg(filePath));
    } else {
        QMessageBox::warning(this, tr("Export Failed"),
            tr("Failed to export highlights"));
    }
}
```

### 7. 键盘快捷键

```cpp
void MainWindow::setupHighlightShortcuts() {
    // Ctrl+H - 高亮选中文本（黄色）
    QShortcut* highlightShortcut = new QShortcut(
        QKeySequence(Qt::CTRL | Qt::Key_H), this);
    connect(highlightShortcut, &QShortcut::activated, [=]() {
        if (m_textSelectionManager->hasSelection()) {
            auto selection = m_textSelectionManager->getSelection();
            HighlightManager::instance().addHighlightFromSelection(
                selection, getCurrentPageNumber(), HighlightColor::Yellow);
        }
    });

    // Ctrl+Shift+H - 高亮并添加备注
    QShortcut* highlightNoteShortcut = new QShortcut(
        QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_H), this);
    connect(highlightNoteShortcut, &QShortcut::activated, [=]() {
        if (m_textSelectionManager->hasSelection()) {
            QString note = QInputDialog::getText(
                this, tr("Add Note"), tr("Enter your note:"));
            if (!note.isEmpty()) {
                auto selection = m_textSelectionManager->getSelection();
                HighlightManager::instance().addHighlightWithNote(
                    selection, getCurrentPageNumber(), note);
            }
        }
    });

    // Delete - 删除选中的高亮
    QShortcut* deleteShortcut = new QShortcut(
        QKeySequence(Qt::Key_Delete), this);
    connect(deleteShortcut, &QShortcut::activated, [=]() {
        QString selectedHighlightId = getSelectedHighlightId();
        if (!selectedHighlightId.isEmpty()) {
            HighlightManager::instance().removeHighlight(selectedHighlightId);
        }
    });
}
```

## 完整示例：PDF 查看器集成

```cpp
class PDFViewerWidget : public QWidget {
    Q_OBJECT

public:
    explicit PDFViewerWidget(QWidget* parent = nullptr)
        : QWidget(parent) {
        initializeComponents();
        setupHighlightSystem();
    }

    void openDocument(const QString& filePath) {
        m_document = Poppler::Document::load(filePath);
        m_documentPath = filePath;

        // 初始化高亮管理器
        auto& highlightManager = HighlightManager::instance();
        highlightManager.setDocument(m_document, filePath);
        highlightManager.loadHighlights();

        // 加载第一页
        loadPage(0);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);

        // 渲染 PDF 页面
        if (m_currentPageImage) {
            painter.drawImage(0, 0, *m_currentPageImage);
        }

        // 渲染高亮
        auto highlights = HighlightManager::instance()
            .getHighlightsForPage(m_currentPage);
        HighlightRenderer::renderHighlights(painter, highlights, m_scaleFactor);

        // 渲染当前选择
        if (m_textSelectionManager->hasSelection()) {
            m_textSelectionManager->renderSelection(painter, m_scaleFactor);
        }
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            QPointF pagePos = mapToPage(event->pos());
            m_textSelectionManager->startSelection(pagePos);
        }
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (m_textSelectionManager->isSelecting()) {
            QPointF pagePos = mapToPage(event->pos());
            m_textSelectionManager->updateSelection(pagePos);
            update();
        }
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            m_textSelectionManager->endSelection();
            update();
        }
    }

    void mouseDoubleClickEvent(QMouseEvent* event) override {
        // 双击选择单词
        QPointF pagePos = mapToPage(event->pos());
        m_textSelectionManager->selectWordAt(pagePos);
        update();
    }

private:
    void initializeComponents() {
        m_textSelectionManager = new TextSelectionManager(this);
        m_undoStack = new QUndoStack(this);
    }

    void setupHighlightSystem() {
        auto& highlightManager = HighlightManager::instance();
        highlightManager.setUndoStack(m_undoStack);
        highlightManager.setTextSelectionManager(m_textSelectionManager);

        connect(&highlightManager, &HighlightManager::highlightAdded,
                this, [=]() { update(); });
        connect(&highlightManager, &HighlightManager::highlightRemoved,
                this, [=]() { update(); });
        connect(&highlightManager, &HighlightManager::highlightUpdated,
                this, [=]() { update(); });
    }

    QPointF mapToPage(const QPoint& widgetPos) {
        // 将窗口坐标转换为页面坐标
        return QPointF(widgetPos.x() / m_scaleFactor,
                       widgetPos.y() / m_scaleFactor);
    }

private:
    Poppler::Document* m_document = nullptr;
    QString m_documentPath;
    int m_currentPage = 0;
    double m_scaleFactor = 1.0;
    QImage* m_currentPageImage = nullptr;

    TextSelectionManager* m_textSelectionManager;
    QUndoStack* m_undoStack;
};
```

这些示例展示了如何在实际应用中集成和使用高亮系统的各个功能。
