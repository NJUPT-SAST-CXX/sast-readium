#include "SideBar.h"

// ElaWidgetTools
#include "ElaTabWidget.h"

// Static const member definitions
const int SideBar::minimumWidth;
const int SideBar::maximumWidth;
const int SideBar::defaultWidth;
const int SideBar::animationDuration;

// Qt
#include <QAccessible>
#include <QApplication>
#include <QEvent>
#include <QGuiApplication>
#include <QPropertyAnimation>
#include <QSettings>
#include <QVBoxLayout>

// Logging
#include "logging/SimpleLogging.h"

// Panels - using relative paths from ui/core/ to ui/widgets/
#include "../widgets/BookmarkPanel.h"
#include "../widgets/OutlinePanel.h"
#include "../widgets/ThumbnailPanel.h"

// Legacy/compat UI types expected by some tests
#include "../thumbnail/ThumbnailListView.h"
#include "../viewer/PDFOutlineWidget.h"

// Business logic
#include "model/BookmarkModel.h"
#include "model/PDFOutlineModel.h"
#include "model/ThumbnailModel.h"

SideBar::SideBar(QWidget* parent)
    : ElaDockWidget(parent),
      m_tabWidget(nullptr),
      m_thumbnailPanel(nullptr),
      m_bookmarkPanel(nullptr),
      m_outlinePanel(nullptr),
      m_thumbnailModel(nullptr),
      m_bookmarkModel(nullptr),
      m_outlineModel(nullptr),
      m_document(nullptr),
      m_currentPage(1),
      m_animation(nullptr),
      m_settings(nullptr),
      m_isCurrentlyVisible(true),
      m_preferredWidth(defaultWidth),
      m_lastWidth(defaultWidth) {
    SLOG_INFO("SideBar: Constructor started");

    initSettings();
    setupUi();
    // Initialize backward-compatibility adapters (not added to layout)
    m_compatOutlineWidget = new PDFOutlineWidget(this);
    connect(m_compatOutlineWidget, &PDFOutlineWidget::pageNavigationRequested,
            this, &SideBar::pageJumpRequested);
    connect(m_compatOutlineWidget, &PDFOutlineWidget::pageNavigationRequested,
            this, &SideBar::pageClicked);

    m_compatThumbnailView = new ThumbnailListView(this);
    // Bridge generic clicked signal to legacy pageClicked with a simple mapping
    connect(
        m_compatThumbnailView, &QListView::clicked, this,
        [this](const QModelIndex& idx) { emit pageClicked(idx.row() + 1); });

    setupTabs();

    // Provide a default thumbnail model so integration tests and legacy
    // adapters can query it immediately
    m_thumbnailModel = new ThumbnailModel(this);
    m_thumbnailPanel->setThumbnailModel(m_thumbnailModel);
    if (m_compatThumbnailView) {
        m_compatThumbnailView->setThumbnailModel(m_thumbnailModel);
    }

    connectSignals();
    initAnimation();
    restoreState();

    SLOG_INFO("SideBar: Constructor completed");
}

SideBar::~SideBar() {
    SLOG_INFO("SideBar: Destructor called");

    // Save state before destruction
    saveState();

    // Proactively disconnect to avoid late signal deliveries during teardown
    disconnect(this, nullptr, nullptr, nullptr);

    // Ensure child panels and compat widgets don't emit to us during teardown
    if (m_thumbnailPanel) {
        m_thumbnailPanel->disconnect(this);
    }
    if (m_bookmarkPanel) {
        m_bookmarkPanel->disconnect(this);
    }
    if (m_outlinePanel) {
        m_outlinePanel->disconnect(this);
    }
    if (m_compatThumbnailView) {
        m_compatThumbnailView->disconnect(this);
    }
    if (m_compatOutlineWidget) {
        m_compatOutlineWidget->disconnect(this);
    }

    // Stop and delete animation safely
    if (m_animation) {
        m_animation->stop();
        m_animation->disconnect(this);
        delete m_animation;
        m_animation = nullptr;
    }
}

void SideBar::setupUi() {
    // Set width constraints
    setMinimumWidth(minimumWidth);
    setMaximumWidth(maximumWidth);
    resize(m_preferredWidth, height());

    auto* centralWidget = new QWidget(this);
    auto* layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_tabWidget = new ElaTabWidget(this);
    layout->addWidget(m_tabWidget);

    setWidget(centralWidget);

    const bool isOffscreen = (QGuiApplication::platformName() == "offscreen");
    if (!isOffscreen) {
        setWindowTitle(tr("Sidebar"));
    } else {
        setAccessibleName(tr("Sidebar"));
    }
}

void SideBar::setupTabs() {
    // 缩略图标签页
    m_thumbnailPanel = new ThumbnailPanel(this);
    m_tabWidget->addTab(m_thumbnailPanel, tr("Thumbnails"));

    // 书签标签页
    m_bookmarkPanel = new BookmarkPanel(this);
    m_tabWidget->addTab(m_bookmarkPanel, tr("Bookmarks"));

    // 大纲标签页
    m_outlinePanel = new OutlinePanel(this);
    m_tabWidget->addTab(m_outlinePanel, tr("Outline"));
}

