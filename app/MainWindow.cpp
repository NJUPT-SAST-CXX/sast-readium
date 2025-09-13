#include "MainWindow.h"
#include <QApplication>
#include <QBoxLayout>
#include <QFile>
#include <QFileInfo>
#include <QFrame>
#include <QLabel>
#include <QLatin1String>
#include <QMainWindow>
#include <QStackedWidget>
#include <QStringList>
#include <QWidget>
#include "managers/StyleManager.h"
#include "managers/FileTypeIconManager.h"
#include "model/RenderModel.h"
#include "ui/widgets/WelcomeWidget.h"
#include "ui/managers/WelcomeScreenManager.h"
#include "logging/SimpleLogging.h"


MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), 
                                         m_logger("MainWindow") {
    SLOG_TIMER("MainWindow::Constructor");
    
    m_logger.debug("Starting initialization...");
    
    // Initialize with StyleManager's default theme to maintain consistency
    QString defaultTheme =
        (STYLE.currentTheme() == Theme::Light) ? "light" : "dark";
    applyTheme(defaultTheme);
    m_logger.debug("Theme applied successfully: " + defaultTheme);

    initWindow();
    m_logger.debug("Window initialized");
    
    initModel();
    m_logger.debug("Models initialized");
    
    initController();
    m_logger.debug("Controllers initialized");
    
    initWelcomeScreen();
    m_logger.debug("Welcome screen initialized");
    
    initContent();
    m_logger.debug("Content initialized");

    initConnection();
    m_logger.debug("Connections initialized");
    
    initWelcomeScreenConnections();
    m_logger.debug("Welcome screen connections initialized");

    // 启动异步初始化以避免阻塞UI
    if (recentFilesManager) {
        try {
            recentFilesManager->initializeAsync();
            m_logger.debug("Async initialization started");
        } catch (const std::exception& e) {
            m_logger.warning("Failed to start async initialization: " + 
                           QString::fromStdString(e.what()));
        } catch (...) {
            m_logger.warning("Unknown error during async initialization startup");
        }
    } else {
        m_logger.warning("RecentFilesManager is null, skipping async initialization");
    }

    m_logger.info("Initialization completed successfully");
}

MainWindow::~MainWindow() noexcept {}

// initialize
void MainWindow::initWindow() { resize(1280, 800); }

void MainWindow::initContent() {
    WidgetFactory* factory = new WidgetFactory(pageController, this);

    menuBar = new MenuBar(this);
    toolBar = new ToolBar(this);
    sideBar = new SideBar(this);
    rightSideBar = new RightSideBar(this);
    statusBar = new StatusBar(factory, this);
    viewWidget = new ViewWidget(this);

    // 设置菜单栏的最近文件管理器
    menuBar->setRecentFilesManager(recentFilesManager);

    // 设置ViewWidget的控制器和模型
    viewWidget->setDocumentController(documentController);
    viewWidget->setDocumentModel(documentModel);

    setMenuBar(menuBar);
    addToolBar(toolBar);
    setStatusBar(statusBar);

    // 创建主内容区域的QStackedWidget
    m_contentStack = new QStackedWidget(this);

    // 创建主PDF查看器区域（包含侧边栏和视图）
    QWidget* mainViewerWidget = new QWidget();
    QHBoxLayout* mainViewerLayout = new QHBoxLayout(mainViewerWidget);
    mainViewerLayout->setContentsMargins(0, 0, 0, 0);

    mainSplitter = new QSplitter(Qt::Horizontal, mainViewerWidget);
    mainSplitter->addWidget(sideBar);
    mainSplitter->addWidget(viewWidget);
    mainSplitter->addWidget(rightSideBar);
    mainSplitter->setCollapsible(0, true);  // Left sidebar collapsible
    mainSplitter->setCollapsible(1, false); // ViewWidget not collapsible
    mainSplitter->setCollapsible(2, true);  // Right sidebar collapsible
    mainSplitter->setStretchFactor(1, 1);   // ViewWidget gets stretch priority

    // 根据侧边栏的可见性设置初始尺寸
    int leftWidth = sideBar->isVisible() ? sideBar->getPreferredWidth() : 0;
    int rightWidth = rightSideBar->isVisible() ? rightSideBar->getPreferredWidth() : 0;
    mainSplitter->setSizes({leftWidth, 1000, rightWidth});

    mainViewerLayout->addWidget(mainSplitter);

    // 添加页面到堆叠组件
    m_contentStack->addWidget(m_welcomeWidget);      // 索引 0: 欢迎界面
    m_contentStack->addWidget(mainViewerWidget);     // 索引 1: 主查看器

    // 设置中央组件
    setCentralWidget(m_contentStack);

    // 初始显示欢迎界面（如果启用）
    if (m_welcomeScreenManager && m_welcomeScreenManager->shouldShowWelcomeScreen()) {
        m_contentStack->setCurrentIndex(0);
    } else {
        m_contentStack->setCurrentIndex(1);
    }
}

