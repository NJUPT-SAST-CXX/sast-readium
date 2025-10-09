#include "RightSideBar.h"
#include <QApplication>
#include <QDebug>
#include <QEasingCurve>
#include <QLabel>
#include <QPropertyAnimation>
#include <QSettings>
#include <QSize>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include "../../managers/StyleManager.h"
#include "../widgets/DebugLogPanel.h"

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

void RightSideBar::initWindow() {
    setMinimumWidth(minimumWidth);
    setMaximumWidth(maximumWidth);
    resize(preferredWidth, height());
}

void RightSideBar::initContent() {
    tabWidget = new QTabWidget(this);

    QWidget* propertiesTab = createPropertiesTab();
    QWidget* toolsTab = createToolsTab();
    QWidget* debugTab = createDebugTab();

    tabWidget->addTab(propertiesTab, "属性");
    tabWidget->addTab(toolsTab, "工具");
    tabWidget->addTab(debugTab, "调试");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabWidget);
}

QWidget* RightSideBar::createPropertiesTab() {
    QWidget* propertiesTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(propertiesTab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    // 文档属性标签
    QLabel* titleLabel = new QLabel("文档属性", propertiesTab);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 12px;");
    layout->addWidget(titleLabel);

    // 占位内容
    QLabel* placeholderLabel =
        new QLabel("文档属性信息将在此显示", propertiesTab);
    placeholderLabel->setStyleSheet("color: gray; font-size: 10px;");
    placeholderLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(placeholderLabel);

    layout->addStretch();
    return propertiesTab;
}

QWidget* RightSideBar::createToolsTab() {
    QWidget* toolsTab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(toolsTab);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    // 工具标签
    QLabel* titleLabel = new QLabel("工具", toolsTab);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 12px;");
    layout->addWidget(titleLabel);

    // 占位内容
    QLabel* placeholderLabel = new QLabel("工具面板将在此显示", toolsTab);
    placeholderLabel->setStyleSheet("color: gray; font-size: 10px;");
    placeholderLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(placeholderLabel);

    layout->addStretch();
    return toolsTab;
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
    if (isCurrentlyVisible)
        return;

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
    if (!isCurrentlyVisible)
        return;

    lastWidth = width();  // 记住当前宽度
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
    if (!settings)
        return;

    settings->beginGroup("RightSideBar");
    settings->setValue("visible", isCurrentlyVisible);
    settings->setValue("width", preferredWidth);
    settings->endGroup();
}

void RightSideBar::restoreState() {
    if (!settings)
        return;

    settings->beginGroup("RightSideBar");
    bool visible = settings->value("visible", true).toBool();
    int width = settings->value("width", defaultWidth).toInt();
    settings->endGroup();

    setPreferredWidth(width);
    setVisible(visible, false);  // 恢复状态时不使用动画
}

void RightSideBar::onAnimationFinished() {
    if (!isCurrentlyVisible) {
        QWidget::setVisible(false);
    }
    emit visibilityChanged(isCurrentlyVisible);
}

void RightSideBar::applyTheme() {
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
            padding: 6px 12px;
            margin-right: 2px;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
        }
        QTabBar::tab:selected {
            background-color: %2;
            color: %5;
            border-bottom: 1px solid %2;
        }
        QTabBar::tab:hover:!selected {
            background-color: %6;
        }
    )")
                                 .arg(STYLE.borderColor().name())
                                 .arg(STYLE.backgroundColor().name())
                                 .arg(STYLE.surfaceColor().name())
                                 .arg(STYLE.textSecondaryColor().name())
                                 .arg(STYLE.textColor().name())
                                 .arg(STYLE.hoverColor().name());

    if (tabWidget) {
        tabWidget->setStyleSheet(tabWidgetStyle);
    }

    // Apply general widget styling
    setStyleSheet(QString(R"(
        RightSideBar {
            background-color: %1;
            border-left: 1px solid %2;
        }
        QLabel {
            color: %3;
        }
    )")
                      .arg(STYLE.backgroundColor().name())
                      .arg(STYLE.borderColor().name())
                      .arg(STYLE.textColor().name()));
}
