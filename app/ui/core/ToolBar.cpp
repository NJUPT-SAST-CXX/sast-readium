#include "ToolBar.h"

// ElaWidgetTools
#include "ElaComboBox.h"
#include "ElaIcon.h"
#include "ElaLineEdit.h"
#include "ElaSlider.h"
#include "ElaToolButton.h"

// Qt
#include <QEvent>
#include <QHBoxLayout>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QWidget>
#include <QWidgetAction>
#include <algorithm>
#include <cmath>
#include "ElaText.h"

// Logging
#include "logging/SimpleLogging.h"

ToolBar::ToolBar(const QString& title, QWidget* parent)
    : ::ElaToolBar(title, parent),
      m_currentPage(0),
      m_totalPages(0),
      m_currentZoom(1.0),
      m_actionsEnabled(false),
      m_isUpdatingZoom(false),
      m_isUpdatingPage(false),
      m_compactMode(false),
      m_currentFileSize(0),
      m_openFolderBtn(nullptr),
      m_saveAsBtn(nullptr),
      m_emailBtn(nullptr),
      m_pageSpinBox(nullptr),
      m_pageCountLabel(nullptr),
      m_pageSlider(nullptr),
      m_thumbnailPreview(nullptr),
      m_layoutCombo(nullptr),
      m_toggleSidebarBtn(nullptr),
      m_nightModeBtn(nullptr),
      m_readingModeBtn(nullptr),
      m_zoomLabel(nullptr),
      m_highlightBtn(nullptr),
      m_snapshotBtn(nullptr),
      m_themeToggleBtn(nullptr),
      m_settingsBtn(nullptr),
      m_helpBtn(nullptr),
      m_documentInfoLabel(nullptr),
      m_fileSizeLabel(nullptr),
      m_lastModifiedLabel(nullptr) {
    SLOG_INFO("ToolBar: Constructor started");

    // 设置工具栏属性
    setMovable(false);
    setFloatable(false);
    setIconSize(QSize(20, 20));

    // 创建所有区域
    setupFileSection();
    addSeparator();
    setupNavigationSection();
    addSeparator();
    setupZoomSection();
    addSeparator();
    setupViewSection();
    addSeparator();
    setupToolsSection();
    addSeparator();
    setupQuickAccessBar();

    // Add stretch to push quick access bar to the right
    QWidget* spacer = new QWidget();
    spacer->setObjectName("ToolBarSpacer");
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    spacer->setToolTip(tr("Toolbar Spacer"));
    if (QAction* spacerAction = addWidget(spacer)) {
        if (auto* widgetAction = qobject_cast<QWidgetAction*>(spacerAction)) {
            widgetAction->setToolTip(spacer->toolTip());
        } else {
            spacerAction->setToolTip(spacer->toolTip());
        }
    }

    setupDocumentInfo();

    // 初始化状态
    updateButtonStates();
    syncActionToolTips();

    SLOG_INFO("ToolBar: Constructor completed");
}

ToolBar::~ToolBar() { SLOG_INFO("ToolBar: Destructor called"); }

void ToolBar::setupFileSection() {
    // 打开文件
    m_openBtn = createToolButton("FolderOpen", tr("Open File"), "Ctrl+O");
    addWidget(m_openBtn);
    connect(m_openBtn, &ElaToolButton::clicked, this,
            [this]() { emit actionTriggered(ActionMap::openFile); });

    // 打开文件夹
    m_openFolderBtn = createToolButton("Folder", tr("Open Folder"), "");
    addWidget(m_openFolderBtn);
    connect(m_openFolderBtn, &ElaToolButton::clicked, this,
            [this]() { emit actionTriggered(ActionMap::openFolder); });

    // 保存副本
    m_saveBtn = createToolButton("FloppyDisk", tr("Save Copy"), "Ctrl+S");
    addWidget(m_saveBtn);
    connect(m_saveBtn, &ElaToolButton::clicked, this,
            [this]() { emit actionTriggered(ActionMap::saveAs); });

    // 另存为
    m_saveAsBtn =
        createToolButton("FloppyDisk", tr("Save As..."), "Ctrl+Shift+S");
    addWidget(m_saveAsBtn);
    connect(m_saveAsBtn, &ElaToolButton::clicked, this,
            [this]() { emit actionTriggered(ActionMap::saveAs); });

    // 打印
    m_printBtn = createToolButton("Print", tr("Print"), "Ctrl+P");
    addWidget(m_printBtn);
    connect(m_printBtn, &ElaToolButton::clicked, this,
            [this]() { emit actionTriggered(ActionMap::printFile); });

    // 邮件 - 注释掉，ActionMap中不存在emailDocument
    // m_emailBtn = createToolButton("Envelope", tr("Email"), "");
    // addWidget(m_emailBtn);
    // connect(m_emailBtn, &ElaToolButton::clicked, this, [this]() {
    //     emit actionTriggered(ActionMap::emailDocument);
    // });
}