void MainWindow::initModel() {
    renderModel = new RenderModel(this->logicalDpiX(), this->logicalDpiY());
    documentModel = new DocumentModel(renderModel);
    pageModel = new PageModel(renderModel);
    recentFilesManager = new RecentFilesManager(this);
}

void MainWindow::initController() {
    documentController = new DocumentController(documentModel);
    pageController = new PageController(pageModel);

    // 设置最近文件管理器
    documentController->setRecentFilesManager(recentFilesManager);
}

void MainWindow::initWelcomeScreen() {
    m_logger.debug("Initializing welcome screen...");

    // 初始化文件类型图标管理器
    FILE_ICON_MANAGER.preloadIcons();

    // 创建欢迎界面组件
    m_welcomeWidget = new WelcomeWidget(this);
    m_welcomeWidget->setRecentFilesManager(recentFilesManager);

    // 创建欢迎界面管理器
    m_welcomeScreenManager = new WelcomeScreenManager(this);
    m_welcomeScreenManager->setMainWindow(this);
    m_welcomeScreenManager->setWelcomeWidget(m_welcomeWidget);
    m_welcomeScreenManager->setDocumentModel(documentModel);

    // 设置管理器到欢迎界面
    m_welcomeWidget->setWelcomeScreenManager(m_welcomeScreenManager);

    // 应用主题
    m_welcomeWidget->applyTheme();

    m_logger.debug("Welcome screen initialized successfully");
}

