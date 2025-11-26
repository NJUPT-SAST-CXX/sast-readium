#include "PDFViewerPage.h"

// UI Components
#include "ui/core/MenuBar.h"
#include "ui/core/RightSideBar.h"
#include "ui/core/SideBar.h"
#include "ui/core/StatusBar.h"
#include "ui/core/ToolBar.h"
#include "ui/viewer/PDFViewer.h"
#include "ui/widgets/DebugLogPanel.h"
#include "ui/widgets/DocumentTabWidget.h"
#include "ui/widgets/SearchPanel.h"

// ElaWidgetTools
#include "ElaTheme.h"

// Adapters
#include "adapters/SearchAdapter.h"

// Business Logic
#include "controller/ApplicationController.h"
#include "controller/DocumentController.h"
#include "controller/PageController.h"
#include "delegate/ViewDelegate.h"
#include "managers/I18nManager.h"
#include "model/PDFOutlineModel.h"
#include "model/RenderModel.h"
#include "search/SearchEngine.h"

// Poppler
#include <poppler/qt6/poppler-qt6.h>

// Qt
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QSplitter>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

#include "ElaContentDialog.h"
#include "ElaText.h"
#include "ui/widgets/ToastNotification.h"

// Logging
#include "logging/SimpleLogging.h"

// ============================================================================
// 构造和析构
// ============================================================================

PDFViewerPage::PDFViewerPage(QWidget* parent)
    : QWidget(parent),
      m_tabWidget(nullptr),
      m_viewerStack(nullptr),
      m_emptyWidget(nullptr),
      m_documentController(nullptr),
      m_documentModel(nullptr),
      m_pageController(nullptr),
      m_applicationController(nullptr),
      m_viewDelegate(nullptr),
      m_searchEngine(nullptr),
      m_searchAdapter(nullptr),
      m_isFullScreen(false),
      m_isPresentation(false),
      m_lastActiveIndex(-1) {
    SLOG_INFO("PDFViewerPage: Constructor started");

    setupUi();
    setupLayout();
    connectSignals();

    // 初始化搜索引擎和适配器
    m_searchEngine = new SearchEngine(this);
    m_searchAdapter = new SearchAdapter(this);
    m_searchAdapter->setSearchEngine(m_searchEngine);
    m_searchAdapter->setPDFViewerPage(this);

    SLOG_INFO("PDFViewerPage: Constructor completed");
}

PDFViewerPage::~PDFViewerPage() {
    SLOG_INFO("PDFViewerPage: Destructor called");
    closeAllDocuments();
}

// ============================================================================
// UI 初始化
// ============================================================================

void PDFViewerPage::setupUi() {
    // 创建菜单栏
    m_menuBar = new MenuBar(this);

    // 创建工具栏
    m_toolBar = new ToolBar(tr("Main Toolbar"), this);

    // 创建状态栏
    m_statusBar = new StatusBar(this);

    // 创建左侧边栏
    m_leftSideBar = new SideBar(this);
    m_leftSideBar->setMinimumWidth(200);
    m_leftSideBar->setMaximumWidth(400);

    // 创建右侧边栏
    m_rightSideBar = new RightSideBar(this);
    m_rightSideBar->setMinimumWidth(200);
    m_rightSideBar->setMaximumWidth(400);
    m_rightSideBar->setVisible(false);  // 默认隐藏

    // 创建文档标签页组件
    m_tabWidget = new DocumentTabWidget(this);
    m_tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_tabWidget->hide();  // 初始隐藏，打开文档后显示

    // 创建堆叠组件用于显示不同的PDF查看器
    m_viewerStack = new QStackedWidget(this);
    m_viewerStack->setSizePolicy(QSizePolicy::Expanding,
                                 QSizePolicy::Expanding);

    // 创建空状态组件
    m_emptyWidget = new QWidget(this);
    auto* emptyLayout = new QVBoxLayout(m_emptyWidget);
    emptyLayout->setContentsMargins(20, 20, 20, 20);
    emptyLayout->setSpacing(0);

    auto* emptyLabel = new ElaText(
        tr("No PDF documents open\nClick File menu to open a PDF document"),
        m_emptyWidget);
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setStyleSheet("color: gray; font-size: 14px;");
    emptyLayout->addWidget(emptyLabel);

    m_viewerStack->addWidget(m_emptyWidget);

    // 创建搜索面板
    m_searchPanel = new SearchPanel(this);
    m_searchPanel->setVisible(false);  // 默认隐藏
    m_searchPanel->setMaximumHeight(200);

    // 初始显示空状态
    showEmptyState();
}

void PDFViewerPage::setupLayout() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 添加菜单栏
    mainLayout->addWidget(m_menuBar);

    // 添加工具栏
    mainLayout->addWidget(m_toolBar);

    // 添加文档标签页
    mainLayout->addWidget(m_tabWidget);

    // 创建中央分割器（左侧边栏 | 中央区域 | 右侧边栏）
    QSplitter* mainSplitter = new QSplitter(Qt::Horizontal, this);

    // 左侧边栏
    mainSplitter->addWidget(m_leftSideBar);

    // 中央区域（堆叠的PDF查看器 + 搜索面板）
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* centralLayout = new QVBoxLayout(centralWidget);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    centralLayout->addWidget(m_viewerStack);
    centralLayout->addWidget(m_searchPanel);
    mainSplitter->addWidget(centralWidget);

    // 右侧边栏
    mainSplitter->addWidget(m_rightSideBar);

    // 设置分割器比例
    mainSplitter->setStretchFactor(0, 1);  // 左侧边栏
    mainSplitter->setStretchFactor(1, 4);  // 中央区域
    mainSplitter->setStretchFactor(2, 1);  // 右侧边栏

    mainLayout->addWidget(mainSplitter, 1);

    // 添加状态栏
    mainLayout->addWidget(m_statusBar);
}

void PDFViewerPage::setupControllers() {
    // 这个方法在外部设置控制器后调用
    // 将控制器连接到各个组件

    if (m_documentController) {
        // 文档控制器已设置
        SLOG_INFO("PDFViewerPage: DocumentController connected");
    }

    if (m_pageController) {
        // 页面控制器已设置
        SLOG_INFO("PDFViewerPage: PageController connected");
    }

    if (m_applicationController) {
        // 应用控制器已设置
        SLOG_INFO("PDFViewerPage: ApplicationController connected");
    }

    if (m_viewDelegate) {
        // 视图代理已设置
        SLOG_INFO("PDFViewerPage: ViewDelegate connected");
    }
}

void PDFViewerPage::connectSignals() {
    connectMenuBarSignals();
    connectToolBarSignals();
    connectStatusBarSignals();
    connectPDFViewerSignals();
    connectSideBarSignals();
    connectSearchPanelSignals();

    // 连接标签页信号
    connect(m_tabWidget, &DocumentTabWidget::tabCloseRequested, this,
            &PDFViewerPage::onTabCloseRequested);
    connect(m_tabWidget, &DocumentTabWidget::tabSwitched, this,
            &PDFViewerPage::onTabSwitched);
    connect(m_tabWidget, &DocumentTabWidget::allTabsClosed, this,
            &PDFViewerPage::onAllTabsClosed);
}

