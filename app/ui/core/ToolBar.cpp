#include "ToolBar.h"
#include <QAction>
#include <QHBoxLayout>
#include <QWidget>
#include "../../managers/StyleManager.h"

ToolBar::ToolBar(QWidget* parent) : QToolBar(parent) {
    setMovable(true);
    setObjectName("MainToolBar");
    setToolButtonStyle(Qt::ToolButtonIconOnly);

    // 初始化所有控件
    setupFileActions();
    createSeparator();
    setupNavigationActions();
    createSeparator();
    setupZoomActions();
    createSeparator();
    setupViewActions();
    createSeparator();
    setupRotationActions();
    createSeparator();
    setupThemeActions();

    // 应用样式
    applyToolBarStyle();

    // 初始状态：禁用所有操作（没有文档时）
    setActionsEnabled(false);
}

void ToolBar::setupFileActions() {
    // 打开文件
    openAction = new QAction("📁", this);
    openAction->setToolTip("打开PDF文件 (Ctrl+O)");
    openAction->setShortcut(QKeySequence("Ctrl+O"));
    addAction(openAction);

    // 打开文件夹
    openFolderAction = new QAction("📂", this);
    openFolderAction->setToolTip("打开文件夹 (Ctrl+Shift+O)");
    openFolderAction->setShortcut(QKeySequence("Ctrl+Shift+O"));
    addAction(openFolderAction);

    // 保存文件
    saveAction = new QAction("💾", this);
    saveAction->setToolTip("保存文件 (Ctrl+S)");
    saveAction->setShortcut(QKeySequence("Ctrl+S"));
    addAction(saveAction);

    // 连接信号
    connect(openAction, &QAction::triggered, this,
            [this]() { emit actionTriggered(ActionMap::openFile); });
    connect(openFolderAction, &QAction::triggered, this,
            [this]() { emit actionTriggered(ActionMap::openFolder); });
    connect(saveAction, &QAction::triggered, this,
            [this]() { emit actionTriggered(ActionMap::save); });
}

void ToolBar::setupNavigationActions() {
    // 第一页
    firstPageAction = new QAction("⏮", this);
    firstPageAction->setToolTip("第一页 (Ctrl+Home)");
    addAction(firstPageAction);

    // 上一页
    prevPageAction = new QAction("◀", this);
    prevPageAction->setToolTip("上一页 (Page Up)");
    addAction(prevPageAction);

    // 页码输入
    QWidget* pageWidget = new QWidget(this);
    QHBoxLayout* pageLayout = new QHBoxLayout(pageWidget);
    pageLayout->setContentsMargins(4, 0, 4, 0);
    pageLayout->setSpacing(2);

    pageSpinBox = new QSpinBox(pageWidget);
    pageSpinBox->setMinimum(1);
    pageSpinBox->setMaximum(1);
    pageSpinBox->setValue(1);
    pageSpinBox->setFixedWidth(60);
    pageSpinBox->setToolTip("当前页码");

    pageCountLabel = new QLabel("/ 1", pageWidget);
    pageCountLabel->setMinimumWidth(30);

    pageLayout->addWidget(pageSpinBox);
    pageLayout->addWidget(pageCountLabel);
    addWidget(pageWidget);

    // 下一页
    nextPageAction = new QAction("▶", this);
    nextPageAction->setToolTip("下一页 (Page Down)");
    addAction(nextPageAction);

    // 最后一页
    lastPageAction = new QAction("⏭", this);
    lastPageAction->setToolTip("最后一页 (Ctrl+End)");
    addAction(lastPageAction);

    // 连接信号
    connect(firstPageAction, &QAction::triggered, this,
            [this]() { emit actionTriggered(ActionMap::firstPage); });
    connect(prevPageAction, &QAction::triggered, this,
            [this]() { emit actionTriggered(ActionMap::previousPage); });
    connect(nextPageAction, &QAction::triggered, this,
            [this]() { emit actionTriggered(ActionMap::nextPage); });
    connect(lastPageAction, &QAction::triggered, this,
            [this]() { emit actionTriggered(ActionMap::lastPage); });
    connect(pageSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &ToolBar::onPageSpinBoxChanged);
}

void ToolBar::setupZoomActions() {
    // 缩小
    zoomOutAction = new QAction("🔍-", this);
    zoomOutAction->setToolTip("缩小 (Ctrl+-)");
    addAction(zoomOutAction);

    // 放大
    zoomInAction = new QAction("🔍+", this);
    zoomInAction->setToolTip("放大 (Ctrl++)");
    addAction(zoomInAction);

    // 适合宽度
    fitWidthAction = new QAction("📏", this);
    fitWidthAction->setToolTip("适合宽度 (Ctrl+1)");
    addAction(fitWidthAction);

    // 适合页面
    fitPageAction = new QAction("🗎", this);
    fitPageAction->setToolTip("适合页面 (Ctrl+0)");
    addAction(fitPageAction);

    // 适合高度
    fitHeightAction = new QAction("📐", this);
    fitHeightAction->setToolTip("适合高度 (Ctrl+2)");
    addAction(fitHeightAction);

    // 连接信号
    connect(zoomOutAction, &QAction::triggered, this,
            [this]() { emit actionTriggered(ActionMap::zoomOut); });
    connect(zoomInAction, &QAction::triggered, this,
            [this]() { emit actionTriggered(ActionMap::zoomIn); });
    connect(fitWidthAction, &QAction::triggered, this,
            [this]() { emit actionTriggered(ActionMap::fitToWidth); });
    connect(fitPageAction, &QAction::triggered, this,
            [this]() { emit actionTriggered(ActionMap::fitToPage); });
    connect(fitHeightAction, &QAction::triggered, this,
            [this]() { emit actionTriggered(ActionMap::fitToHeight); });
}