void MainWindow::initConnection() {
    connect(menuBar, &MenuBar::themeChanged, this, &MainWindow::applyTheme);

    connect(menuBar, &MenuBar::onExecuted, documentController,
            &DocumentController::execute);
    connect(menuBar, &MenuBar::onExecuted, this,
            &MainWindow::handleActionExecuted);

    // 连接最近文件信号
    connect(menuBar, &MenuBar::openRecentFileRequested, this,
            &MainWindow::onOpenRecentFileRequested);

    // 连接工具栏信号
    connect(toolBar, &ToolBar::actionTriggered, this, [this](ActionMap action) {
        documentController->execute(action, this);
    });
    connect(toolBar, &ToolBar::pageJumpRequested, this,
            &MainWindow::onPageJumpRequested);

    // 连接文档控制器的操作完成信号
    connect(documentController, &DocumentController::documentOperationCompleted,
            this, &MainWindow::onDocumentOperationCompleted);

    // 连接文档控制器的侧边栏信号
    connect(documentController, &DocumentController::sideBarToggleRequested,
            sideBar, [this]() { sideBar->toggleVisibility(true); });
    connect(documentController, &DocumentController::sideBarShowRequested,
            sideBar, [this]() { sideBar->show(true); });
    connect(documentController, &DocumentController::sideBarHideRequested,
            sideBar, [this]() { sideBar->hide(true); });

    // 连接侧边栏信号
    connect(sideBar, &SideBar::visibilityChanged, this,
            &MainWindow::onSideBarVisibilityChanged);

    // 连接缩略图点击信号
    connect(sideBar, &SideBar::pageClicked, this,
            &MainWindow::onThumbnailPageClicked);
    connect(sideBar, &SideBar::pageDoubleClicked, this,
            &MainWindow::onThumbnailPageDoubleClicked);

    // 连接分隔器信号
    connect(mainSplitter, &QSplitter::splitterMoved, this,
            &MainWindow::onSplitterMoved);

    // 连接文档模型信号以同步目录
    connect(documentModel, &DocumentModel::currentDocumentChanged, this,
            &MainWindow::onCurrentDocumentChangedForOutline);

    // 连接文档模型信号以更新状态栏
    connect(documentModel, &DocumentModel::documentOpened, this,
            [this](int index, const QString& fileName) {
                statusBar->hideLoadingProgress();
                updateStatusBarInfo();
            });
    connect(documentModel, &DocumentModel::currentDocumentChanged, this,
            &MainWindow::updateStatusBarInfo);
    connect(documentModel, &DocumentModel::allDocumentsClosed, this, [this]() {
        statusBar->clearDocumentInfo();
        toolBar->setActionsEnabled(false);
    });

    // 连接异步加载进度信号
    connect(documentModel, &DocumentModel::loadingStarted, this,
            [this](const QString& filePath) {
                QFileInfo fileInfo(filePath);
                statusBar->showLoadingProgress(
                    QString("正在加载 %1...").arg(fileInfo.fileName()));
            });
    connect(documentModel, &DocumentModel::loadingProgressChanged, statusBar,
            &StatusBar::updateLoadingProgress);
    connect(documentModel, &DocumentModel::loadingMessageChanged, statusBar,
            &StatusBar::setLoadingMessage);
    connect(documentModel, &DocumentModel::loadingFailed, this,
            [this](const QString& error, const QString& filePath) {
                statusBar->hideLoadingProgress();
                statusBar->setMessage(QString("加载失败: %1").arg(error));
            });

    // 连接文档打开/关闭状态变化
    connect(documentModel, &DocumentModel::documentOpened, this,
            [this](int, const QString&) {
                toolBar->setActionsEnabled(true);
                // 通知欢迎界面管理器文档已打开
                if (m_welcomeScreenManager) {
                    m_welcomeScreenManager->onDocumentOpened();
                }
            });
    connect(documentModel, &DocumentModel::documentClosed, this, [this](int) {
        if (documentModel->isEmpty()) {
            toolBar->setActionsEnabled(false);
            // 通知欢迎界面管理器所有文档已关闭
            if (m_welcomeScreenManager) {
                m_welcomeScreenManager->onAllDocumentsClosed();
            }
        } else {
            // 通知欢迎界面管理器文档已关闭
            if (m_welcomeScreenManager) {
                m_welcomeScreenManager->onDocumentClosed();
            }
        }
    });

    // 连接ViewWidget的PDF查看器状态信号
    connect(viewWidget, &ViewWidget::currentViewerPageChanged, this,
            [this](int pageNumber, int totalPages) {
                statusBar->setPageInfo(pageNumber, totalPages);
                toolBar->updatePageInfo(pageNumber, totalPages);
            });
    connect(viewWidget, &ViewWidget::currentViewerZoomChanged, this,
            [this](double zoomFactor) {
                statusBar->setZoomLevel(zoomFactor);
                toolBar->updateZoomLevel(zoomFactor);
            });

    // 连接查看模式变化信号
    connect(documentController, &DocumentController::viewModeChangeRequested,
            this, &MainWindow::onViewModeChangeRequested);

    // 连接PDF操作信号
    connect(documentController, &DocumentController::pdfActionRequested, this,
            &MainWindow::onPDFActionRequested);

    // 连接主题切换信号
    connect(documentController, &DocumentController::themeToggleRequested, this,
            &MainWindow::onThemeToggleRequested);

    // 连接MainWindow的PDF操作信号到ViewWidget
    connect(this, &MainWindow::pdfViewerActionRequested, viewWidget,
            &ViewWidget::executePDFAction);

    // 连接状态栏页码跳转信号
    connect(statusBar, &StatusBar::pageJumpRequested, this,
            &MainWindow::onPageJumpRequested);

    // 添加RenderModel连接（从合并分支集成）
    connect(renderModel, &RenderModel::renderPageDone, viewWidget,
            &ViewWidget::onRenderPageDone);
    connect(renderModel, &RenderModel::documentChanged, pageModel,
            &PageModel::updateInfo);
    connect(pageModel, &PageModel::pageUpdate, statusBar,
            &StatusBar::setPageInfo);
    connect(documentModel, &DocumentModel::pageUpdate, statusBar,
            &StatusBar::setPageInfo);
    connect(viewWidget, &ViewWidget::scaleChanged, statusBar,
            static_cast<void(StatusBar::*)(double)>(&StatusBar::setZoomLevel));
}

