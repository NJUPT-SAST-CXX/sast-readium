#include "StyleManager.h"
#include <QFontDatabase>
#include <QWidget>
#include <QApplication>
#include "utils/Logger.h"

StyleManager& StyleManager::instance() {
    static StyleManager instance;
    return instance;
}

StyleManager::StyleManager() : m_currentTheme(Theme::Light) {
    Logger::instance().info(
        "[managers] StyleManager initialized with Light theme");
    updateColors();
}

void StyleManager::setTheme(Theme theme) {
    if (m_currentTheme != theme) {
        Logger::instance().info("[managers] Changing theme from {} to {}",
                                static_cast<int>(m_currentTheme),
                                static_cast<int>(theme));
        m_currentTheme = theme;
        updateColors();
        emit themeChanged(theme);
        Logger::instance().debug(
            "[managers] Theme change completed and signal emitted");
    }
}

void StyleManager::toggleTheme() {
    Theme newTheme = (m_currentTheme == Theme::Light) ? Theme::Dark : Theme::Light;
    setTheme(newTheme);
}

void StyleManager::setLightTheme() {
    setTheme(Theme::Light);
}

void StyleManager::setDarkTheme() {
    setTheme(Theme::Dark);
}

void StyleManager::updateColors() {
    Logger::instance().debug("[managers] Updating colors for theme: {}",
                             m_currentTheme == Theme::Light ? "Light" : "Dark");
    if (m_currentTheme == Theme::Light) {
        // 亮色主题
        m_primaryColor = QColor(0, 120, 212);       // 蓝色
        m_secondaryColor = QColor(96, 94, 92);      // 灰色
        m_backgroundColor = QColor(255, 255, 255);  // 白色
        m_surfaceColor = QColor(250, 250, 250);     // 浅灰
        m_textColor = QColor(32, 31, 30);           // 深灰
        m_textSecondaryColor = QColor(96, 94, 92);  // 中灰
        m_borderColor = QColor(225, 223, 221);      // 边框灰
        m_hoverColor = QColor(243, 242, 241);       // 悬停灰
        m_pressedColor = QColor(237, 235, 233);     // 按下灰
        m_accentColor = QColor(16, 110, 190);       // 强调蓝
    } else {
        // 暗色主题
        m_primaryColor = QColor(96, 205, 255);         // 亮蓝
        m_secondaryColor = QColor(152, 151, 149);      // 亮灰
        m_backgroundColor = QColor(32, 31, 30);        // 深灰
        m_surfaceColor = QColor(40, 39, 38);           // 表面灰
        m_textColor = QColor(255, 255, 255);           // 白色
        m_textSecondaryColor = QColor(200, 198, 196);  // 浅灰
        m_borderColor = QColor(72, 70, 68);            // 边框深灰
        m_hoverColor = QColor(50, 49, 48);             // 悬停深灰
        m_pressedColor = QColor(60, 58, 56);           // 按下深灰
        m_accentColor = QColor(118, 185, 237);         // 强调亮蓝
    }
}

QString StyleManager::getApplicationStyleSheet() const {
    return QString(R"(
        QMainWindow {
            background-color: %1;
            color: %2;
        }
        QWidget {
            background-color: %1;
            color: %2;
            font-family: "Segoe UI", Arial, sans-serif;
            font-size: 9pt;
        }
        QGroupBox {
            font-weight: bold;
            border: 1px solid %3;
            border-radius: %4px;
            margin-top: 8px;
            padding-top: 4px;
            background-color: %5;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 8px;
            padding: 0 4px 0 4px;
            color: %6;
        }
    )")
        .arg(backgroundColor().name())
        .arg(textColor().name())
        .arg(borderColor().name())
        .arg(borderRadius())
        .arg(surfaceColor().name())
        .arg(textSecondaryColor().name());
}

QString StyleManager::getToolbarStyleSheet() const {
    return QString(R"(
        QWidget#toolbar {
            background-color: %1;
            border-bottom: 1px solid %2;
            padding: %3px;
        }
    )")
        .arg(surfaceColor().name())
        .arg(borderColor().name())
        .arg(spacing());
}

QString StyleManager::getButtonStyleSheet() const {
    return createButtonStyle();
}

QString StyleManager::createButtonStyle() const {
    return QString(R"(
        QPushButton {
            background-color: %1;
            border: 1px solid %2;
            border-radius: %3px;
            color: %4;
            font-weight: 500;
            padding: 6px 12px;
        }
        QPushButton:hover {
            background-color: %7;
            border-color: %8;
        }
        QPushButton:pressed {
            background-color: %9;
            border-color: %8;
        }
        QPushButton:disabled {
            background-color: %1;
            border-color: %2;
            color: %10;
        }
        QPushButton:focus {
            border: 2px solid %8;
        }
    )")
        .arg(surfaceColor().name())
        .arg(borderColor().name())
        .arg(borderRadius())
        .arg(textColor().name())
        .arg(buttonMinWidth())
        .arg(buttonHeight())
        .arg(hoverColor().name())
        .arg(accentColor().name())
        .arg(pressedColor().name())
        .arg(textSecondaryColor().name());
}

QColor StyleManager::primaryColor() const { return m_primaryColor; }
QColor StyleManager::secondaryColor() const { return m_secondaryColor; }
QColor StyleManager::backgroundColor() const { return m_backgroundColor; }
QColor StyleManager::surfaceColor() const { return m_surfaceColor; }
QColor StyleManager::textColor() const { return m_textColor; }
QColor StyleManager::textSecondaryColor() const { return m_textSecondaryColor; }
QColor StyleManager::borderColor() const { return m_borderColor; }
QColor StyleManager::hoverColor() const { return m_hoverColor; }
QColor StyleManager::pressedColor() const { return m_pressedColor; }
QColor StyleManager::accentColor() const { return m_accentColor; }

