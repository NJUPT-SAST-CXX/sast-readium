#include "RightSideBar.h"

// ElaWidgetTools
#include "ElaTabWidget.h"

// Static const member definitions
const int RightSideBar::minimumWidth;
const int RightSideBar::maximumWidth;
const int RightSideBar::defaultWidth;
const int RightSideBar::animationDuration;

// Panels - using relative paths from ui/core/ to ui/widgets/
#include "../widgets/AnnotationsPanel.h"
#include "../widgets/DebugLogPanel.h"
#include "../widgets/LayersPanel.h"
#include "../widgets/PropertiesPanel.h"
#include "../widgets/SearchPanel.h"

// Qt
#include <QApplication>
#include <QEvent>
#include <QPropertyAnimation>
#include <QSettings>
#include <QVBoxLayout>

// Logging
#include "logging/SimpleLogging.h"

// ============================================================================
// 构造和析构
// ============================================================================

RightSideBar::RightSideBar(QWidget* parent)
    : ElaDockWidget(parent),
      m_tabWidget(nullptr),
      m_propertiesPanel(nullptr),
      m_annotationsPanel(nullptr),
      m_layersPanel(nullptr),
      m_searchPanel(nullptr),
      m_debugPanel(nullptr),
      m_animation(nullptr),
      m_settings(nullptr),
      m_isCurrentlyVisible(true),
      m_preferredWidth(defaultWidth),
      m_lastWidth(defaultWidth) {
    SLOG_INFO("RightSideBar: Constructor started");

    initSettings();
    setupUi();
    initAnimation();
    restoreState();

    SLOG_INFO("RightSideBar: Constructor completed");
}

RightSideBar::~RightSideBar() {
    SLOG_INFO("RightSideBar: Destructor called");

    // Save state before destruction
    saveState();

    // Stop animation if running
    if (m_animation != nullptr) {
        m_animation->stop();
    }
}

// ============================================================================
// UI 初始化
// ============================================================================

void RightSideBar::setupUi() {
    setWindowTitle(tr("Right Sidebar"));

    // Set width constraints
    setMinimumWidth(minimumWidth);
    setMaximumWidth(maximumWidth);
    resize(m_preferredWidth, height());

    // Create a container widget for the dock widget content
    QWidget* containerWidget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(containerWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 创建标签页控件
    m_tabWidget = new ElaTabWidget(containerWidget);

    // 属性标签页 - Full implementation
    m_propertiesPanel = new PropertiesPanel(m_tabWidget);
    m_tabWidget->addTab(m_propertiesPanel, tr("Properties"));

    // 注释标签页 - Full implementation
    m_annotationsPanel = new AnnotationsPanel(m_tabWidget);
    m_tabWidget->addTab(m_annotationsPanel, tr("Annotations"));

    // 图层标签页 - Full implementation
    m_layersPanel = new LayersPanel(m_tabWidget);
    m_tabWidget->addTab(m_layersPanel, tr("Layers"));

    // 搜索标签页 - Full implementation
    m_searchPanel = new SearchPanel(m_tabWidget);
    m_tabWidget->addTab(m_searchPanel, tr("Search"));

    // 调试标签页 - Full implementation
    m_debugPanel = new DebugLogPanel(m_tabWidget);
    m_tabWidget->addTab(m_debugPanel, tr("Debug"));

    layout->addWidget(m_tabWidget);

    // Set the container widget as the dock widget's content
    setWidget(containerWidget);

    SLOG_INFO("RightSideBar: All panels initialized successfully");
}

// ============================================================================
// Tab Management
// ============================================================================

void RightSideBar::switchToTab(TabIndex index) {
    m_tabWidget->setCurrentIndex(static_cast<int>(index));
    emit tabChanged(index);
    SLOG_INFO("RightSideBar: Switched to tab " +
              QString::number(static_cast<int>(index)));
}

RightSideBar::TabIndex RightSideBar::currentTab() const {
    return static_cast<TabIndex>(m_tabWidget->currentIndex());
}

// ============================================================================
// Document Management
// ============================================================================

void RightSideBar::setDocument(Poppler::Document* document,
                               const QString& filePath) {
    if (!document) {
        SLOG_WARNING("RightSideBar::setDocument: Null document provided");
        clearDocument();
        return;
    }

    SLOG_INFO("RightSideBar: Setting document in all panels");

    // Set document in all panels
    m_propertiesPanel->setDocument(document, filePath);
    m_annotationsPanel->setDocument(document);
    m_layersPanel->setDocument(document);

    // Connect annotation panel navigation signal
    connect(m_annotationsPanel, &AnnotationsPanel::navigateToPage, this,
            &RightSideBar::navigateToPage, Qt::UniqueConnection);

    SLOG_INFO("RightSideBar: Document set successfully in all panels");
}

void RightSideBar::clearDocument() {
    SLOG_INFO("RightSideBar: Clearing document from all panels");

    m_propertiesPanel->clearDocument();
    m_annotationsPanel->clearDocument();
    m_layersPanel->clearDocument();
}

bool RightSideBar::hasDocument() const {
    return m_propertiesPanel->hasDocument();
}

// ============================================================================
// 事件处理
// ============================================================================

void RightSideBar::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    ElaDockWidget::changeEvent(event);
}