void MainWindow::onDocumentOperationCompleted(ActionMap action, bool success) {
    QString message;
    switch (action) {
        case ActionMap::openFile:
        case ActionMap::newTab:
            message = success ? "文档打开成功" : "文档打开失败";
            break;
        case ActionMap::closeTab:
        case ActionMap::closeCurrentTab:
            message = success ? "文档关闭成功" : "文档关闭失败";
            break;
        case ActionMap::closeAllTabs:
            message = success ? "所有文档已关闭" : "关闭文档时出错";
            break;
        default:
            return;
    }

    statusBar->setMessage(message);
}

void MainWindow::onSideBarVisibilityChanged(bool visible) {
    // 可以在这里添加状态栏消息或其他UI反馈
    QString message = visible ? "侧边栏已显示" : "侧边栏已隐藏";
    statusBar->setMessage(message);
}

void MainWindow::onSplitterMoved(int pos, int index) {
    // 当用户拖拽分隔器时，更新侧边栏的首选宽度
    if (index == 0 && sideBar->isVisible()) {
        QList<int> sizes = mainSplitter->sizes();
        if (!sizes.isEmpty()) {
            int newWidth = sizes[0];
            if (newWidth > 0) {
                sideBar->setPreferredWidth(newWidth);
                sideBar->saveState();  // 保存新的宽度设置
            }
        }
    }
}

void MainWindow::onCurrentDocumentChangedForOutline(int index) {
    // 获取当前文档的目录模型并设置到侧边栏
    PDFOutlineModel* currentOutlineModel = viewWidget->getCurrentOutlineModel();
    sideBar->setOutlineModel(currentOutlineModel);

    // 设置缩略图文档
    if (documentModel && index >= 0) {
        Poppler::Document* document = documentModel->getDocument(index);
        if (document) {
            // 创建shared_ptr包装
            std::shared_ptr<Poppler::Document> sharedDoc(
                document, [](Poppler::Document*) {
                    // 不删除，因为DocumentModel管理生命周期
                });
            sideBar->setDocument(sharedDoc);
        }
    }

    // 连接目录点击跳转信号
    if (sideBar->getOutlineWidget()) {
        // 断开之前的连接
        disconnect(sideBar->getOutlineWidget(),
                   &PDFOutlineWidget::pageNavigationRequested, nullptr,
                   nullptr);

        // 连接到当前PDF查看器的页面跳转
        connect(sideBar->getOutlineWidget(),
                &PDFOutlineWidget::pageNavigationRequested, this,
                [this](int pageNumber) {
                    // 通过ViewWidget获取当前的PDF查看器并跳转页面
                    viewWidget->goToPage(pageNumber);
                });
    }
}

void MainWindow::onThumbnailPageClicked(int pageNumber) {
    // 缩略图单击跳转到对应页面
    if (viewWidget) {
        viewWidget->goToPage(pageNumber);
    }

    // 可选：显示状态消息
    if (statusBar) {
        statusBar->setMessage(QString("跳转到第 %1 页").arg(pageNumber + 1));
    }
}

void MainWindow::onThumbnailPageDoubleClicked(int pageNumber) {
    // 缩略图双击可以有不同的行为，比如放大显示
    onThumbnailPageClicked(pageNumber);

    // 可选：触发特殊操作，如适合页面宽度
    // 这里可以添加更多功能
}

void MainWindow::updateStatusBarInfo() {
    if (!documentModel || documentModel->isEmpty()) {
        statusBar->clearDocumentInfo();
        return;
    }

    // 获取当前文档信息
    QString fileName = documentModel->getCurrentFileName();

    // 获取当前PDF查看器的页面和缩放信息
    int currentPage = viewWidget->getCurrentPage();
    int totalPages = viewWidget->getCurrentPageCount();
    double zoomLevel = viewWidget->getCurrentZoom();

    statusBar->setDocumentInfo(fileName, currentPage, totalPages, zoomLevel);
}