QFont StyleManager::defaultFont() const {
    QFont font("Segoe UI", 9);
    return font;
}

QFont StyleManager::titleFont() const {
    QFont font("Segoe UI", 10);
    font.setBold(true);
    return font;
}

QFont StyleManager::buttonFont() const {
    QFont font("Segoe UI", 9);
    font.setWeight(QFont::Medium);
    return font;
}

QString StyleManager::getStatusBarStyleSheet() const {
    return QString(R"(
        QStatusBar {
            background-color: %1;
            border-top: 1px solid %2;
            color: %3;
            padding: 4px;
        }
        QStatusBar::item {
            border: none;
        }
        QStatusBar QLabel {
            color: %4;
            padding: 2px 8px;
        }
        QStatusBar QLineEdit {
            background-color: %5;
            border: 1px solid %2;
            border-radius: 3px;
            padding: 2px 6px;
            color: %3;
        }
        QStatusBar QLineEdit:focus {
            border-color: %6;
        }
    )")
        .arg(surfaceColor().name())
        .arg(borderColor().name())
        .arg(textColor().name())
        .arg(textSecondaryColor().name())
        .arg(backgroundColor().name())
        .arg(accentColor().name());
}

QString StyleManager::getPDFViewerStyleSheet() const {
    // 根据主题动态设置PDF查看器背景色
    QColor pdfBackgroundColor = (m_currentTheme == Theme::Light) 
                                ? QColor(245, 245, 245)  // 亮色主题使用浅灰
                                : QColor(30, 30, 30);    // 暗色主题使用深灰
    
    QColor pageBackgroundColor = (m_currentTheme == Theme::Light) 
                                ? QColor(255, 255, 255)  // 亮色主题PDF页面白色
                                : QColor(45, 45, 48);    // 暗色主题PDF页面深灰
    
    return QString(R"(
        QScrollArea#singlePageScrollArea {
            background-color: %1;
            border: none;
        }
        QScrollArea#continuousScrollArea {
            background-color: %1;
            border: none;
        }
        QScrollArea > QWidget > QWidget {
            background-color: %1;
        }
        QLabel#pdfPage {
            background-color: %2;
            border: 1px solid %3;
            border-radius: 4px;
            margin: 8px;
        }
        PDFPageWidget {
            background-color: %2;
            border: none;
        }
    )")
        .arg(pdfBackgroundColor.name())
        .arg(pageBackgroundColor.name())
        .arg(borderColor().name());
}

QString StyleManager::getScrollBarStyleSheet() const {
    return createScrollBarStyle();
}

void StyleManager::applyThemeStyleSheet(const QString& styleSheet) {
    // 获取主窗口并应用样式表
    QWidget* mainWindow = nullptr;
    
    // 首先尝试获取活动窗口
    mainWindow = QApplication::activeWindow();
    
    // 如果没有活动窗口，尝试获取所有顶级窗口中的第一个
    if (!mainWindow) {
        QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
        for (QWidget* widget : topLevelWidgets) {
            if (widget && widget->inherits("QMainWindow")) {
                mainWindow = widget;
                break;
            }
        }
    }
    
    // 最后的备选方案：获取任何顶级窗口
    if (!mainWindow) {
        QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
        if (!topLevelWidgets.isEmpty()) {
            mainWindow = topLevelWidgets.first();
        }
    }
    
    if (mainWindow) {
        mainWindow->setStyleSheet(styleSheet);
        Logger::instance().debug("[StyleManager] Applied theme stylesheet to window: {}, stylesheet length: {}", 
                                mainWindow->metaObject()->className(), styleSheet.length());
    } else {
        Logger::instance().warning("[StyleManager] No main window found to apply stylesheet");
    }
    
    // 发出样式表应用完成信号
    emit styleSheetApplied();
}

void StyleManager::forceApplyTheme(QWidget* widget, const QString& styleSheet) {
    if (widget) {
        widget->setStyleSheet(styleSheet);
        Logger::instance().debug("[StyleManager] Force applied theme to widget: {}",
                               widget->metaObject()->className());
        emit styleSheetApplied();
    } else {
        Logger::instance().warning("[StyleManager] Cannot force apply theme to null widget");
    }
}

QString StyleManager::createScrollBarStyle() const {
    return QString(R"(
        QScrollBar:vertical {
            background-color: %1;
            width: 12px;
            border: none;
            border-radius: 6px;
        }
        QScrollBar::handle:vertical {
            background-color: %2;
            border-radius: 6px;
            min-height: 20px;
            margin: 0px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: %3;
        }
        QScrollBar::handle:vertical:pressed {
            background-color: %4;
        }
        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar:horizontal {
            background-color: %1;
            height: 12px;
            border: none;
            border-radius: 6px;
        }
        QScrollBar::handle:horizontal {
            background-color: %2;
            border-radius: 6px;
            min-width: 20px;
            margin: 0px;
        }
        QScrollBar::handle:horizontal:hover {
            background-color: %3;
        }
        QScrollBar::handle:horizontal:pressed {
            background-color: %4;
        }
        QScrollBar::add-line:horizontal,
        QScrollBar::sub-line:horizontal {
            width: 0px;
        }
    )")
        .arg(surfaceColor().name())
        .arg(borderColor().name())
        .arg(textSecondaryColor().name())
        .arg(secondaryColor().name());
}