void ToolBar::setupNavigationSection() {
    // 首页
    m_firstPageBtn = createToolButton("BackwardStep", tr("First Page"), "Home");
    addWidget(m_firstPageBtn);
    connect(m_firstPageBtn, &ElaToolButton::clicked, this,
            &ToolBar::goToFirstPageRequested);

    // 上一页
    m_prevPageBtn =
        createToolButton("ChevronLeft", tr("Previous Page"), "Page Up");
    addWidget(m_prevPageBtn);
    connect(m_prevPageBtn, &ElaToolButton::clicked, this,
            &ToolBar::goToPreviousPageRequested);

    // 后退
    m_backBtn = createToolButton("ArrowLeft", tr("Go Back"), "Alt+Left");
    addWidget(m_backBtn);
    connect(m_backBtn, &ElaToolButton::clicked, this,
            &ToolBar::goBackRequested);

    // 前进
    m_forwardBtn =
        createToolButton("ArrowRight", tr("Go Forward"), "Alt+Right");
    addWidget(m_forwardBtn);
    connect(m_forwardBtn, &ElaToolButton::clicked, this,
            &ToolBar::goForwardRequested);

    // 页码输入
    m_pageSpinBox = new QSpinBox(this);
    m_pageSpinBox->setObjectName("ToolBarPageSpinBox");
    m_pageSpinBox->setFixedWidth(60);
    m_pageSpinBox->setRange(1, 1);
    m_pageSpinBox->setValue(1);
    m_pageSpinBox->setAlignment(Qt::AlignCenter);
    m_pageSpinBox->setToolTip(tr("Page Number"));
    addWidget(m_pageSpinBox);
    connect(m_pageSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int displayValue) {
                if (m_isUpdatingPage || m_totalPages <= 0) {
                    return;
                }

                const int targetIndex =
                    std::clamp(displayValue, 1, m_totalPages) - 1;
                if (targetIndex != m_currentPage) {
                    m_currentPage = targetIndex;
                    updateButtonStates();
                    emit pageJumpRequested(targetIndex);
                }
            });

    // 页数标签
    m_pageCountLabel = new ElaText(tr("/ 0"), this);
    m_pageCountLabel->setMinimumWidth(50);
    m_pageCountLabel->setToolTip(tr("Total Pages"));
    addWidget(m_pageCountLabel);

    // 下一页
    m_nextPageBtn =
        createToolButton("ChevronRight", tr("Next Page"), "Page Down");
    addWidget(m_nextPageBtn);
    connect(m_nextPageBtn, &ElaToolButton::clicked, this,
            &ToolBar::goToNextPageRequested);

    // 末页
    m_lastPageBtn = createToolButton("ForwardStep", tr("Last Page"), "End");
    addWidget(m_lastPageBtn);
    connect(m_lastPageBtn, &ElaToolButton::clicked, this,
            &ToolBar::goToLastPageRequested);
}