void MainWindow::onViewModeChangeRequested(int mode) {
    // 将查看模式变化请求传递给当前的PDF查看器
    viewWidget->setCurrentViewMode(mode);
}

void MainWindow::onPageJumpRequested(int pageNumber) {
    // 将页码跳转请求传递给当前的PDF查看器
    viewWidget->goToPage(pageNumber);
}

void MainWindow::onPDFActionRequested(ActionMap action) {
    // 获取当前活动的PDF查看器并执行相应操作
    if (!viewWidget->hasDocuments()) {
        return;  // 没有文档时不执行操作
    }

    int currentIndex = viewWidget->getCurrentDocumentIndex();
    if (currentIndex < 0)
        return;

    // 通过ViewWidget路由到当前PDFViewer
    switch (action) {
        case ActionMap::firstPage:
        case ActionMap::previousPage:
        case ActionMap::nextPage:
        case ActionMap::lastPage:
        case ActionMap::zoomIn:
        case ActionMap::zoomOut:
        case ActionMap::fitToWidth:
        case ActionMap::fitToPage:
        case ActionMap::fitToHeight:
        case ActionMap::rotateLeft:
        case ActionMap::rotateRight:
            emit pdfViewerActionRequested(action);
            break;
        default:
            m_logger.warning(QString("Unhandled PDF action: %1").arg(static_cast<int>(action)));
            break;
    }
}

void MainWindow::onThemeToggleRequested() {
    // 切换主题
    QString currentTheme =
        (STYLE.currentTheme() == Theme::Light) ? "dark" : "light";
    applyTheme(currentTheme);
}

void MainWindow::onOpenRecentFileRequested(const QString& filePath) {
    // 通过DocumentController打开最近文件
    if (documentController) {
        bool success = documentController->openDocument(filePath);
        if (!success) {
            m_logger.warning("Failed to open recent file: " + filePath);
        }
    }
}

// function
void MainWindow::applyTheme(const QString& theme) {
    // 防止重复应用相同主题
    if (m_currentAppliedTheme == theme) {
        m_logger.debug("Theme is already applied, skipping redundant application: " + theme);
        return;
    }

    // 首先更新StyleManager状态以保持一致性
    Theme styleManagerTheme = (theme == "dark") ? Theme::Dark : Theme::Light;
    STYLE.setTheme(styleManagerTheme);

    // 尝试从外部样式文件加载 - 支持多种部署场景
    QStringList possiblePaths = {
        // 开发环境：相对于可执行文件的assets目录
        QString("%1/../assets/styles/%2.qss").arg(qApp->applicationDirPath(), theme),
        // 部署环境：可执行文件同级的styles目录
        QString("%1/styles/%2.qss").arg(qApp->applicationDirPath(), theme),
        // 备选：相对于工作目录的assets目录
        QString("assets/styles/%2.qss").arg(theme),
        // 备选：当前目录的styles子目录
        QString("styles/%2.qss").arg(theme)
    };

    QString selectedPath;
    for (const QString& candidatePath : possiblePaths) {
        QFileInfo fileInfo(candidatePath);
        m_logger.debug(QString("Checking QSS path: %1 exists: %2")
                      .arg(candidatePath).arg(fileInfo.exists()));

        if (fileInfo.exists() && fileInfo.isReadable()) {
            selectedPath = candidatePath;
            m_logger.debug("Selected QSS path: " + selectedPath);
            break;
        }
    }

    if (!selectedPath.isEmpty()) {
        QFile file(selectedPath);
        if (file.open(QFile::ReadOnly)) {
            QString styleSheet = QLatin1String(file.readAll());
            file.close();

            if (!styleSheet.isEmpty()) {
                setStyleSheet(styleSheet);
                m_currentAppliedTheme = theme;
                m_logger.debug(QString("Applied external theme: %1 from %2")
                              .arg(theme).arg(selectedPath));
                return;
            } else {
                m_logger.warning("QSS file is empty: " + selectedPath);
            }
        } else {
            m_logger.warning("Failed to open QSS file: " + selectedPath);
        }
    }

    // 外部文件不可用时，使用StyleManager作为备选方案
    m_logger.warning("No external theme file found for theme: " + theme);
    m_logger.debug("Attempted paths: [" + possiblePaths.join(", ") + "]");
    m_logger.debug("Falling back to StyleManager for theme: " + theme);

    // 应用StyleManager生成的样式
    QString fallbackStyleSheet = STYLE.getApplicationStyleSheet();
    setStyleSheet(fallbackStyleSheet);
    m_currentAppliedTheme = theme;  // 更新当前应用的主题状态

    m_logger.debug("Applied fallback theme using StyleManager: " + theme);
}