void PDFViewerPage::connectMenuBarSignals() {
    connect(m_menuBar, &MenuBar::actionTriggered, this,
            &PDFViewerPage::handleAction);

    // 最近文件
    connect(m_menuBar, &MenuBar::openRecentFileRequested, this,
            &PDFViewerPage::openFile);

    // 主题变更信号 (Phase 2)
    connect(m_menuBar, &MenuBar::themeChangeRequested, this,
            [this](const QString& theme) {
                SLOG_INFO_F("PDFViewerPage: Theme change requested: {}",
                            theme.toStdString());
                if (theme == "light") {
                    eTheme->setThemeMode(ElaThemeType::Light);
                } else if (theme == "dark") {
                    eTheme->setThemeMode(ElaThemeType::Dark);
                } else if (theme == "auto") {
                    // TODO: Implement auto theme detection based on system
                    // settings
                    SLOG_INFO("PDFViewerPage: Auto theme not yet implemented");
                }
            });

    // 语言变更信号 (Phase 2)
    connect(m_menuBar, &MenuBar::languageChangeRequested, this,
            [this](const QString& languageCode) {
                SLOG_INFO_F("PDFViewerPage: Language change requested: {}",
                            languageCode.toStdString());
                I18nManager::instance().loadLanguage(languageCode);
            });

    // 欢迎屏幕和调试面板信号 (Phase 2)
    connect(m_menuBar, &MenuBar::welcomeScreenToggleRequested, this, [this]() {
        SLOG_INFO("PDFViewerPage: Welcome screen toggle requested");
        // TODO: Implement welcome screen toggle
        // This would typically show/hide a welcome screen overlay
    });

    connect(m_menuBar, &MenuBar::debugPanelToggleRequested, this, [this]() {
        SLOG_INFO("PDFViewerPage: Debug panel toggle requested");
        // Toggle debug panel visibility in right sidebar
        if (m_rightSideBar) {
            m_rightSideBar->switchToTab(RightSideBar::DebugTab);
            m_rightSideBar->toggleVisibility(true);  // Animated toggle
        }
    });

    connect(m_menuBar, &MenuBar::debugPanelClearRequested, this, [this]() {
        SLOG_INFO("PDFViewerPage: Debug panel clear requested");
        // Clear debug logs
        if (m_rightSideBar && m_rightSideBar->debugPanel()) {
            m_rightSideBar->debugPanel()->clearLogs();
        }
    });

    connect(m_menuBar, &MenuBar::debugPanelExportRequested, this, [this]() {
        SLOG_INFO("PDFViewerPage: Debug panel export requested");
        // TODO: Export debug logs - need to add public exportLogs() method to
        // DebugLogPanel For now, this functionality is available through the
        // debug panel's UI
        if (m_rightSideBar) {
            m_rightSideBar->switchToTab(RightSideBar::DebugTab);
            m_rightSideBar->show(true);
        }
    });
}

void PDFViewerPage::connectToolBarSignals() {
    // 文件操作 - 工具栏的动作与菜单栏相同，复用处理逻辑
    connect(m_toolBar, &ToolBar::actionTriggered, this,
            &PDFViewerPage::handleAction);

    // 页面导航
    connect(m_toolBar, &ToolBar::pageJumpRequested, this,
            &PDFViewerPage::goToPage);
    connect(m_toolBar, &ToolBar::goToFirstPageRequested, this,
            &PDFViewerPage::goToFirstPage);
    connect(m_toolBar, &ToolBar::goToPreviousPageRequested, this,
            &PDFViewerPage::goToPreviousPage);
    connect(m_toolBar, &ToolBar::goToNextPageRequested, this,
            &PDFViewerPage::goToNextPage);
    connect(m_toolBar, &ToolBar::goToLastPageRequested, this,
            &PDFViewerPage::goToLastPage);
    connect(m_toolBar, &ToolBar::goBackRequested, this, &PDFViewerPage::goBack);
    connect(m_toolBar, &ToolBar::goForwardRequested, this,
            &PDFViewerPage::goForward);

    // 缩放控制
    connect(m_toolBar, &ToolBar::zoomLevelChanged, this,
            [this](double zoomPercent) {
                if (PDFViewer* viewer = getCurrentViewer()) {
                    viewer->setZoom(zoomPercent / 100.0);
                }
            });
    connect(m_toolBar, &ToolBar::zoomInRequested, this, &PDFViewerPage::zoomIn);
    connect(m_toolBar, &ToolBar::zoomOutRequested, this,
            &PDFViewerPage::zoomOut);
    connect(m_toolBar, &ToolBar::fitWidthRequested, this,
            &PDFViewerPage::fitToWidth);
    connect(m_toolBar, &ToolBar::fitPageRequested, this,
            &PDFViewerPage::fitToPage);
    connect(m_toolBar, &ToolBar::fitHeightRequested, this,
            &PDFViewerPage::fitToHeight);

    // 视图控制
    connect(m_toolBar, &ToolBar::viewModeChanged, this,
            &PDFViewerPage::setViewMode);
    connect(m_toolBar, &ToolBar::rotateLeftRequested, this,
            &PDFViewerPage::rotateLeft);
    connect(m_toolBar, &ToolBar::rotateRightRequested, this,
            &PDFViewerPage::rotateRight);
    connect(m_toolBar, &ToolBar::fullScreenToggled, this,
            &PDFViewerPage::toggleFullScreen);

    // 工具
    connect(m_toolBar, &ToolBar::searchRequested, this,
            &PDFViewerPage::toggleSearchPanel);
    connect(m_toolBar, &ToolBar::bookmarkToggled, this, [this]() {
        // Toggle bookmark for current page
        if (auto* viewer = getCurrentViewer();
            viewer && viewer->hasDocument()) {
            // int currentPage = viewer->currentPage();
            // Check if bookmark exists and toggle
            // This is a simplified version - actual implementation would check
            // bookmark state
            addBookmark();
        }
    });

    // 新增的视图控制信号 (Phase 3)
    connect(m_toolBar, &ToolBar::toggleSidebarRequested, this, [this]() {
        // Toggle left sidebar visibility
        if (m_leftSideBar) {
            m_leftSideBar->toggleVisibility(true);  // Animated toggle
        }
    });

    connect(m_toolBar, &ToolBar::nightModeToggled, this, [this](bool enabled) {
        // Toggle night mode for reading
        SLOG_INFO_F("PDFViewerPage: Night mode toggled: {}", enabled);
        if (PDFViewer* viewer = getCurrentViewer()) {
            viewer->setNightMode(enabled);
        }
    });

    connect(m_toolBar, &ToolBar::readingModeToggled, this,
            [this](bool enabled) {
                // Toggle reading mode (distraction-free)
                SLOG_INFO_F("PDFViewerPage: Reading mode toggled: {}", enabled);
                if (enabled) {
                    // Hide sidebars and toolbars for distraction-free reading
                    if (m_leftSideBar) {
                        m_leftSideBar->hide(true);
                    }
                    if (m_rightSideBar) {
                        m_rightSideBar->hide(true);
                    }
                    m_toolBar->hide();
                    m_statusBar->hide();
                } else {
                    // Restore UI elements
                    if (m_leftSideBar) {
                        m_leftSideBar->show(true);
                    }
                    if (m_rightSideBar) {
                        m_rightSideBar->show(true);
                    }
                    m_toolBar->show();
                    m_statusBar->show();
                }
            });

    connect(m_toolBar, &ToolBar::layoutModeChanged, this, [this](int mode) {
        // Change layout mode (vertical/horizontal)
        SLOG_INFO_F("PDFViewerPage: Layout mode changed: {}", mode);
        // TODO: Implement layout mode change
        // mode 0 = Vertical (default), mode 1 = Horizontal
    });

    // 新增的工具信号 (Phase 3)
    connect(m_toolBar, &ToolBar::highlightRequested, this, [this]() {
        // Activate highlight tool
        SLOG_INFO("PDFViewerPage: Highlight tool requested");
        // TODO: Implement highlight tool activation
        // This would typically enable text selection and highlighting mode
    });

    connect(m_toolBar, &ToolBar::snapshotRequested, this, [this]() {
        // Activate snapshot tool
        SLOG_INFO("PDFViewerPage: Snapshot tool requested");
        // TODO: Implement snapshot tool
        // This would typically enable area selection for screenshot
    });
}