void ToolBar::setupZoomSection() {
    // 缩小
    m_zoomOutBtn =
        createToolButton("MagnifyingGlassMinus", tr("Zoom Out"), "Ctrl+-");
    addWidget(m_zoomOutBtn);
    connect(m_zoomOutBtn, &ElaToolButton::clicked, this,
            &ToolBar::zoomOutRequested);

    // 缩放滑块
    m_zoomSlider = new ElaSlider(Qt::Horizontal, this);
    m_zoomSlider->setFixedWidth(120);
    m_zoomSlider->setMinimum(25);   // 25%
    m_zoomSlider->setMaximum(400);  // 400%
    m_zoomSlider->setValue(100);    // 100%
    m_zoomSlider->setTickPosition(QSlider::NoTicks);
    m_zoomSlider->setToolTip(tr("Zoom Level"));
    addWidget(m_zoomSlider);
    connect(m_zoomSlider, &ElaSlider::valueChanged, this, [this](int value) {
        if (!m_isUpdatingZoom) {
            const double zoomPercent = static_cast<double>(value);
            emit zoomLevelChanged(zoomPercent);
        }
    });

    // 缩放显示标签
    m_zoomLabel = new ElaText("100%", this);
    m_zoomLabel->setAlignment(Qt::AlignCenter);
    m_zoomLabel->setMinimumWidth(50);
    m_zoomLabel->setToolTip(tr("Current Zoom"));
    addWidget(m_zoomLabel);

    // 缩放输入
    m_zoomInput = new ElaLineEdit(this);
    m_zoomInput->setFixedWidth(70);
    m_zoomInput->setText("100%");
    m_zoomInput->setAlignment(Qt::AlignCenter);
    m_zoomInput->setToolTip(tr("Zoom Percentage"));
    addWidget(m_zoomInput);
    connect(m_zoomInput, &ElaLineEdit::returnPressed, this, [this]() {
        QString text = m_zoomInput->text();
        text.remove('%');
        bool parseSuccessful = false;
        const double zoom = text.toDouble(&parseSuccessful);
        if (parseSuccessful && zoom >= 25.0 && zoom <= 400.0) {
            emit zoomLevelChanged(zoom);
        } else {
            // 恢复当前缩放
            m_zoomInput->setText(QString::number(m_currentZoom * 100, 'f', 0) +
                                 "%");
        }
    });

    // 放大
    m_zoomInBtn =
        createToolButton("MagnifyingGlassPlus", tr("Zoom In"), "Ctrl++");
    addWidget(m_zoomInBtn);
    connect(m_zoomInBtn, &ElaToolButton::clicked, this,
            &ToolBar::zoomInRequested);

    // 缩放预设
    m_zoomPresets = new ElaComboBox(this);
    m_zoomPresets->setFixedWidth(120);
    m_zoomPresets->setToolTip(tr("Zoom Presets"));
    m_zoomPresets->addItem(tr("Fit Width"), "fitWidth");
    m_zoomPresets->addItem(tr("Fit Page"), "fitPage");
    m_zoomPresets->addItem(tr("Fit Height"), "fitHeight");
    m_zoomPresets->addItem("50%", 50);
    m_zoomPresets->addItem("75%", 75);
    m_zoomPresets->addItem("100%", 100);
    m_zoomPresets->addItem("125%", 125);
    m_zoomPresets->addItem("150%", 150);
    m_zoomPresets->addItem("200%", 200);
    m_zoomPresets->addItem("300%", 300);
    m_zoomPresets->addItem("400%", 400);
    m_zoomPresets->setCurrentIndex(5);  // 100%
    addWidget(m_zoomPresets);
    connect(m_zoomPresets,
            QOverload<int>::of(&ElaComboBox::currentIndexChanged), this,
            [this](int index) {
                QVariant data = m_zoomPresets->itemData(index);
                if (data.typeId() ==
                    QMetaType::QString) {  // Use typeId() and QMetaType instead
                    QString preset = data.toString();
                    if (preset == "fitWidth") {
                        emit fitWidthRequested();
                    } else if (preset == "fitPage") {
                        emit fitPageRequested();
                    } else if (preset == "fitHeight") {
                        emit fitHeightRequested();
                    }
                } else if (data.canConvert<int>()) {
                    emit zoomLevelChanged(data.toInt());
                }
            });

    // 快速缩放按钮
    m_fitWidthBtn =
        createToolButton("ArrowsLeftRight", tr("Fit Width"), "Ctrl+1");
    addWidget(m_fitWidthBtn);
    connect(m_fitWidthBtn, &ElaToolButton::clicked, this,
            &ToolBar::fitWidthRequested);

    m_fitPageBtn = createToolButton("Maximize", tr("Fit Page"), "Ctrl+2");
    addWidget(m_fitPageBtn);
    connect(m_fitPageBtn, &ElaToolButton::clicked, this,
            &ToolBar::fitPageRequested);

    m_fitHeightBtn =
        createToolButton("ArrowsUpDown", tr("Fit Height"), "Ctrl+3");
    addWidget(m_fitHeightBtn);
    connect(m_fitHeightBtn, &ElaToolButton::clicked, this,
            &ToolBar::fitHeightRequested);
}