void RightSideBar::retranslateUi() {
    SLOG_INFO("RightSideBar: Retranslating UI");

    setWindowTitle(tr("Right Sidebar"));
    m_tabWidget->setTabText(PropertiesTab, tr("Properties"));
    m_tabWidget->setTabText(AnnotationsTab, tr("Annotations"));
    m_tabWidget->setTabText(LayersTab, tr("Layers"));
    m_tabWidget->setTabText(SearchTab, tr("Search"));
    m_tabWidget->setTabText(DebugTab, tr("Debug"));
}

// ============================================================================
// 可见性和宽度管理
// ============================================================================

void RightSideBar::show(bool animated) {
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

    SLOG_INFO("RightSideBar: Shown");
}

void RightSideBar::hide(bool animated) {
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

    SLOG_INFO("RightSideBar: Hidden");
}

void RightSideBar::toggleVisibility(bool animated) {
    if (m_isCurrentlyVisible) {
        hide(animated);
    } else {
        show(animated);
    }
}

void RightSideBar::setVisible(bool visible) {
    // Preserve existing semantics: non-animated by default for QWidget-style
    // calls
    setVisible(visible, /*animated=*/false);
}

void RightSideBar::setVisible(bool visible, bool animated) {
    if (visible) {
        show(animated);
    } else {
        hide(animated);
    }
}

void RightSideBar::setPreferredWidth(int width) {
    m_preferredWidth = qBound(minimumWidth, width, maximumWidth);

    if (m_isCurrentlyVisible) {
        setMaximumWidth(m_preferredWidth);
        resize(m_preferredWidth, height());
    }

    emit widthChanged(m_preferredWidth);
    SLOG_INFO("RightSideBar: Preferred width set");
}

void RightSideBar::saveState() {
    if (!m_settings) {
        return;
    }

    m_settings->beginGroup("RightSideBar");
    m_settings->setValue("visible", m_isCurrentlyVisible);
    m_settings->setValue("width", m_preferredWidth);
    m_settings->setValue("currentTab",
                         m_tabWidget ? m_tabWidget->currentIndex() : 0);
    m_settings->endGroup();

    SLOG_INFO("RightSideBar: State saved");
}

void RightSideBar::restoreState() {
    if (!m_settings) {
        return;
    }

    m_settings->beginGroup("RightSideBar");
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

    SLOG_INFO("RightSideBar: State restored");
}

// ============================================================================
// 初始化方法
// ============================================================================

void RightSideBar::initAnimation() {
    m_animation = new QPropertyAnimation(this, "maximumWidth", this);
    m_animation->setDuration(animationDuration);
    m_animation->setEasingCurve(QEasingCurve::InOutCubic);

    connect(m_animation, &QPropertyAnimation::finished, this,
            &RightSideBar::onAnimationFinished);

    SLOG_INFO("RightSideBar: Animation initialized");
}

void RightSideBar::initSettings() {
    m_settings = new QSettings(QApplication::organizationName(),
                               QApplication::applicationName(), this);

    SLOG_INFO("RightSideBar: Settings initialized");
}

void RightSideBar::onAnimationFinished() {
    if (!m_isCurrentlyVisible) {
        QWidget::setVisible(false);
    }
    emit visibilityChanged(m_isCurrentlyVisible);

    SLOG_INFO("RightSideBar: Animation finished");
}
