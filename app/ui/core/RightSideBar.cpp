#include "RightSideBar.h"
#include <QApplication>
#include <QDebug>
#include <QEasingCurve>
#include <QLabel>
#include <QPropertyAnimation>
#include <QScrollArea>
#include <QSettings>
#include <QSize>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include "../../logging/LoggingMacros.h"
#include "../../managers/StyleManager.h"
#include "../dialogs/DocumentMetadataDialog.h"
#include "../widgets/AnnotationToolbar.h"
#include "../widgets/DebugLogPanel.h"
#include "../widgets/DocumentPropertiesPanel.h"
#include "../widgets/SearchWidget.h"

// 定义静态常量
const int RightSideBar::minimumWidth;
const int RightSideBar::maximumWidth;
const int RightSideBar::defaultWidth;
const int RightSideBar::animationDuration;

RightSideBar::RightSideBar(QWidget* parent)
    : QWidget(parent),
      animation(nullptr),
      settings(nullptr),
      debugLogPanel(nullptr),
      m_propertiesPanel(nullptr),
      m_annotationToolbar(nullptr),
      m_searchWidget(nullptr),
      isCurrentlyVisible(true),
      preferredWidth(defaultWidth),
      lastWidth(defaultWidth) {
    initSettings();
    initWindow();
    initContent();
    initAnimation();
    restoreState();

    // Connect to theme changes
    connect(&STYLE, &StyleManager::themeChanged, this, [this](Theme theme) {
        Q_UNUSED(theme)
        applyTheme();
    });

    // Apply initial theme
    applyTheme();
}

RightSideBar::~RightSideBar() {
    // Save state before destruction
    saveState();

    // Stop animation if running
    if (animation) {
        animation->stop();
        // Animation will be deleted by Qt parent-child ownership
    }

    // All widgets are deleted automatically by Qt parent-child ownership
    // No manual deletion needed for widgets created with 'this' as parent

    LOG_DEBUG("RightSideBar destroyed successfully");
}

void RightSideBar::initWindow() {
    // Set object name for QSS styling
    setObjectName("RightSideBar");

    setMinimumWidth(minimumWidth);
    setMaximumWidth(maximumWidth);
    resize(preferredWidth, height());

    // Set size policy for responsive behavior
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
}

void RightSideBar::initContent() {
    tabWidget = new QTabWidget(this);
    tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QWidget* propertiesTab = createPropertiesTab();
    QWidget* toolsTab = createToolsTab();
    QWidget* searchTab = createSearchTab();
    QWidget* debugTab = createDebugTab();

    tabWidget->addTab(propertiesTab, tr("Properties"));
    tabWidget->addTab(toolsTab, tr("Tools"));
    tabWidget->addTab(searchTab, tr("Search"));
    tabWidget->addTab(debugTab, tr("Debug"));

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);  // No margins for sidebar
    mainLayout->setSpacing(0);                   // No spacing for tight layout
    mainLayout->addWidget(tabWidget);
}

QWidget* RightSideBar::createPropertiesTab() {
    QWidget* propertiesTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(propertiesTab);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Create document properties panel
    m_propertiesPanel = new DocumentPropertiesPanel(propertiesTab);
    layout->addWidget(m_propertiesPanel);

    // Connect signal to forward to parent
    connect(m_propertiesPanel,
            &DocumentPropertiesPanel::viewFullDetailsRequested, this,
            &RightSideBar::onViewFullDetailsRequested);

    return propertiesTab;
}

QWidget* RightSideBar::createToolsTab() {
    QWidget* toolsTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(toolsTab);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Create scroll area for annotation toolbar
    QScrollArea* scrollArea = new QScrollArea(toolsTab);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Create annotation toolbar
    m_annotationToolbar = new AnnotationToolbar(scrollArea);
    scrollArea->setWidget(m_annotationToolbar);

    layout->addWidget(scrollArea);

    return toolsTab;
}

QWidget* RightSideBar::createSearchTab() {
    QWidget* searchTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(searchTab);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Create search widget
    m_searchWidget = new SearchWidget(searchTab);
    layout->addWidget(m_searchWidget);

    return searchTab;
}

QWidget* RightSideBar::createDebugTab() {
    // Create the debug log panel
    debugLogPanel = new DebugLogPanel();

    // The DebugLogPanel is already a complete widget, so we can return it
    // directly
    return debugLogPanel;
}

void RightSideBar::initAnimation() {
    animation = new QPropertyAnimation(this, "maximumWidth", this);
    animation->setDuration(animationDuration);
    animation->setEasingCurve(QEasingCurve::OutCubic);

    connect(animation, &QPropertyAnimation::finished, this,
            &RightSideBar::onAnimationFinished);
}

void RightSideBar::initSettings() { settings = new QSettings(this); }

bool RightSideBar::isVisible() const { return isCurrentlyVisible; }

void RightSideBar::setVisible(bool visible, bool animated) {
    if (visible) {
        show(animated);
    } else {
        hide(animated);
    }
}

void RightSideBar::toggleVisibility(bool animated) {
    setVisible(!isCurrentlyVisible, animated);
}

void RightSideBar::show(bool animated) {
    if (isCurrentlyVisible) {
        return;
    }

    isCurrentlyVisible = true;
    QWidget::setVisible(true);

    if (animated && animation) {
        animation->setStartValue(0);
        animation->setEndValue(preferredWidth);
        animation->start();
    } else {
        setMaximumWidth(preferredWidth);
        emit visibilityChanged(true);
    }
}