void ToolBar::setupViewActions() {
    // 侧边栏切换
    toggleSidebarAction = new QAction("📋", this);
    toggleSidebarAction->setToolTip("切换侧边栏 (F9)");
    toggleSidebarAction->setCheckable(true);
    toggleSidebarAction->setChecked(true);
    addAction(toggleSidebarAction);

    // 视图模式选择
    QWidget* viewWidget = new QWidget(this);
    QHBoxLayout* viewLayout = new QHBoxLayout(viewWidget);
    viewLayout->setContentsMargins(4, 0, 4, 0);

    viewModeCombo = new QComboBox(viewWidget);
    viewModeCombo->addItem("单页视图");
    viewModeCombo->addItem("连续滚动");
    viewModeCombo->setCurrentIndex(0);
    viewModeCombo->setToolTip("选择视图模式");
    viewModeCombo->setFixedWidth(100);

    viewLayout->addWidget(viewModeCombo);
    addWidget(viewWidget);

    // 连接信号
    connect(toggleSidebarAction, &QAction::triggered, this,
            [this]() { emit actionTriggered(ActionMap::toggleSideBar); });
    connect(viewModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ToolBar::onViewModeChanged);
}

void ToolBar::setupRotationActions() {
    // 向左旋转
    rotateLeftAction = new QAction("↺", this);
    rotateLeftAction->setToolTip("向左旋转90度 (Ctrl+L)");
    addAction(rotateLeftAction);

    // 向右旋转
    rotateRightAction = new QAction("↻", this);
    rotateRightAction->setToolTip("向右旋转90度 (Ctrl+R)");
    addAction(rotateRightAction);

    // 连接信号
    connect(rotateLeftAction, &QAction::triggered, this,
            [this]() { emit actionTriggered(ActionMap::rotateLeft); });
    connect(rotateRightAction, &QAction::triggered, this,
            [this]() { emit actionTriggered(ActionMap::rotateRight); });
}

void ToolBar::setupThemeActions() {
    // 主题切换
    themeToggleAction = new QAction("🌙", this);
    themeToggleAction->setToolTip("切换主题 (Ctrl+T)");
    addAction(themeToggleAction);

    // 连接信号
    connect(themeToggleAction, &QAction::triggered, this,
            [this]() { emit actionTriggered(ActionMap::toggleTheme); });
}

void ToolBar::createSeparator() { addSeparator(); }

void ToolBar::applyToolBarStyle() {
    // 应用工具栏样式
    setStyleSheet(STYLE.getToolbarStyleSheet());

    // 设置工具按钮样式
    QList<QAction*> actions = this->actions();
    for (QAction* action : actions) {
        if (!action->isSeparator()) {
            QWidget* widget = widgetForAction(action);
            if (widget) {
                widget->setStyleSheet(STYLE.getButtonStyleSheet());
            }
        }
    }
}

void ToolBar::updatePageInfo(int currentPage, int totalPages) {
    if (pageSpinBox && pageCountLabel) {
        pageSpinBox->blockSignals(true);
        pageSpinBox->setMaximum(totalPages);
        pageSpinBox->setValue(currentPage +
                              1);  // Convert from 0-based to 1-based
        pageSpinBox->blockSignals(false);

        pageCountLabel->setText(QString("/ %1").arg(totalPages));

        // 更新导航按钮状态
        firstPageAction->setEnabled(currentPage > 0);
        prevPageAction->setEnabled(currentPage > 0);
        nextPageAction->setEnabled(currentPage < totalPages - 1);
        lastPageAction->setEnabled(currentPage < totalPages - 1);
    }
}

void ToolBar::updateZoomLevel(double zoomFactor) {
    // 可以在这里更新缩放相关的UI状态
    // 例如禁用/启用缩放按钮基于当前缩放级别
    Q_UNUSED(zoomFactor)
}

void ToolBar::setActionsEnabled(bool enabled) {
    // 文件操作始终可用
    openAction->setEnabled(true);
    openFolderAction->setEnabled(true);
    saveAction->setEnabled(enabled);

    // 文档相关操作只有在有文档时才可用
    firstPageAction->setEnabled(enabled);
    prevPageAction->setEnabled(enabled);
    nextPageAction->setEnabled(enabled);
    lastPageAction->setEnabled(enabled);
    pageSpinBox->setEnabled(enabled);

    zoomInAction->setEnabled(enabled);
    zoomOutAction->setEnabled(enabled);
    fitWidthAction->setEnabled(enabled);
    fitPageAction->setEnabled(enabled);
    fitHeightAction->setEnabled(enabled);

    viewModeCombo->setEnabled(enabled);

    rotateLeftAction->setEnabled(enabled);
    rotateRightAction->setEnabled(enabled);

    // 侧边栏和主题切换始终可用
    toggleSidebarAction->setEnabled(true);
    themeToggleAction->setEnabled(true);
}

void ToolBar::onPageSpinBoxChanged(int pageNumber) {
    // 发出页码跳转请求（转换为0-based）
    emit pageJumpRequested(pageNumber - 1);
}

void ToolBar::onViewModeChanged() {
    int mode = viewModeCombo->currentIndex();
    if (mode == 0) {
        emit actionTriggered(ActionMap::setSinglePageMode);
    } else if (mode == 1) {
        emit actionTriggered(ActionMap::setContinuousScrollMode);
    }
}