void MainWindow::initWelcomeScreenConnections() {
    if (!m_welcomeScreenManager || !m_welcomeWidget) return;

    m_logger.debug("Setting up welcome screen connections...");

    // 连接欢迎界面管理器信号
    connect(m_welcomeScreenManager, &WelcomeScreenManager::showWelcomeScreenRequested,
            this, &MainWindow::onWelcomeScreenShowRequested);
    connect(m_welcomeScreenManager, &WelcomeScreenManager::hideWelcomeScreenRequested,
            this, &MainWindow::onWelcomeScreenHideRequested);
    connect(m_welcomeScreenManager, &WelcomeScreenManager::welcomeScreenEnabledChanged,
            menuBar, &MenuBar::setWelcomeScreenEnabled);

    // 连接菜单栏欢迎界面切换信号
    connect(menuBar, &MenuBar::welcomeScreenToggleRequested,
            m_welcomeScreenManager, &WelcomeScreenManager::onWelcomeScreenToggleRequested);

    // 连接欢迎界面组件信号
    connect(m_welcomeWidget, &WelcomeWidget::fileOpenRequested,
            this, &MainWindow::onWelcomeFileOpenRequested);
    connect(m_welcomeWidget, &WelcomeWidget::newFileRequested,
            this, &MainWindow::onWelcomeNewFileRequested);
    connect(m_welcomeWidget, &WelcomeWidget::openFileRequested,
            this, &MainWindow::onWelcomeOpenFileRequested);

    // 初始化菜单状态
    menuBar->setWelcomeScreenEnabled(m_welcomeScreenManager->isWelcomeScreenEnabled());

    // 启动欢迎界面管理器
    m_welcomeScreenManager->onApplicationStartup();

    m_logger.debug("Welcome screen connections established");
}

// Welcome screen slot implementations
void MainWindow::onWelcomeScreenShowRequested() {
    m_logger.debug("Showing welcome screen");
    if (m_contentStack) {
        m_contentStack->setCurrentIndex(0); // 欢迎界面
    }
}

void MainWindow::onWelcomeScreenHideRequested() {
    m_logger.debug("Hiding welcome screen");
    if (m_contentStack) {
        m_contentStack->setCurrentIndex(1); // 主查看器
    }
}

void MainWindow::onWelcomeFileOpenRequested(const QString& filePath) {
    m_logger.debug("Opening file from welcome screen: " + filePath);

    // 使用现有的文档控制器打开文件
    if (documentController) {
        documentController->openDocument(filePath);
    }
}

void MainWindow::onWelcomeNewFileRequested() {
    m_logger.debug("New file requested from welcome screen");

    // 这里可以实现新建文件的逻辑
    // 目前PDF阅读器可能不支持新建文件，所以可以显示打开文件对话框
    onWelcomeOpenFileRequested();
}

void MainWindow::onWelcomeOpenFileRequested() {
    m_logger.debug("Open file requested from welcome screen");

    // 使用现有的文档控制器打开文件对话框
    if (documentController) {
        documentController->execute(ActionMap::openFile, this);
    }
}

void MainWindow::handleActionExecuted(ActionMap id) {
    switch (id) {
        case ActionMap::fullScreen:
            if (isFullScreen()) {
                showNormal();
            } else {
                showFullScreen();
            }
            break;
        case ActionMap::zoomIn:
            // 通过现有的PDF操作信号处理缩放
            emit pdfViewerActionRequested(ActionMap::zoomIn);
            break;
        case ActionMap::zoomOut:
            // 通过现有的PDF操作信号处理缩放
            emit pdfViewerActionRequested(ActionMap::zoomOut);
            break;
        default:
            // 其他操作通过DocumentController处理
            break;
    }
}