void RightSideBar::hide(bool animated) {
    if (!isCurrentlyVisible) {
        return;
    }

    lastWidth = width();  // Remember current width
    isCurrentlyVisible = false;

    if (animated && animation) {
        animation->setStartValue(width());
        animation->setEndValue(0);
        animation->start();
    } else {
        setMaximumWidth(0);
        QWidget::setVisible(false);
        emit visibilityChanged(false);
    }
}

int RightSideBar::getPreferredWidth() const { return preferredWidth; }

void RightSideBar::setPreferredWidth(int width) {
    preferredWidth = qBound(minimumWidth, width, maximumWidth);
    if (isCurrentlyVisible) {
        setMaximumWidth(preferredWidth);
        resize(preferredWidth, height());
    }
    emit widthChanged(preferredWidth);
}

void RightSideBar::saveState() {
    if (!settings) {
        return;
    }

    settings->beginGroup("RightSideBar");
    settings->setValue("visible", isCurrentlyVisible);
    settings->setValue("width", preferredWidth);
    settings->endGroup();
}

void RightSideBar::restoreState() {
    if (!settings) {
        return;
    }

    settings->beginGroup("RightSideBar");
    bool visible = settings->value("visible", true).toBool();
    int width = settings->value("width", defaultWidth).toInt();
    settings->endGroup();

    setPreferredWidth(width);
    setVisible(visible, false);  // Don't use animation when restoring state
}

void RightSideBar::onAnimationFinished() {
    if (!isCurrentlyVisible) {
        QWidget::setVisible(false);
    }
    emit visibilityChanged(isCurrentlyVisible);
}

void RightSideBar::applyTheme() {
    // Create disabled colors similar to StyleManager's approach
    QColor disabledBg = STYLE.surfaceAltColor();
    QColor disabledText = STYLE.textSecondaryColor();

    if (STYLE.currentTheme() == Theme::Dark) {
        disabledBg = disabledBg.darker(135);
        disabledText = disabledText.darker(110);
    } else {
        disabledBg = disabledBg.lighter(104);
    }

    // Apply consistent styling using StyleManager
    QString tabWidgetStyle = QString(R"(
        QTabWidget::pane {
            border: 1px solid %1;
            background-color: %2;
            border-radius: 4px;
        }
        QTabWidget::tab-bar {
            alignment: center;
        }
        QTabBar::tab {
            background-color: %3;
            color: %4;
            border: 1px solid %1;
            padding: 8px 14px;
            margin-right: 2px;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
            min-width: 60px;
        }
        QTabBar::tab:selected {
            background-color: %2;
            color: %5;
            border-bottom: 1px solid %2;
            font-weight: bold;
        }
        QTabBar::tab:hover:!selected {
            background-color: %6;
            color: %5;
            transition: background-color 0.2s ease;
        }
        QTabBar::tab:pressed {
            background-color: %7;
        }
    )")
                                 .arg(STYLE.borderColor().name())
                                 .arg(STYLE.backgroundColor().name())
                                 .arg(STYLE.surfaceColor().name())
                                 .arg(STYLE.textSecondaryColor().name())
                                 .arg(STYLE.textColor().name())
                                 .arg(STYLE.hoverColor().name())
                                 .arg(STYLE.pressedColor().name());

    if (tabWidget) {
        tabWidget->setStyleSheet(tabWidgetStyle);
    }

    // Apply general widget styling with enhanced visual feedback
    setStyleSheet(QString(R"(
        RightSideBar {
            background-color: %1;
            border-left: 1px solid %2;
        }
        QLabel {
            color: %3;
        }
        QPushButton {
            background-color: %4;
            color: %3;
            border: 1px solid %2;
            border-radius: 4px;
            padding: 6px 12px;
            min-height: 20px;
        }
        QPushButton:hover {
            background-color: %5;
            border-color: %6;
        }
        QPushButton:pressed {
            background-color: %7;
        }
        QPushButton:disabled {
            background-color: %8;
            color: %9;
            border-color: %8;
        }
        QGroupBox {
            font-weight: bold;
            border: 1px solid %2;
            border-radius: 4px;
            margin-top: 8px;
            padding-top: 4px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 8px;
            padding: 0 4px 0 4px;
        }
    )")
                      .arg(STYLE.backgroundColor().name())
                      .arg(STYLE.borderColor().name())
                      .arg(STYLE.textColor().name())
                      .arg(STYLE.surfaceColor().name())
                      .arg(STYLE.hoverColor().name())
                      .arg(STYLE.accentColor().name())
                      .arg(STYLE.pressedColor().name())
                      .arg(disabledBg.name())
                      .arg(disabledText.name()));
}

void RightSideBar::setDocument(Poppler::Document* document,
                               const QString& filePath) {
    if (m_propertiesPanel) {
        m_propertiesPanel->setDocument(document, filePath);
    }

    if (m_searchWidget) {
        m_searchWidget->setDocument(document);
    }
}

void RightSideBar::clearDocument() {
    if (m_propertiesPanel) {
        m_propertiesPanel->clearProperties();
    }

    if (m_searchWidget) {
        m_searchWidget->clearSearch();
    }
}

void RightSideBar::onViewFullDetailsRequested(Poppler::Document* document,
                                              const QString& filePath) {
    // Create and show the full metadata dialog
    DocumentMetadataDialog* dialog = new DocumentMetadataDialog(this);
    dialog->setDocument(document, filePath);
    dialog->exec();
    dialog->deleteLater();
}