void ToolBar::setupViewSection() {
    // 视图模式
    m_viewModeCombo = new ElaComboBox(this);
    m_viewModeCombo->setFixedWidth(140);
    m_viewModeCombo->setToolTip(tr("View Mode"));
    m_viewModeCombo->addItem(tr("Single Page"), 0);
    m_viewModeCombo->addItem(tr("Continuous"), 1);
    m_viewModeCombo->addItem(tr("Two Pages"), 2);
    m_viewModeCombo->addItem(tr("Book Mode"), 3);
    addWidget(m_viewModeCombo);
    connect(m_viewModeCombo,
            QOverload<int>::of(&ElaComboBox::currentIndexChanged), this,
            [this](int index) {
                int mode = m_viewModeCombo->itemData(index).toInt();
                emit viewModeChanged(mode);
            });

    // 布局模式
    m_layoutCombo = new ElaComboBox(this);
    m_layoutCombo->setFixedWidth(120);
    m_layoutCombo->setToolTip(tr("Layout Mode"));
    m_layoutCombo->addItem(tr("Vertical"), 0);
    m_layoutCombo->addItem(tr("Horizontal"), 1);
    addWidget(m_layoutCombo);
    connect(m_layoutCombo,
            QOverload<int>::of(&ElaComboBox::currentIndexChanged), this,
            [this](int index) {
                int mode = m_layoutCombo->itemData(index).toInt();
                emit layoutModeChanged(mode);
            });

    // 向左旋转
    m_rotateLeftBtn =
        createToolButton("RotateLeft", tr("Rotate Left"), "Ctrl+L");
    addWidget(m_rotateLeftBtn);
    connect(m_rotateLeftBtn, &ElaToolButton::clicked, this,
            &ToolBar::rotateLeftRequested);

    // 向右旋转
    m_rotateRightBtn =
        createToolButton("RotateRight", tr("Rotate Right"), "Ctrl+R");
    addWidget(m_rotateRightBtn);
    connect(m_rotateRightBtn, &ElaToolButton::clicked, this,
            &ToolBar::rotateRightRequested);

    // 全屏
    m_fullscreenBtn = createToolButton("Expand", tr("Full Screen"), "F11");
    m_fullscreenBtn->setCheckable(true);
    addWidget(m_fullscreenBtn);
    connect(m_fullscreenBtn, &ElaToolButton::toggled, this,
            &ToolBar::fullScreenToggled);

    // 侧边栏切换
    m_toggleSidebarBtn =
        createToolButton("Sidebar", tr("Toggle Sidebar"), "F9");
    m_toggleSidebarBtn->setCheckable(true);
    m_toggleSidebarBtn->setChecked(true);
    addWidget(m_toggleSidebarBtn);
    connect(m_toggleSidebarBtn, &ElaToolButton::toggled, this,
            &ToolBar::toggleSidebarRequested);

    // 夜间模式
    m_nightModeBtn = createToolButton("Moon", tr("Night Mode"), "");
    m_nightModeBtn->setCheckable(true);
    addWidget(m_nightModeBtn);
    connect(m_nightModeBtn, &ElaToolButton::toggled, this,
            &ToolBar::nightModeToggled);

    // 阅读模式
    m_readingModeBtn = createToolButton("Book", tr("Reading Mode"), "");
    m_readingModeBtn->setCheckable(true);
    addWidget(m_readingModeBtn);
    connect(m_readingModeBtn, &ElaToolButton::toggled, this,
            &ToolBar::readingModeToggled);
}

void ToolBar::setupToolsSection() {
    // 搜索
    m_searchBtn = createToolButton("MagnifyingGlass", tr("Search"), "Ctrl+F");
    m_searchBtn->setCheckable(true);
    addWidget(m_searchBtn);
    connect(m_searchBtn, &ElaToolButton::toggled, this,
            &ToolBar::searchRequested);

    // 书签
    m_bookmarkBtn = createToolButton("Bookmark", tr("Bookmark"), "Ctrl+D");
    m_bookmarkBtn->setCheckable(true);
    addWidget(m_bookmarkBtn);
    connect(m_bookmarkBtn, &ElaToolButton::toggled, this,
            &ToolBar::bookmarkToggled);

    // 注释
    m_annotationBtn = createToolButton("PenToSquare", tr("Annotate"), "Ctrl+A");
    m_annotationBtn->setCheckable(true);
    addWidget(m_annotationBtn);
    connect(m_annotationBtn, &ElaToolButton::toggled, this,
            &ToolBar::annotationModeToggled);

    // 高亮
    m_highlightBtn = createToolButton("Highlighter", tr("Highlight"), "Ctrl+H");
    m_highlightBtn->setCheckable(true);
    addWidget(m_highlightBtn);
    connect(m_highlightBtn, &ElaToolButton::toggled, this,
            &ToolBar::highlightRequested);

    // 快照
    m_snapshotBtn = createToolButton("Camera", tr("Snapshot"), "Ctrl+Shift+S");
    addWidget(m_snapshotBtn);
    connect(m_snapshotBtn, &ElaToolButton::clicked, this,
            &ToolBar::snapshotRequested);
}