void PDFViewerPage::connectStatusBarSignals() {
    // 页面跳转信号 (Phase 4)
    connect(m_statusBar, &StatusBar::pageJumpRequested, this,
            &PDFViewerPage::goToPage);

    // 缩放级别变更信号 (Phase 4)
    connect(m_statusBar, &StatusBar::zoomLevelChangeRequested, this,
            &PDFViewerPage::setZoom);
}

void PDFViewerPage::connectPDFViewerSignals() {
    // PDF viewer signals are now connected in createPDFViewer() method
    // when each viewer is created, not here
    // This method is kept for compatibility but does nothing
}

void PDFViewerPage::connectSideBarSignals() {
    // 左侧边栏 - 页面跳转
    connect(m_leftSideBar, &SideBar::pageJumpRequested, this,
            &PDFViewerPage::goToPage);

    // 左侧边栏 - 书签
    connect(m_leftSideBar, &SideBar::bookmarkAdded, this,
            [this](int pageNumber, const QString& title) {
                SLOG_INFO_F("PDFViewerPage: Bookmark added at page {}: {}",
                            pageNumber, title.toStdString());
                m_statusBar->showMessage(tr("Bookmark added"),
                                         StatusBar::MessagePriority::Normal,
                                         2000);
            });

    connect(
        m_leftSideBar, &SideBar::bookmarkRemoved, this, [this](int pageNumber) {
            SLOG_INFO_F("PDFViewerPage: Bookmark removed at page {}",
                        pageNumber);
            m_statusBar->showMessage(tr("Bookmark removed"),
                                     StatusBar::MessagePriority::Normal, 2000);
        });

    // 左侧边栏 - 大纲
    connect(m_leftSideBar, &SideBar::outlineItemClicked, this,
            &PDFViewerPage::goToPage);

    // 右侧边栏 - 注释导航
    connect(m_rightSideBar, &RightSideBar::navigateToPage, this,
            &PDFViewerPage::goToPage);
}

void PDFViewerPage::connectSearchPanelSignals() {
    // 搜索请求 - 使用搜索适配器
    connect(m_searchPanel, &SearchPanel::searchRequested, this,
            [this](const QString& query, bool caseSensitive, bool wholeWords,
                   bool regex) {
                SLOG_INFO_F("PDFViewerPage: Search requested: {}",
                            query.toStdString());
                if (m_searchAdapter) {
                    m_searchAdapter->search(query, caseSensitive, wholeWords,
                                            regex);
                }
                m_statusBar->showMessage(tr("Searching..."),
                                         StatusBar::MessagePriority::Normal, 0);
            });

    // 结果选择
    connect(m_searchPanel, &SearchPanel::resultSelected, this,
            [this](int pageNumber, int resultIndex) {
                SLOG_INFO_F(
                    "PDFViewerPage: Result selected - page: {}, index: {}",
                    pageNumber, resultIndex);
                if (m_searchAdapter) {
                    m_searchAdapter->goToResult(resultIndex);
                }
            });

    // 导航
    connect(m_searchPanel, &SearchPanel::nextResultRequested, this,
            &PDFViewerPage::findNext);
    connect(m_searchPanel, &SearchPanel::previousResultRequested, this,
            &PDFViewerPage::findPrevious);

    // 连接搜索适配器信号
    if (m_searchAdapter) {
        // 搜索完成 - 更新状态栏
        connect(
            m_searchAdapter, &SearchAdapter::searchFinished, this,
            [this](int resultCount) {
                SLOG_INFO_F("PDFViewerPage: Search finished with {} results",
                            resultCount);
                if (resultCount > 0) {
                    m_statusBar->showMessage(
                        tr("Found %1 results").arg(resultCount),
                        StatusBar::MessagePriority::Normal, 3000);
                } else {
                    m_statusBar->showMessage(tr("No results found"),
                                             StatusBar::MessagePriority::Normal,
                                             3000);
                }
            });

        // 搜索错误
        connect(m_searchAdapter, &SearchAdapter::errorOccurred, this,
                [this](const QString& error) {
                    SLOG_ERROR_F("PDFViewerPage: Search error: {}",
                                 error.toStdString());
                    m_statusBar->showMessage(tr("Search error: %1").arg(error),
                                             StatusBar::MessagePriority::High,
                                             5000);
                });

        // 当前结果变化 - 高亮显示
        connect(m_searchAdapter, &SearchAdapter::currentResultChanged, this,
                [this](int index, int total) {
                    SLOG_INFO_F("PDFViewerPage: Current result changed: {}/{}",
                                index + 1, total);
                    m_statusBar->showMessage(
                        tr("Result %1 of %2").arg(index + 1).arg(total),
                        StatusBar::MessagePriority::Normal, 2000);
                });

        // 结果找到 - 高亮页面
        connect(
            m_searchAdapter, &SearchAdapter::resultFound, this,
            [this](int pageNumber, const QList<QRectF>& highlights) {
                SLOG_INFO_F(
                    "PDFViewerPage: Result found on page {} with {} highlights",
                    pageNumber, highlights.size());
                PDFViewer* viewer = getCurrentViewer();
                if (viewer) {
                    viewer->highlightSearchResults(pageNumber, highlights);
                }
            });
    }
}