void SideBar::connectSignals() {
    // 标签页切换
    connect(
        m_tabWidget, &ElaTabWidget::currentChanged, this,
        [this](int index) { emit tabChanged(static_cast<TabIndex>(index)); });

    // 缩略图面板信号
    connect(m_thumbnailPanel, &ThumbnailPanel::pageSelected, this,
            &SideBar::pageJumpRequested);
    // Also emit legacy signal for compatibility
    connect(m_thumbnailPanel, &ThumbnailPanel::pageSelected, this,
            &SideBar::pageClicked);

    // 书签面板信号
    connect(m_bookmarkPanel, &BookmarkPanel::bookmarkSelected, this,
            &SideBar::pageJumpRequested);
    connect(m_bookmarkPanel, &BookmarkPanel::bookmarkAdded, this,
            &SideBar::bookmarkAdded);
    connect(m_bookmarkPanel, &BookmarkPanel::bookmarkRemoved, this,
            &SideBar::bookmarkRemoved);

    // 大纲面板信号
    connect(m_outlinePanel, &OutlinePanel::outlineItemClicked, this,
            &SideBar::pageJumpRequested);
    // Also emit legacy signal for compatibility
    connect(m_outlinePanel, &OutlinePanel::outlineItemClicked, this,
            &SideBar::pageClicked);
}

void SideBar::setDocument(std::shared_ptr<Poppler::Document> document) {
    SLOG_INFO("SideBar: Setting document");

    m_document = document;

    m_thumbnailPanel->setDocument(document);
    m_bookmarkPanel->setDocument(document);
    m_outlinePanel->setDocument(document);
}

void SideBar::clearDocument() {
    SLOG_INFO("SideBar: Clearing document");

    m_document.reset();

    m_thumbnailPanel->clearDocument();
    m_bookmarkPanel->clearDocument();
    m_outlinePanel->clearDocument();
}

void SideBar::switchToTab(TabIndex index) {
    m_tabWidget->setCurrentIndex(static_cast<int>(index));
}

void SideBar::setThumbnailSize(const QSize& size) {
    // Use the larger dimension to maintain aspect in panel-based API
    setThumbnailSize(qMax(size.width(), size.height()));
    if (m_compatThumbnailView) {
        m_compatThumbnailView->setThumbnailSize(size);
    }
}

SideBar::TabIndex SideBar::currentTab() const {
    return static_cast<TabIndex>(m_tabWidget->currentIndex());
}

ElaTabWidget* SideBar::tabWidget() const { return m_tabWidget; }

void SideBar::setCurrentPage(int pageNumber) {
    m_currentPage = pageNumber;
    m_thumbnailPanel->setCurrentPage(pageNumber);
}

void SideBar::refreshThumbnails() { m_thumbnailPanel->refresh(); }

void SideBar::setThumbnailSize(int size) {
    m_thumbnailPanel->setThumbnailSize(size);
}

void SideBar::addBookmark(int pageNumber, const QString& title) {
    m_bookmarkPanel->addBookmark(pageNumber, title);
}

void SideBar::removeBookmark(int pageNumber) {
    m_bookmarkPanel->removeBookmark(pageNumber);
}

void SideBar::clearBookmarks() { m_bookmarkPanel->clearBookmarks(); }

bool SideBar::exportBookmarks(const QString& filePath) {
    return m_bookmarkPanel->exportBookmarks(filePath);
}

bool SideBar::importBookmarks(const QString& filePath) {
    return m_bookmarkPanel->importBookmarks(filePath);
}

void SideBar::refreshOutline() { m_outlinePanel->refresh(); }

void SideBar::expandAllOutline() { m_outlinePanel->expandAll(); }

void SideBar::collapseAllOutline() { m_outlinePanel->collapseAll(); }

void SideBar::setThumbnailModel(ThumbnailModel* model) {
    m_thumbnailModel = model;
    m_thumbnailPanel->setThumbnailModel(model);
    // Keep legacy adapter in sync
    if (m_compatThumbnailView) {
        m_compatThumbnailView->setThumbnailModel(model);
    }
}

void SideBar::setBookmarkModel(BookmarkModel* model) {
    m_bookmarkModel = model;
    // BookmarkPanel 内部创建自己的模型
}

void SideBar::setOutlineModel(PDFOutlineModel* model) {
    m_outlineModel = model;
    m_outlinePanel->setOutlineModel(model);
    // Keep legacy adapter in sync
    if (m_compatOutlineWidget) {
        m_compatOutlineWidget->setOutlineModel(model);
    }
}

// ============================================================================
// 可见性和宽度管理
// ============================================================================