void ToolBar::setupQuickAccessBar() {
    // 主题切换
    m_themeToggleBtn = createToolButton("Palette", tr("Toggle Theme"), "");
    addWidget(m_themeToggleBtn);
    connect(m_themeToggleBtn, &ElaToolButton::clicked, this,
            [this]() { emit actionTriggered(ActionMap::toggleTheme); });

    // 设置
    m_settingsBtn = createToolButton("Gear", tr("Settings"), "");
    addWidget(m_settingsBtn);
    connect(m_settingsBtn, &ElaToolButton::clicked, this,
            [this]() { emit actionTriggered(ActionMap::showSettings); });

    // 帮助
    m_helpBtn = createToolButton("CircleQuestion", tr("Help"), "F1");
    addWidget(m_helpBtn);
    connect(m_helpBtn, &ElaToolButton::clicked, this,
            [this]() { emit actionTriggered(ActionMap::showHelp); });
}

void ToolBar::setupDocumentInfo() {
    // 文档信息标签
    m_documentInfoLabel = new ElaText(this);
    m_documentInfoLabel->setStyleSheet(
        "QLabel { color: palette(text); padding: 0 8px; }");
    m_documentInfoLabel->setToolTip(tr("Document Name"));
    addWidget(m_documentInfoLabel);

    m_fileSizeLabel = new ElaText(this);
    m_fileSizeLabel->setStyleSheet(
        "QLabel { color: palette(mid); padding: 0 8px; font-size: 10px; }");
    m_fileSizeLabel->setToolTip(tr("File Size"));
    addWidget(m_fileSizeLabel);

    m_lastModifiedLabel = new ElaText(this);
    m_lastModifiedLabel->setStyleSheet(
        "QLabel { color: palette(mid); padding: 0 8px; font-size: 10px; }");
    m_lastModifiedLabel->setToolTip(tr("Last Modified"));
    addWidget(m_lastModifiedLabel);

    // 初始隐藏
    m_documentInfoLabel->setVisible(false);
    m_fileSizeLabel->setVisible(false);
    m_lastModifiedLabel->setVisible(false);
}

void ToolBar::updatePageInfo(int currentPage, int totalPages) {
    m_totalPages = std::max(totalPages, 0);
    if (m_totalPages > 0) {
        m_currentPage = std::clamp(currentPage, 0, m_totalPages - 1);
    } else {
        m_currentPage = 0;
    }

    if (m_pageSpinBox != nullptr) {
        const QSignalBlocker blocker(m_pageSpinBox);
        m_isUpdatingPage = true;
        if (m_totalPages > 0) {
            m_pageSpinBox->setRange(1, m_totalPages);
            m_pageSpinBox->setValue(m_currentPage + 1);
        } else {
            m_pageSpinBox->setRange(0, 0);
            m_pageSpinBox->setValue(0);
        }
        m_pageSpinBox->setEnabled(m_actionsEnabled && m_totalPages > 0);
        m_isUpdatingPage = false;
    }

    if (m_pageCountLabel != nullptr) {
        m_pageCountLabel->setText(QString("/ %1").arg(m_totalPages));
    }

    updateButtonStates();
}

void ToolBar::updateZoomLevel(double zoomFactor) {
    m_currentZoom = zoomFactor;
    m_isUpdatingZoom = true;

    // 更新滑块
    const int zoomPercent = static_cast<int>(std::round(zoomFactor * 100.0));
    const int clampedPercent = std::clamp(zoomPercent, m_zoomSlider->minimum(),
                                          m_zoomSlider->maximum());
    m_zoomSlider->setValue(clampedPercent);

    // 更新输入框
    m_zoomInput->setText(QString::number(clampedPercent) + "%");

    // 更新预设下拉框（如果匹配）
    for (int i = 0; i < m_zoomPresets->count(); ++i) {
        QVariant data = m_zoomPresets->itemData(i);
        if (data.canConvert<int>() && data.toInt() == clampedPercent) {
            m_zoomPresets->setCurrentIndex(i);
            break;
        }
    }

    if (m_zoomLabel != nullptr) {
        m_zoomLabel->setText(QString::number(clampedPercent) + "%");
    }

    m_isUpdatingZoom = false;
}

void ToolBar::updateDocumentInfo(const QString& fileName, qint64 fileSize,
                                 const QDateTime& lastModified) {
    m_currentFileName = fileName;
    m_currentFileSize = fileSize;
    m_currentLastModified = lastModified;
    updateDocumentInfoDisplay();
}

void ToolBar::setActionsEnabled(bool enabled) {
    m_actionsEnabled = enabled;
    updateButtonStates();
}

void ToolBar::setCompactMode(bool compact) {
    m_compactMode = compact;

    // 在紧凑模式下隐藏某些控件
    if (m_documentInfoLabel) {
        m_documentInfoLabel->setVisible(!compact &&
                                        !m_currentFileName.isEmpty());
    }
    if (m_fileSizeLabel) {
        m_fileSizeLabel->setVisible(!compact && m_currentFileSize > 0);
    }
    if (m_lastModifiedLabel) {
        m_lastModifiedLabel->setVisible(!compact &&
                                        m_currentLastModified.isValid());
    }
}