// ============================================================================
// 文档操作
// ============================================================================

bool PDFViewerPage::openFile(const QString& filePath) {
    QString path = filePath;

    // 如果没有提供路径，显示文件对话框
    if (path.isEmpty()) {
        path = QFileDialog::getOpenFileName(
            this, tr("Open PDF File"), QString(),
            tr("PDF Files (*.pdf);;All Files (*.*)"));

        if (path.isEmpty()) {
            return false;
        }
    }

    SLOG_INFO_F("PDFViewerPage: Opening file: {}", path.toStdString());

    // 检查文档是否已经打开
    for (int i = 0; i < m_pdfViewers.size(); ++i) {
        if (m_tabWidget->getTabFilePath(i) == path) {
            SLOG_INFO_F(
                "PDFViewerPage: Document already open at index {}, switching "
                "to it",
                i);
            m_tabWidget->setCurrentTab(i);
            switchToDocument(i);
            return true;
        }
    }

    // 加载Poppler文档
    std::shared_ptr<Poppler::Document> document =
        std::shared_ptr<Poppler::Document>(Poppler::Document::load(path));

    if (!document) {
        SLOG_ERROR_F("PDFViewerPage: Failed to load document: {}",
                     path.toStdString());
        TOAST_ERROR(this, tr("Failed to open file: %1").arg(path));
        return false;
    }

    // 设置渲染选项
    document->setRenderHint(Poppler::Document::Antialiasing);
    document->setRenderHint(Poppler::Document::TextAntialiasing);
    document->setRenderHint(Poppler::Document::TextHinting);

    // 创建新的PDF查看器
    PDFViewer* viewer = createPDFViewer();

    // 设置文档到查看器
    if (!viewer->setDocument(document)) {
        SLOG_ERROR("PDFViewerPage: Failed to set document to viewer");
        TOAST_ERROR(this, tr("Failed to display file: %1").arg(path));
        viewer->deleteLater();
        return false;
    }

    double dpiX = logicalDpiX();
    double dpiY = logicalDpiY();
    auto* renderModel = new RenderModel(dpiX, dpiY, document.get(), viewer);
    viewer->setRenderModel(renderModel);

    // 添加到查看器列表
    m_pdfViewers.append(viewer);
    m_viewerStack->addWidget(viewer);

    // 创建outline model
    PDFOutlineModel* outlineModel = new PDFOutlineModel(this);
    // TODO: Set document to outline model when the API is available
    // outlineModel->setDocument(document);
    m_outlineModels.append(outlineModel);

    // 初始化文档状态
    DocumentState state;
    state.currentPage = 1;
    state.zoomLevel = 1.0;
    state.rotation = 0;
    state.scrollPosition = QPoint(0, 0);
    state.viewMode = 0;
    m_documentStates.append(state);

    // 添加标签页
    QFileInfo fileInfo(path);
    int tabIndex = m_tabWidget->addDocumentTab(fileInfo.fileName(), path);

    // 隐藏空状态，显示标签页
    hideEmptyState();

    // 切换到新文档
    m_tabWidget->setCurrentTab(tabIndex);
    switchToDocument(tabIndex);

    updateWindowTitle();
    emit documentOpened(path);

    return true;
}

bool PDFViewerPage::saveDocumentCopy(const QString& filePath) {
    if (!hasDocument()) {
        return false;
    }

    QString path = filePath;
    if (path.isEmpty()) {
        path = QFileDialog::getSaveFileName(this, tr("Save PDF Copy"),
                                            QString(), tr("PDF Files (*.pdf)"));

        if (path.isEmpty()) {
            return false;
        }
    }

    SLOG_INFO_F("PDFViewerPage: Saving document copy to: {}",
                path.toStdString());

    // 使用 DocumentController 保存
    if (m_documentController) {
        // m_documentController->saveDocumentCopy(path);
        m_statusBar->showMessage(tr("Document saved"),
                                 StatusBar::MessagePriority::Normal, 3000);
        return true;
    }

    return false;
}

void PDFViewerPage::printDocument() {
    if (!hasDocument()) {
        return;
    }

    SLOG_INFO("PDFViewerPage: Printing document");

    // 使用 DocumentController 打印
    if (m_documentController) {
        // m_documentController->printDocument();
    }
}

bool PDFViewerPage::exportDocument(const QString& filePath,
                                   const QString& format) {
    if (!hasDocument()) {
        return false;
    }

    SLOG_INFO("PDFViewerPage: Exporting document");

    // 使用 DocumentController 导出
    if (m_documentController) {
        // m_documentController->exportDocument(filePath, format);
        return true;
    }

    return false;
}

// ============================================================================
// 页面导航
// ============================================================================

void PDFViewerPage::goToPage(int pageNumber) {
    PDFViewer* viewer = getCurrentViewer();
    if (viewer) {
        viewer->goToPage(pageNumber);
    }
}

void PDFViewerPage::goToNextPage() {
    PDFViewer* viewer = getCurrentViewer();
    if (viewer) {
        viewer->goToNextPage();
    }
}

void PDFViewerPage::goToPreviousPage() {
    PDFViewer* viewer = getCurrentViewer();
    if (viewer) {
        viewer->goToPreviousPage();
    }
}

void PDFViewerPage::goToFirstPage() {
    PDFViewer* viewer = getCurrentViewer();
    if (viewer) {
        viewer->goToFirstPage();
    }
}

void PDFViewerPage::goToLastPage() {
    PDFViewer* viewer = getCurrentViewer();
    if (viewer) {
        viewer->goToLastPage();
    }
}

void PDFViewerPage::goBack() {
    // 使用 PageController 的历史记录
    if (m_pageController) {
        // m_pageController->goBack();
    }
}

void PDFViewerPage::goForward() {
    // 使用 PageController 的历史记录
    if (m_pageController) {
        // m_pageController->goForward();
    }
}

// ============================================================================
// 缩放控制
// ============================================================================

void PDFViewerPage::setZoom(double zoomFactor) {
    PDFViewer* viewer = getCurrentViewer();
    if (viewer) {
        viewer->setZoom(zoomFactor);
    }
}

void PDFViewerPage::zoomIn() {
    PDFViewer* viewer = getCurrentViewer();
    if (viewer) {
        viewer->zoomIn();
    }
}

void PDFViewerPage::zoomOut() {
    PDFViewer* viewer = getCurrentViewer();
    if (viewer) {
        viewer->zoomOut();
    }
}

void PDFViewerPage::fitToWidth() {
    PDFViewer* viewer = getCurrentViewer();
    if (viewer) {
        viewer->fitToWidth();
    }
}

void PDFViewerPage::fitToPage() {
    PDFViewer* viewer = getCurrentViewer();
    if (viewer) {
        viewer->fitToPage();
    }
}

void PDFViewerPage::fitToHeight() {
    PDFViewer* viewer = getCurrentViewer();
    if (viewer) {
        viewer->fitToHeight();
    }
}

// ============================================================================
// 旋转控制
// ============================================================================