void SideBar::show(bool animated) {
    if (m_isCurrentlyVisible) {
        return;
    }

    m_isCurrentlyVisible = true;
    QWidget::setVisible(true);

    if (animated && m_animation) {
        m_animation->setStartValue(0);
        m_animation->setEndValue(m_preferredWidth);
        m_animation->start();
    } else {
        setMaximumWidth(m_preferredWidth);
        emit visibilityChanged(true);
    }

    SLOG_INFO("SideBar: Shown");
}

void SideBar::hide(bool animated) {
    if (!m_isCurrentlyVisible) {
        return;
    }

    m_lastWidth = width();  // Remember current width
    m_isCurrentlyVisible = false;

    if (animated && m_animation) {
        m_animation->setStartValue(width());
        m_animation->setEndValue(0);
        m_animation->start();
    } else {
        setMaximumWidth(0);
        QWidget::setVisible(false);
        emit visibilityChanged(false);
    }

    SLOG_INFO("SideBar: Hidden");
}

void SideBar::toggleVisibility(bool animated) {
    if (m_isCurrentlyVisible) {
        hide(animated);
    } else {
        show(animated);
    }
}

void SideBar::setVisible(bool visible, bool animated) {
    // Backward-compatible convenience: choose show/hide with optional animation
    if (visible) {
        show(animated);
    } else {
        hide(animated);
    }
}

void SideBar::setVisible(bool visible) {
    // Delegate to the animated-aware variant with animation disabled by default
    setVisible(visible, /*animated=*/false);
}

void SideBar::setPreferredWidth(int width) {
    const int clampedWidth = qBound(minimumWidth, width, maximumWidth);

    if (clampedWidth == m_preferredWidth) {
        return;
    }

    m_preferredWidth = clampedWidth;

    if (m_isCurrentlyVisible) {
        setMaximumWidth(m_preferredWidth);
        resize(m_preferredWidth, height());
    }

    emit widthChanged(m_preferredWidth);
    SLOG_INFO_F("SideBar: Preferred width set to {}", m_preferredWidth);
}

void SideBar::saveState() {
    if (!m_settings) {
        return;
    }

    m_settings->beginGroup("SideBar");
    m_settings->setValue("visible", m_isCurrentlyVisible);
    m_settings->setValue("width", m_preferredWidth);
    m_settings->setValue("currentTab",
                         m_tabWidget ? m_tabWidget->currentIndex() : 0);
    m_settings->endGroup();

    SLOG_INFO("SideBar: State saved");
}

void SideBar::restoreState() {
    if (!m_settings) {
        return;
    }

    m_settings->beginGroup("SideBar");
    m_isCurrentlyVisible = m_settings->value("visible", true).toBool();
    m_preferredWidth = m_settings->value("width", defaultWidth).toInt();
    m_preferredWidth = qBound(minimumWidth, m_preferredWidth, maximumWidth);
    int currentTab = m_settings->value("currentTab", 0).toInt();
    m_settings->endGroup();

    // Apply restored state
    if (m_tabWidget && currentTab >= 0 && currentTab < m_tabWidget->count()) {
        m_tabWidget->setCurrentIndex(currentTab);
    }

    // Set visibility without animation when restoring
    if (m_isCurrentlyVisible) {
        QWidget::setVisible(true);
        setMaximumWidth(m_preferredWidth);
    } else {
        setMaximumWidth(0);
        QWidget::setVisible(false);
    }

    SLOG_INFO("SideBar: State restored");
}

// ============================================================================
// 初始化方法
// ============================================================================

void SideBar::initAnimation() {
    m_animation = new QPropertyAnimation(this, "maximumWidth", this);
    m_animation->setDuration(animationDuration);
    m_animation->setEasingCurve(QEasingCurve::InOutCubic);

    connect(m_animation, &QPropertyAnimation::finished, this,
            &SideBar::onAnimationFinished);

    SLOG_INFO("SideBar: Animation initialized");
}

void SideBar::initSettings() {
    m_settings = new QSettings(QApplication::organizationName(),
                               QApplication::applicationName(), this);

    SLOG_INFO("SideBar: Settings initialized");
}

void SideBar::onAnimationFinished() {
    if (!m_isCurrentlyVisible) {
        QWidget::setVisible(false);
    }
    emit visibilityChanged(m_isCurrentlyVisible);

    SLOG_INFO("SideBar: Animation finished");
}

// ============================================================================
// 其他方法
// ============================================================================

void SideBar::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    ElaDockWidget::changeEvent(event);
}

void SideBar::retranslateUi() {
    SLOG_INFO("SideBar: Retranslating UI");

    const bool isOffscreen = (QGuiApplication::platformName() == "offscreen");
    if (!isOffscreen) {
        setWindowTitle(tr("Sidebar"));
    } else {
        setAccessibleName(tr("Sidebar"));
    }
    m_tabWidget->setTabText(ThumbnailsTab, tr("Thumbnails"));
    m_tabWidget->setTabText(BookmarksTab, tr("Bookmarks"));
    m_tabWidget->setTabText(OutlineTab, tr("Outline"));
}