void ToolBar::setNavigationEnabled(bool canGoBack, bool canGoForward) {
    m_backBtn->setEnabled(canGoBack);
    m_forwardBtn->setEnabled(canGoForward);
}

void ToolBar::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    ::ElaToolBar::changeEvent(event);  // Call base class, not self
}

void ToolBar::retranslateUi() {
    SLOG_INFO("ToolBar: Retranslating UI");

    // 更新文件操作工具提示
    setButtonTooltip(m_openBtn, tr("Open File"),
                     m_openBtn->property("tooltipShortcut").toString());
    if (m_openFolderBtn)
        setButtonTooltip(
            m_openFolderBtn, tr("Open Folder"),
            m_openFolderBtn->property("tooltipShortcut").toString());
    setButtonTooltip(m_saveBtn, tr("Save Copy"),
                     m_saveBtn->property("tooltipShortcut").toString());
    if (m_saveAsBtn)
        setButtonTooltip(m_saveAsBtn, tr("Save As..."),
                         m_saveAsBtn->property("tooltipShortcut").toString());
    setButtonTooltip(m_printBtn, tr("Print"),
                     m_printBtn->property("tooltipShortcut").toString());

    // 更新导航工具提示
    setButtonTooltip(m_firstPageBtn, tr("First Page"),
                     m_firstPageBtn->property("tooltipShortcut").toString());
    setButtonTooltip(m_prevPageBtn, tr("Previous Page"),
                     m_prevPageBtn->property("tooltipShortcut").toString());
    setButtonTooltip(m_backBtn, tr("Go Back"),
                     m_backBtn->property("tooltipShortcut").toString());
    setButtonTooltip(m_forwardBtn, tr("Go Forward"),
                     m_forwardBtn->property("tooltipShortcut").toString());
    setButtonTooltip(m_nextPageBtn, tr("Next Page"),
                     m_nextPageBtn->property("tooltipShortcut").toString());
    setButtonTooltip(m_lastPageBtn, tr("Last Page"),
                     m_lastPageBtn->property("tooltipShortcut").toString());

    // 更新缩放工具提示
    setButtonTooltip(m_zoomOutBtn, tr("Zoom Out"),
                     m_zoomOutBtn->property("tooltipShortcut").toString());
    setButtonTooltip(m_zoomInBtn, tr("Zoom In"),
                     m_zoomInBtn->property("tooltipShortcut").toString());
    setButtonTooltip(m_fitWidthBtn, tr("Fit Width"),
                     m_fitWidthBtn->property("tooltipShortcut").toString());
    setButtonTooltip(m_fitPageBtn, tr("Fit Page"),
                     m_fitPageBtn->property("tooltipShortcut").toString());
    setButtonTooltip(m_fitHeightBtn, tr("Fit Height"),
                     m_fitHeightBtn->property("tooltipShortcut").toString());
    if (m_pageSpinBox)
        m_pageSpinBox->setToolTip(tr("Page Number"));
    if (m_pageCountLabel)
        m_pageCountLabel->setToolTip(tr("Total Pages"));
    if (m_zoomSlider)
        m_zoomSlider->setToolTip(tr("Zoom Level"));
    if (m_zoomLabel)
        m_zoomLabel->setToolTip(tr("Current Zoom"));
    if (m_zoomInput)
        m_zoomInput->setToolTip(tr("Zoom Percentage"));
    if (m_zoomPresets)
        m_zoomPresets->setToolTip(tr("Zoom Presets"));

    // 更新视图工具提示
    setButtonTooltip(m_rotateLeftBtn, tr("Rotate Left"),
                     m_rotateLeftBtn->property("tooltipShortcut").toString());
    setButtonTooltip(m_rotateRightBtn, tr("Rotate Right"),
                     m_rotateRightBtn->property("tooltipShortcut").toString());
    setButtonTooltip(m_fullscreenBtn, tr("Full Screen"),
                     m_fullscreenBtn->property("tooltipShortcut").toString());
    if (m_toggleSidebarBtn)
        setButtonTooltip(
            m_toggleSidebarBtn, tr("Toggle Sidebar"),
            m_toggleSidebarBtn->property("tooltipShortcut").toString());
    if (m_nightModeBtn)
        setButtonTooltip(
            m_nightModeBtn, tr("Night Mode"),
            m_nightModeBtn->property("tooltipShortcut").toString());
    if (m_readingModeBtn)
        setButtonTooltip(
            m_readingModeBtn, tr("Reading Mode"),
            m_readingModeBtn->property("tooltipShortcut").toString());

    // 更新工具提示
    setButtonTooltip(m_searchBtn, tr("Search"),
                     m_searchBtn->property("tooltipShortcut").toString());
    setButtonTooltip(m_bookmarkBtn, tr("Bookmark"),
                     m_bookmarkBtn->property("tooltipShortcut").toString());
    setButtonTooltip(m_annotationBtn, tr("Annotate"),
                     m_annotationBtn->property("tooltipShortcut").toString());
    if (m_highlightBtn)
        setButtonTooltip(
            m_highlightBtn, tr("Highlight"),
            m_highlightBtn->property("tooltipShortcut").toString());
    if (m_snapshotBtn)
        setButtonTooltip(m_snapshotBtn, tr("Snapshot"),
                         m_snapshotBtn->property("tooltipShortcut").toString());

    // 更新快速访问栏工具提示
    if (m_themeToggleBtn)
        setButtonTooltip(
            m_themeToggleBtn, tr("Toggle Theme"),
            m_themeToggleBtn->property("tooltipShortcut").toString());
    if (m_settingsBtn)
        setButtonTooltip(m_settingsBtn, tr("Settings"),
                         m_settingsBtn->property("tooltipShortcut").toString());
    if (m_helpBtn)
        setButtonTooltip(m_helpBtn, tr("Help"),
                         m_helpBtn->property("tooltipShortcut").toString());

    if (QWidget* spacerWidget = findChild<QWidget*>("ToolBarSpacer")) {
        spacerWidget->setToolTip(tr("Toolbar Spacer"));
    }
    if (m_documentInfoLabel)
        m_documentInfoLabel->setToolTip(tr("Document Name"));
    if (m_fileSizeLabel)
        m_fileSizeLabel->setToolTip(tr("File Size"));
    if (m_lastModifiedLabel)
        m_lastModifiedLabel->setToolTip(tr("Last Modified"));

    // 更新下拉框项
    m_zoomPresets->setItemText(0, tr("Fit Width"));
    m_zoomPresets->setItemText(1, tr("Fit Page"));
    m_zoomPresets->setItemText(2, tr("Fit Height"));

    m_viewModeCombo->setItemText(0, tr("Single Page"));
    m_viewModeCombo->setItemText(1, tr("Continuous"));
    m_viewModeCombo->setItemText(2, tr("Two Pages"));
    m_viewModeCombo->setItemText(3, tr("Book Mode"));
    m_viewModeCombo->setToolTip(tr("View Mode"));

    if (m_layoutCombo) {
        m_layoutCombo->setItemText(0, tr("Vertical"));
        m_layoutCombo->setItemText(1, tr("Horizontal"));
        m_layoutCombo->setToolTip(tr("Layout Mode"));
    }

    syncActionToolTips();
}