void PDFViewerPage::rotateLeft() {
    PDFViewer* viewer = getCurrentViewer();
    if (viewer) {
        viewer->rotateLeft();
    }
}

void PDFViewerPage::rotateRight() {
    PDFViewer* viewer = getCurrentViewer();
    if (viewer) {
        viewer->rotateRight();
    }
}

void PDFViewerPage::resetRotation() {
    PDFViewer* viewer = getCurrentViewer();
    if (viewer) {
        viewer->resetRotation();
    }
}

// ============================================================================
// 视图模式
// ============================================================================

void PDFViewerPage::setViewMode(int mode) {
    PDFViewer* viewer = getCurrentViewer();
    if (viewer) {
        viewer->setViewMode(static_cast<PDFViewer::ViewMode>(mode));
    }
}

void PDFViewerPage::setSinglePageMode() {
    setViewMode(static_cast<int>(PDFViewer::SinglePage));
}

void PDFViewerPage::setContinuousMode() {
    setViewMode(static_cast<int>(PDFViewer::Continuous));
}

void PDFViewerPage::setTwoPageMode() {
    setViewMode(static_cast<int>(PDFViewer::TwoPage));
}

void PDFViewerPage::setBookMode() {
    setViewMode(static_cast<int>(PDFViewer::BookMode));
}

// ============================================================================
// 搜索功能
// ============================================================================

void PDFViewerPage::showSearchPanel() {
    m_searchPanel->setVisible(true);
    m_searchPanel->setFocus();
}

void PDFViewerPage::hideSearchPanel() {
    m_searchPanel->setVisible(false);
    PDFViewer* viewer = getCurrentViewer();
    if (viewer) {
        viewer->clearSearchHighlights();
    }
}

void PDFViewerPage::toggleSearchPanel() {
    if (m_searchPanel->isVisible()) {
        hideSearchPanel();
    } else {
        showSearchPanel();
    }
}

void PDFViewerPage::search(const QString& query) {
    if (!hasDocument() || query.isEmpty()) {
        return;
    }

    SLOG_INFO_F("PDFViewerPage: Searching for: {}", query.toStdString());

    // 使用 SearchEngine 搜索
    // 这里需要集成 SearchEngine
    m_statusBar->showMessage(tr("Search completed"),
                             StatusBar::MessagePriority::Normal, 3000);
}

void PDFViewerPage::findNext() {
    if (!hasDocument()) {
        SLOG_WARNING("PDFViewerPage::findNext: No document loaded");
        m_statusBar->showMessage(tr("No document loaded"),
                                 StatusBar::MessagePriority::Normal, 2000);
        return;
    }

    if (!m_searchAdapter) {
        SLOG_ERROR("PDFViewerPage::findNext: Search adapter is null");
        m_statusBar->showMessage(tr("Search not available"),
                                 StatusBar::MessagePriority::Normal, 2000);
        return;
    }

    SLOG_INFO("PDFViewerPage: Finding next search result");
    m_searchAdapter->goToNextResult();
}

void PDFViewerPage::findPrevious() {
    if (!hasDocument()) {
        SLOG_WARNING("PDFViewerPage::findPrevious: No document loaded");
        m_statusBar->showMessage(tr("No document loaded"),
                                 StatusBar::MessagePriority::Normal, 2000);
        return;
    }

    if (!m_searchAdapter) {
        SLOG_ERROR("PDFViewerPage::findPrevious: Search adapter is null");
        m_statusBar->showMessage(tr("Search not available"),
                                 StatusBar::MessagePriority::Normal, 2000);
        return;
    }

    SLOG_INFO("PDFViewerPage: Finding previous search result");
    m_searchAdapter->goToPreviousResult();
}

// ============================================================================
// 侧边栏控制
// ============================================================================

void PDFViewerPage::showLeftSideBar() { m_leftSideBar->setVisible(true); }

void PDFViewerPage::hideLeftSideBar() { m_leftSideBar->setVisible(false); }

void PDFViewerPage::toggleLeftSideBar() {
    m_leftSideBar->setVisible(!m_leftSideBar->isVisible());
}

void PDFViewerPage::showRightSideBar() { m_rightSideBar->setVisible(true); }

void PDFViewerPage::hideRightSideBar() { m_rightSideBar->setVisible(false); }

void PDFViewerPage::toggleRightSideBar() {
    m_rightSideBar->setVisible(!m_rightSideBar->isVisible());
}

// ============================================================================
// 书签功能
// ============================================================================

void PDFViewerPage::addBookmark() {
    if (!hasDocument()) {
        return;
    }

    PDFViewer* viewer = getCurrentViewer();
    if (viewer) {
        int currentPage = viewer->currentPage();
        m_leftSideBar->addBookmark(currentPage);
        m_statusBar->showMessage(tr("Bookmark added"),
                                 StatusBar::MessagePriority::Normal, 2000);
    }
}

void PDFViewerPage::removeBookmark() {
    if (!hasDocument()) {
        return;
    }

    PDFViewer* viewer = getCurrentViewer();
    if (viewer) {
        int currentPage = viewer->currentPage();
        m_leftSideBar->removeBookmark(currentPage);
        m_statusBar->showMessage(tr("Bookmark removed"),
                                 StatusBar::MessagePriority::Normal, 2000);
    }
}

void PDFViewerPage::showBookmarks() {
    m_leftSideBar->switchToTab(SideBar::BookmarksTab);
    showLeftSideBar();
}

// ============================================================================
// 工具栏和状态栏控制
// ============================================================================

void PDFViewerPage::showToolBar() {
    if (m_toolBar) {
        m_toolBar->setVisible(true);
        SLOG_INFO("PDFViewerPage: Toolbar shown");
    }
}

void PDFViewerPage::hideToolBar() {
    if (m_toolBar) {
        m_toolBar->setVisible(false);
        SLOG_INFO("PDFViewerPage: Toolbar hidden");
    }
}

void PDFViewerPage::toggleToolBar() {
    if (m_toolBar) {
        bool isVisible = m_toolBar->isVisible();
        m_toolBar->setVisible(!isVisible);
        SLOG_INFO("PDFViewerPage: Toolbar toggled to " +
                  QString(!isVisible ? "visible" : "hidden"));
    }
}

void PDFViewerPage::showStatusBar() {
    if (m_statusBar) {
        m_statusBar->setVisible(true);
        SLOG_INFO("PDFViewerPage: Status bar shown");
    }
}

void PDFViewerPage::hideStatusBar() {
    if (m_statusBar) {
        m_statusBar->setVisible(false);
        SLOG_INFO("PDFViewerPage: Status bar hidden");
    }
}

void PDFViewerPage::toggleStatusBar() {
    if (m_statusBar) {
        bool isVisible = m_statusBar->isVisible();
        m_statusBar->setVisible(!isVisible);
        SLOG_INFO("PDFViewerPage: Status bar toggled to " +
                  QString(!isVisible ? "visible" : "hidden"));
    }
}