void ToolBar::syncActionToolTips() {
    for (QAction* action : actions()) {
        if (!action || action->isSeparator()) {
            continue;
        }

        if (auto* widgetAction = qobject_cast<QWidgetAction*>(action)) {
            if (QWidget* widget = widgetAction->defaultWidget()) {
                action->setToolTip(widget->toolTip());
            }
        }
    }
}

void ToolBar::setButtonTooltip(ElaToolButton* button, const QString& text,
                               const QString& shortcut) {
    if (!button) {
        return;
    }

    button->setProperty("tooltipBase", text);
    button->setProperty("tooltipShortcut", shortcut);

    QString formatted = text;
    if (!shortcut.isEmpty()) {
        formatted.append(" (" + shortcut + ")");
    }

    button->setToolTip(formatted);
}

void ToolBar::updateDocumentInfoDisplay() {
    if (!m_documentInfoLabel || !m_fileSizeLabel || !m_lastModifiedLabel) {
        return;
    }

    // 更新文件名
    if (!m_currentFileName.isEmpty()) {
        m_documentInfoLabel->setText(m_currentFileName);
        m_documentInfoLabel->setVisible(!m_compactMode);
    } else {
        m_documentInfoLabel->setVisible(false);
    }

    // 更新文件大小
    if (m_currentFileSize > 0) {
        QString sizeText;
        if (m_currentFileSize < 1024) {
            sizeText = QString("%1 B").arg(m_currentFileSize);
        } else if (m_currentFileSize < 1024 * 1024) {
            sizeText =
                QString("%1 KB").arg(m_currentFileSize / 1024.0, 0, 'f', 1);
        } else {
            sizeText = QString("%1 MB").arg(
                m_currentFileSize / (1024.0 * 1024.0), 0, 'f', 1);
        }
        m_fileSizeLabel->setText(sizeText);
        m_fileSizeLabel->setVisible(!m_compactMode);
    } else {
        m_fileSizeLabel->setVisible(false);
    }

    // 更新最后修改时间
    if (m_currentLastModified.isValid()) {
        m_lastModifiedLabel->setText(
            m_currentLastModified.toString("yyyy-MM-dd hh:mm"));
        m_lastModifiedLabel->setVisible(!m_compactMode);
    } else {
        m_lastModifiedLabel->setVisible(false);
    }
}

void ToolBar::updateButtonStates() {
    bool hasDocument = m_actionsEnabled;

    // 文件操作
    if (m_openFolderBtn)
        m_openFolderBtn->setEnabled(true);  // Always enabled
    if (m_saveBtn)
        m_saveBtn->setEnabled(hasDocument);
    if (m_saveAsBtn)
        m_saveAsBtn->setEnabled(hasDocument);
    if (m_printBtn)
        m_printBtn->setEnabled(hasDocument);

    // 导航
    bool hasPages = hasDocument && m_totalPages > 0;
    m_firstPageBtn->setEnabled(hasPages && m_currentPage > 0);
    m_prevPageBtn->setEnabled(hasPages && m_currentPage > 0);
    if (m_pageSpinBox != nullptr) {
        m_pageSpinBox->setEnabled(hasPages);
    }
    m_nextPageBtn->setEnabled(hasPages && m_currentPage < m_totalPages - 1);
    m_lastPageBtn->setEnabled(hasPages && m_currentPage < m_totalPages - 1);

    // 缩放
    m_zoomOutBtn->setEnabled(hasDocument);
    m_zoomSlider->setEnabled(hasDocument);
    m_zoomInput->setEnabled(hasDocument);
    m_zoomInBtn->setEnabled(hasDocument);
    m_zoomPresets->setEnabled(hasDocument);
    m_fitWidthBtn->setEnabled(hasDocument);
    m_fitPageBtn->setEnabled(hasDocument);
    m_fitHeightBtn->setEnabled(hasDocument);
    if (m_zoomLabel) {
        m_zoomLabel->setEnabled(hasDocument);
    }

    // 视图
    m_viewModeCombo->setEnabled(hasDocument);
    if (m_layoutCombo)
        m_layoutCombo->setEnabled(hasDocument);
    m_rotateLeftBtn->setEnabled(hasDocument);
    m_rotateRightBtn->setEnabled(hasDocument);
    m_fullscreenBtn->setEnabled(hasDocument);
    if (m_toggleSidebarBtn)
        m_toggleSidebarBtn->setEnabled(true);  // Always enabled
    if (m_nightModeBtn)
        m_nightModeBtn->setEnabled(hasDocument);
    if (m_readingModeBtn)
        m_readingModeBtn->setEnabled(hasDocument);

    // 工具
    m_searchBtn->setEnabled(hasDocument);
    m_bookmarkBtn->setEnabled(hasDocument);
    m_annotationBtn->setEnabled(hasDocument);
    if (m_highlightBtn)
        m_highlightBtn->setEnabled(hasDocument);
    if (m_snapshotBtn)
        m_snapshotBtn->setEnabled(hasDocument);
}

ElaToolButton* ToolBar::createToolButton(const QString& iconName,
                                         const QString& tooltip,
                                         const QString& shortcut) {
    ElaToolButton* button = new ElaToolButton(this);

    // Set icon using the icon mapping system
    if (!iconName.isEmpty()) {
        static QMap<QString, ElaIconType::IconName> iconMap = {
            {"File", ElaIconType::File},
            {"FolderOpen", ElaIconType::FolderOpen},
            {"FloppyDisk", ElaIconType::FloppyDisk},
            {"Print", ElaIconType::Print},
            {"ArrowLeft", ElaIconType::ArrowLeft},
            {"ArrowRight", ElaIconType::ArrowRight},
            {"MagnifyingGlassPlus", ElaIconType::MagnifyingGlassPlus},
            {"MagnifyingGlassMinus", ElaIconType::MagnifyingGlassMinus},
            {"ArrowsRotate", ElaIconType::ArrowsRotate},
            {"Expand", ElaIconType::Expand},
            {"Bookmark", ElaIconType::Bookmark},
            {"MagnifyingGlass", ElaIconType::MagnifyingGlass}};

        if (iconMap.contains(iconName)) {
            button->setIcon(
                ElaIcon::getInstance()->getElaIcon(iconMap[iconName]));
        }
    }

    setButtonTooltip(button, tooltip, shortcut);
    button->setFixedSize(32, 32);
    return button;
}

void ToolBar::addSeparator() {
    ::ElaToolBar::addSeparator();  // Call base class, not self
}