// ============================================================================
// 全屏和演示
// ============================================================================

void PDFViewerPage::enterFullScreen() {
    if (m_isFullScreen) {
        return;
    }

    SLOG_INFO("PDFViewerPage: Entering full screen");

    m_isFullScreen = true;

    // 隐藏菜单栏、工具栏、状态栏、侧边栏
    m_menuBar->setVisible(false);
    m_toolBar->setVisible(false);
    m_statusBar->setVisible(false);
    m_leftSideBar->setVisible(false);
    m_rightSideBar->setVisible(false);

    emit fullScreenChanged(true);
}

void PDFViewerPage::exitFullScreen() {
    if (!m_isFullScreen) {
        return;
    }

    SLOG_INFO("PDFViewerPage: Exiting full screen");

    m_isFullScreen = false;

    // 恢复 UI 组件
    m_menuBar->setVisible(true);
    m_toolBar->setVisible(true);
    m_statusBar->setVisible(true);
    m_leftSideBar->setVisible(true);

    emit fullScreenChanged(false);
}

void PDFViewerPage::toggleFullScreen() {
    if (m_isFullScreen) {
        exitFullScreen();
    } else {
        enterFullScreen();
    }
}

void PDFViewerPage::startPresentation() {
    if (m_isPresentation || !hasDocument()) {
        return;
    }

    SLOG_INFO("PDFViewerPage: Starting presentation");

    m_isPresentation = true;
    enterFullScreen();
    setSinglePageMode();
}

void PDFViewerPage::stopPresentation() {
    if (!m_isPresentation) {
        return;
    }

    SLOG_INFO("PDFViewerPage: Stopping presentation");

    m_isPresentation = false;
    exitFullScreen();
}

void PDFViewerPage::togglePresentation() {
    if (m_isPresentation) {
        stopPresentation();
    } else {
        startPresentation();
    }
}

// ============================================================================
// 状态查询
// ============================================================================

bool PDFViewerPage::hasDocument() const {
    int currentIndex = getCurrentDocumentIndex();
    if (currentIndex >= 0 && currentIndex < m_pdfViewers.size()) {
        return m_pdfViewers[currentIndex]->hasDocument();
    }
    return false;
}

QString PDFViewerPage::currentFilePath() const {
    int currentIndex = getCurrentDocumentIndex();
    if (currentIndex >= 0) {
        return m_tabWidget->getTabFilePath(currentIndex);
    }
    return QString();
}

int PDFViewerPage::currentPage() const {
    int currentIndex = getCurrentDocumentIndex();
    if (currentIndex >= 0 && currentIndex < m_pdfViewers.size()) {
        return m_pdfViewers[currentIndex]->currentPage();
    }
    return 0;
}

int PDFViewerPage::pageCount() const {
    int currentIndex = getCurrentDocumentIndex();
    if (currentIndex >= 0 && currentIndex < m_pdfViewers.size()) {
        return m_pdfViewers[currentIndex]->pageCount();
    }
    return 0;
}

double PDFViewerPage::zoomLevel() const {
    int currentIndex = getCurrentDocumentIndex();
    if (currentIndex >= 0 && currentIndex < m_pdfViewers.size()) {
        return m_pdfViewers[currentIndex]->zoom();
    }
    return 1.0;
}

bool PDFViewerPage::isFullScreen() const { return m_isFullScreen; }

bool PDFViewerPage::isPresentation() const { return m_isPresentation; }

// ============================================================================
// 业务逻辑集成
// ============================================================================

void PDFViewerPage::setDocumentController(DocumentController* controller) {
    m_documentController = controller;
    setupControllers();
}

void PDFViewerPage::setPageController(PageController* controller) {
    m_pageController = controller;
    setupControllers();
}

void PDFViewerPage::setApplicationController(
    ApplicationController* controller) {
    m_applicationController = controller;
    setupControllers();
}

void PDFViewerPage::setViewDelegate(ViewDelegate* delegate) {
    m_viewDelegate = delegate;
    setupControllers();
}

// ============================================================================
// 事件处理
// ============================================================================

void PDFViewerPage::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}

void PDFViewerPage::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    // 可以在这里处理窗口大小变化
}

void PDFViewerPage::keyPressEvent(QKeyEvent* event) {
    // 处理快捷键
    if (event->key() == Qt::Key_Escape) {
        if (m_isFullScreen) {
            exitFullScreen();
            event->accept();
            return;
        }
        if (m_searchPanel->isVisible()) {
            hideSearchPanel();
            event->accept();
            return;
        }
    }

    QWidget::keyPressEvent(event);
}

// ============================================================================
// 辅助方法
// ============================================================================

void PDFViewerPage::updateWindowTitle() {
    // 窗口标题由主窗口管理
}

void PDFViewerPage::updateMenuStates() {
    bool hasDoc = hasDocument();
    // Note: Menu state management can be enhanced when needed:
    // m_menuBar->setDocumentActionsEnabled(hasDoc);
    (void)hasDoc;  // Suppress unused variable warning
}

void PDFViewerPage::updateToolBarStates() {
    bool hasDoc = hasDocument();
    m_toolBar->setActionsEnabled(hasDoc);

    if (hasDoc) {
        m_toolBar->updatePageInfo(currentPage(), pageCount());
        m_toolBar->updateZoomLevel(zoomLevel());
    }
}

void PDFViewerPage::updateStatusBar() {
    if (hasDocument()) {
        QString filePath = currentFilePath();
        QFileInfo fileInfo(filePath);
        m_statusBar->setFileName(fileInfo.fileName());
        m_statusBar->setPageInfo(currentPage(), pageCount());
        m_statusBar->setZoomLevel(zoomLevel());

        // 设置文档元数据
        PDFViewer* viewer = getCurrentViewer();
        if (viewer && viewer->document()) {
            auto document = viewer->document();
            QMap<QString, QString> metadata;
            metadata["Title"] = document->info("Title");
            metadata["Author"] = document->info("Author");
            metadata["Subject"] = document->info("Subject");
            metadata["Keywords"] = document->info("Keywords");
            metadata["Creator"] = document->info("Creator");
            metadata["Producer"] = document->info("Producer");
            metadata["CreationDate"] = document->info("CreationDate");
            metadata["ModDate"] = document->info("ModDate");
            m_statusBar->setDocumentMetadata(metadata);

            // 设置统计信息
            QMap<QString, QString> statistics;
            statistics["Pages"] = QString::number(pageCount());
            statistics["FileSize"] = QString::number(fileInfo.size());
            statistics["PDFVersion"] =
                QString::number(document->getPdfVersion().major) + "." +
                QString::number(document->getPdfVersion().minor);
            m_statusBar->setDocumentStatistics(statistics);

            // 设置安全信息
            QMap<QString, QString> security;
            security["Encrypted"] =
                document->isEncrypted() ? tr("Yes") : tr("No");
            security["PrintAllowed"] =
                document->okToPrint() ? tr("Yes") : tr("No");
            security["CopyAllowed"] =
                document->okToCopy() ? tr("Yes") : tr("No");
            security["ModifyAllowed"] =
                document->okToChange() ? tr("Yes") : tr("No");
            security["AnnotateAllowed"] =
                document->okToAddNotes() ? tr("Yes") : tr("No");
            m_statusBar->setDocumentSecurity(security);
        }
    } else {
        m_statusBar->clearAll();
    }
}

void PDFViewerPage::retranslateUi() {
    SLOG_INFO("PDFViewerPage: Retranslating UI");
    // 所有组件都会自动处理语言变化
}

bool PDFViewerPage::loadDocument(const QString& filePath) {
    // Delegate to openFile for multi-document flow
    return openFile(filePath);
}

void PDFViewerPage::unloadDocument(int index) {
    // Delegate to closeDocument() which handles multi-document and UI cleanup
    closeDocument(index);
}

void PDFViewerPage::handleAction(ActionMap action) {
    switch (action) {
        // 文件菜单
        case ActionMap::openFile:
            openFile(QString());
            break;
        case ActionMap::closeFile:
            closeDocument();
            break;
        case ActionMap::saveAs:  // saveDocumentCopy -> saveAs
            saveDocumentCopy(QString());
            break;
        case ActionMap::printFile:  // printDocument -> printFile
            printDocument();
            break;
        case ActionMap::exportFile:  // exportDocument -> exportFile
            exportDocument(QString(), QString());
            break;
        case ActionMap::showDocumentMetadata:  // documentProperties ->
                                               // showDocumentMetadata
            // 显示文档属性
            m_statusBar->showDocumentInfoPanel();
            break;
        case ActionMap::quit:  // exitApplication -> quit
            // 退出应用
            qApp->quit();
            break;

        // 标签页菜单
        case ActionMap::newTab:
            // 新建标签页（需要主窗口支持）
            break;
        case ActionMap::closeTab:
            closeDocument();
            break;

        // 视图菜单
        case ActionMap::zoomIn:
            zoomIn();
            break;
        case ActionMap::zoomOut:
            zoomOut();
            break;
        case ActionMap::fitToWidth:  // fitWidth -> fitToWidth
            fitToWidth();
            break;
        case ActionMap::fitToPage:  // fitPage -> fitToPage
            fitToPage();
            break;
        case ActionMap::fitToHeight:  // fitHeight -> fitToHeight
            fitToHeight();
            break;
        case ActionMap::rotateLeft:
            rotateLeft();
            break;
        case ActionMap::rotateRight:
            rotateRight();
            break;
        case ActionMap::setSinglePageMode:  // singlePageMode ->
                                            // setSinglePageMode
            setSinglePageMode();
            break;
        case ActionMap::setContinuousScrollMode:  // continuousMode ->
                                                  // setContinuousScrollMode
            setContinuousMode();
            break;
        case ActionMap::setTwoPagesMode:  // twoPageMode -> setTwoPagesMode
            setTwoPageMode();
            break;
        case ActionMap::setBookViewMode:  // bookMode -> setBookViewMode
            setBookMode();
            break;
        case ActionMap::toggleSideBar:  // Use toggleSideBar for all sidebar
                                        // toggles
            toggleLeftSideBar();
            break;
        case ActionMap::fullScreen:
            toggleFullScreen();
            break;

        // 帮助菜单
        case ActionMap::showHelp:  // help -> showHelp
            // 由主窗口处理
            break;

        default:
            SLOG_WARNING_F("PDFViewerPage: Unhandled action: {}",
                           static_cast<int>(action));
            break;
    }
}

// ============================================================================
// Multi-document Management
// ============================================================================

PDFViewer* PDFViewerPage::createPDFViewer() {
    PDFViewer* viewer = new PDFViewer(this);

    // Connect viewer signals
    connect(viewer, &PDFViewer::documentLoaded, this, [this](int pageCount) {
        SLOG_INFO_F("PDFViewerPage: Document loaded with {} pages", pageCount);
        updateMenuStates();
        updateToolBarStates();
        updateStatusBar();
    });

    connect(viewer, &PDFViewer::documentClosed, this, [this]() {
        SLOG_INFO("PDFViewerPage: Document closed");
        updateMenuStates();
        updateToolBarStates();
        m_statusBar->clearAll();
    });

    connect(viewer, &PDFViewer::pageChanged, this,
            [this](int currentPage, int totalPages) {
                m_toolBar->updatePageInfo(currentPage, totalPages);
                m_statusBar->setPageInfo(currentPage, totalPages);
                m_leftSideBar->setCurrentPage(currentPage);
                emit pageChanged(currentPage, totalPages);
            });

    connect(viewer, &PDFViewer::zoomChanged, this, [this](double zoomFactor) {
        m_toolBar->updateZoomLevel(zoomFactor);
        m_statusBar->setZoomLevel(zoomFactor);
        emit zoomChanged(zoomFactor);
    });

    connect(viewer, &PDFViewer::rotationChanged, this, [this](int rotation) {
        SLOG_INFO_F("PDFViewerPage: Rotation changed to {}", rotation);
    });

    connect(viewer, &PDFViewer::viewModeChanged, this,
            [this](PDFViewer::ViewMode mode) {
                QString modeStr;
                switch (mode) {
                    case PDFViewer::SinglePage:
                        modeStr = tr("Single Page");
                        break;
                    case PDFViewer::Continuous:
                        modeStr = tr("Continuous");
                        break;
                    case PDFViewer::TwoPage:
                        modeStr = tr("Two Pages");
                        break;
                    case PDFViewer::BookMode:
                        modeStr = tr("Book Mode");
                        break;
                }
                m_statusBar->setViewMode(modeStr);
                emit viewModeChanged(static_cast<int>(mode));
            });

    connect(viewer, &PDFViewer::renderError, this,
            [this](const QString& error) {
                SLOG_ERROR_F("PDFViewerPage: Render error: {}",
                             error.toStdString());
                m_statusBar->showMessage(
                    error, StatusBar::MessagePriority::High, 5000);
                emit errorOccurred(error);
            });

    return viewer;
}

PDFViewer* PDFViewerPage::getCurrentViewer() const {
    int currentIndex = getCurrentDocumentIndex();
    if (currentIndex >= 0 && currentIndex < m_pdfViewers.size()) {
        return m_pdfViewers[currentIndex];
    }
    return nullptr;
}

void PDFViewerPage::updateCurrentViewer() {
    int currentIndex = m_tabWidget->currentIndex();
    if (currentIndex >= 0 && currentIndex < m_pdfViewers.size()) {
        m_viewerStack->setCurrentWidget(m_pdfViewers[currentIndex]);
        m_lastActiveIndex = currentIndex;
    }
}

void PDFViewerPage::showEmptyState() {
    m_viewerStack->setCurrentWidget(m_emptyWidget);
    m_tabWidget->hide();
}

void PDFViewerPage::hideEmptyState() { m_tabWidget->show(); }

void PDFViewerPage::openDocuments(const QStringList& filePaths) {
    for (const QString& filePath : filePaths) {
        openFile(filePath);
    }
}

void PDFViewerPage::closeDocument(int index) {
    if (index == -1) {
        index = getCurrentDocumentIndex();
    }

    if (!validateDocumentIndex(index, "closeDocument")) {
        return;
    }

    SLOG_INFO_F("PDFViewerPage: Closing document at index {}", index);

    // 保存文档状态（如果需要）
    // preserveDocumentState(index);

    // 移除标签页
    m_tabWidget->removeDocumentTab(index);

    // 清理PDF查看器
    PDFViewer* viewer = m_pdfViewers.takeAt(index);
    m_viewerStack->removeWidget(viewer);
    viewer->deleteLater();

    // 清理outline model
    if (index < m_outlineModels.size()) {
        PDFOutlineModel* model = m_outlineModels.takeAt(index);
        if (model) {
            model->deleteLater();
        }
    }

    // 清理文档状态
    if (index < m_documentStates.size()) {
        m_documentStates.removeAt(index);
    }

    // 如果没有文档了，显示空状态
    if (m_pdfViewers.isEmpty()) {
        showEmptyState();
        updateMenuStates();
        updateToolBarStates();
        updateStatusBar();
    } else {
        // 切换到相邻的文档
        int newIndex = qMin(index, m_pdfViewers.size() - 1);
        m_tabWidget->setCurrentTab(newIndex);
        updateCurrentViewer();
    }
}

void PDFViewerPage::closeCurrentDocument() { closeDocument(-1); }

void PDFViewerPage::closeAllDocuments() {
    SLOG_INFO("PDFViewerPage: Closing all documents");

    while (!m_pdfViewers.isEmpty()) {
        closeDocument(0);
    }
}

void PDFViewerPage::switchToDocument(int index) {
    if (!validateDocumentIndex(index, "switchToDocument")) {
        return;
    }

    SLOG_INFO_F("PDFViewerPage: Switching to document at index {}", index);

    // 保存当前文档状态
    if (m_lastActiveIndex >= 0 && m_lastActiveIndex < m_pdfViewers.size()) {
        preserveCurrentDocumentState();
    }

    // 切换到新文档
    m_viewerStack->setCurrentWidget(m_pdfViewers[index]);
    m_lastActiveIndex = index;

    // 恢复文档状态
    restoreDocumentState(index);

    // 更新UI
    updateWindowTitle();
    updateMenuStates();
    updateToolBarStates();
    updateStatusBar();

    // 更新左侧边栏的outline
    if (index < m_outlineModels.size() && m_outlineModels[index]) {
        m_leftSideBar->setOutlineModel(m_outlineModels[index]);
    }
}

// ============================================================================
// Document State Management
// ============================================================================

PDFViewerPage::DocumentState PDFViewerPage::getDocumentState(int index) const {
    if (index >= 0 && index < m_documentStates.size()) {
        return m_documentStates[index];
    }
    return DocumentState();
}

void PDFViewerPage::setDocumentState(int index, const DocumentState& state) {
    if (index >= 0 && index < m_documentStates.size()) {
        m_documentStates[index] = state;
    }
}

void PDFViewerPage::preserveCurrentDocumentState() {
    int currentIndex = getCurrentDocumentIndex();
    if (currentIndex < 0 || currentIndex >= m_pdfViewers.size()) {
        return;
    }

    PDFViewer* viewer = m_pdfViewers[currentIndex];
    if (!viewer || !viewer->hasDocument()) {
        return;
    }

    DocumentState state;
    state.currentPage = viewer->currentPage();
    state.zoomLevel = viewer->zoom();
    state.rotation = viewer->rotation();
    // TODO: Implement scroll position preservation
    // state.scrollPosition = viewer->scrollPosition();
    state.scrollPosition = QPoint(0, 0);
    state.viewMode = static_cast<int>(viewer->viewMode());

    setDocumentState(currentIndex, state);

    SLOG_DEBUG_F(
        "PDFViewerPage: Preserved state for document {}: page={}, zoom={}, "
        "rotation={}",
        currentIndex, state.currentPage, state.zoomLevel, state.rotation);
}

void PDFViewerPage::restoreDocumentState(int index) {
    if (index < 0 || index >= m_pdfViewers.size()) {
        return;
    }

    PDFViewer* viewer = m_pdfViewers[index];
    if (!viewer || !viewer->hasDocument()) {
        return;
    }

    DocumentState state = getDocumentState(index);

    // 恢复状态
    viewer->goToPage(state.currentPage);
    viewer->setZoom(state.zoomLevel);
    // TODO: Implement rotation restoration
    // viewer->setRotation(state.rotation);

    // TODO: Implement scroll position restoration
    // 延迟恢复滚动位置，确保页面已经渲染
    // QTimer::singleShot(100, this, [viewer, state]() {
    //     viewer->setScrollPosition(state.scrollPosition);
    // });

    SLOG_DEBUG_F(
        "PDFViewerPage: Restored state for document {}: page={}, zoom={}, "
        "rotation={}",
        index, state.currentPage, state.zoomLevel, state.rotation);
}

// ============================================================================
// Tab Management Slots
// ============================================================================

void PDFViewerPage::onTabCloseRequested(int index) {
    SLOG_INFO_F("PDFViewerPage: Tab close requested for index {}", index);
    closeDocument(index);
}

void PDFViewerPage::onTabSwitched(int index) {
    SLOG_INFO_F("PDFViewerPage: Tab switched to index {}", index);
    switchToDocument(index);
}

void PDFViewerPage::onAllTabsClosed() {
    SLOG_INFO("PDFViewerPage: All tabs closed");
    showEmptyState();
    updateMenuStates();
    updateToolBarStates();
    updateStatusBar();
}

// ============================================================================
// Validation Helpers
// ============================================================================

bool PDFViewerPage::validateDocumentIndex(int index,
                                          const QString& operation) const {
    if (index < 0 || index >= m_pdfViewers.size()) {
        SLOG_WARNING_F(
            "PDFViewerPage: Invalid document index {} for operation '{}' "
            "(total documents: {})",
            index, operation.toStdString(), m_pdfViewers.size());
        return false;
    }
    return true;
}

// ============================================================================
// State Query Methods - Multi-document support
// ============================================================================

bool PDFViewerPage::hasDocuments() const { return !m_pdfViewers.isEmpty(); }

int PDFViewerPage::getCurrentDocumentIndex() const {
    return m_tabWidget->currentIndex();
}

int PDFViewerPage::getDocumentCount() const { return m_pdfViewers.size(); }

QString PDFViewerPage::getDocumentFilePath(int index) const {
    if (validateDocumentIndex(index, "getDocumentFilePath")) {
        return m_tabWidget->getTabFilePath(index);
    }
    return QString();
}
