#include "StyleManager.h"
#include <QApplication>
#include <QFile>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QTextStream>
#include "../logging/Logger.h"

// Private implementation class
class StyleManagerImpl {
public:
    StyleManagerImpl() {
        updateColors();
        loadQssFiles();
    }

    void updateColors();
    [[nodiscard]] QString createButtonStyle() const;
    [[nodiscard]] QString createScrollBarStyle() const;
    void loadQssFiles();
    QString loadQssFile(const QString& resourcePath);
    [[nodiscard]] QString getQssStyleSheet() const;

    Theme m_currentTheme{Theme::Light};

    // QSS file caching
    QString m_cachedLightQss;
    QString m_cachedDarkQss;
    QString m_cachedThumbnailsQss;
    QString m_cachedCommonQss;

    // 颜色定义
    QColor m_primaryColor;
    QColor m_secondaryColor;
    QColor m_backgroundColor;
    QColor m_surfaceColor;
    QColor m_surfaceAltColor;
    QColor m_elevatedSurfaceColor;
    QColor m_overlayColor;
    QColor m_textColor;
    QColor m_textSecondaryColor;
    QColor m_borderColor;
    QColor m_mutedBorderColor;
    QColor m_hoverColor;
    QColor m_pressedColor;
    QColor m_accentColor;
    QColor m_focusColor;
};

StyleManager& StyleManager::instance() {
    static StyleManager instance;
    return instance;
}

StyleManager::StyleManager() : m_pImpl(std::make_unique<StyleManagerImpl>()) {
    Logger::instance().info(
        "[managers] StyleManager initialized with Light theme");
}

StyleManager::~StyleManager() = default;

Theme StyleManager::currentTheme() const { return m_pImpl->m_currentTheme; }

void StyleManager::setTheme(Theme theme) {
    if (m_pImpl->m_currentTheme != theme) {
        Logger::instance().info("[managers] Changing theme from {} to {}",
                                static_cast<int>(m_pImpl->m_currentTheme),
                                static_cast<int>(theme));
        m_pImpl->m_currentTheme = theme;
        m_pImpl->updateColors();
        emit themeChanged(theme);
        Logger::instance().debug(
            "[managers] Theme change completed and signal emitted");
    }
}

void StyleManagerImpl::updateColors() {
    Logger::instance().debug("[managers] Updating colors for theme: {}",
                             m_currentTheme == Theme::Light ? "Light" : "Dark");
    if (m_currentTheme == Theme::Light) {
        // 亮色主题
        m_primaryColor = QColor(0, 120, 212);       // 蓝色
        m_secondaryColor = QColor(96, 94, 92);      // 灰色
        m_backgroundColor = QColor(255, 255, 255);  // 白色
        m_surfaceColor = QColor(250, 250, 252);     // 浅灰
        m_surfaceAltColor = QColor(244, 246, 249);
        m_elevatedSurfaceColor = QColor(255, 255, 255);
        m_overlayColor = QColor(255, 255, 255, 235);
        m_textColor = QColor(32, 31, 30);           // 深灰
        m_textSecondaryColor = QColor(96, 94, 92);  // 中灰
        m_borderColor = QColor(225, 223, 221);      // 边框灰
        m_mutedBorderColor = QColor(210, 214, 220);
        m_hoverColor = QColor(243, 242, 241);    // 悬停灰
        m_pressedColor = QColor(237, 235, 233);  // 按下灰
        m_accentColor = QColor(16, 110, 190);    // 强调蓝
        m_focusColor = QColor(0, 99, 191);
    } else {
        // 暗色主题
        m_primaryColor = QColor(96, 205, 255);     // 亮蓝
        m_secondaryColor = QColor(152, 151, 149);  // 亮灰
        m_backgroundColor = QColor(26, 28, 33);    // 深灰
        m_surfaceColor = QColor(34, 36, 41);       // 表面灰
        m_surfaceAltColor = QColor(40, 42, 48);
        m_elevatedSurfaceColor = QColor(44, 46, 53);
        m_overlayColor = QColor(7, 9, 12, 220);
        m_textColor = QColor(246, 247, 249);           // 白色
        m_textSecondaryColor = QColor(188, 192, 198);  // 浅灰
        m_borderColor = QColor(64, 66, 71);            // 边框深灰
        m_mutedBorderColor = QColor(74, 76, 84);
        m_hoverColor = QColor(56, 58, 65);      // 悬停深灰
        m_pressedColor = QColor(48, 50, 58);    // 按下深灰
        m_accentColor = QColor(118, 185, 237);  // 强调亮蓝
        m_focusColor = QColor(104, 173, 255);
    }
}

QString StyleManager::getApplicationStyleSheet() const {
    // Start with QSS file content for comprehensive widget styling
    QString stylesheet = m_pImpl->getQssStyleSheet();

    // Append minimal programmatic styles only for components requiring dynamic
    // color injection QSS files handle the majority of styling - these are only
    // for elements that need runtime theme color access that QSS cannot provide
    stylesheet += QString(R"(

        /* Programmatic styles for dynamic color injection */
        QGroupBox {
            font-weight: bold;
            border: 1px solid %1;
            border-radius: %2px;
            margin-top: 8px;
            padding-top: 4px;
            background-color: %3;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 8px;
            padding: 0 4px 0 4px;
            color: %4;
        }
    )")
                      .arg(borderColor().name())
                      .arg(borderRadius())
                      .arg(surfaceColor().name())
                      .arg(textSecondaryColor().name());

    return stylesheet;
}

QString StyleManager::getToolbarStyleSheet() const {
    return QString(R"(
        QToolBar {
            background-color: %1;
            border: none;
            border-bottom: 1px solid %2;
            padding: %3px %4px;
        }
        QToolBar QToolButton {
            border-radius: %5px;
            padding: 6px;
        }
        QToolBar::separator {
            background-color: %2;
            width: 1px;
            margin: %6px;
        }
    )")
        .arg(surfaceColor().name())
        .arg(mutedBorderColor().name())
        .arg(spacingSM())
        .arg(spacingMD())
        .arg(radiusMD())
        .arg(spacingSM());
}

QString StyleManager::getButtonStyleSheet() const {
    return createButtonStyle();
}

QString StyleManager::createButtonStyle() const {
    QColor focusShadow = focusColor();
    focusShadow.setAlphaF(0.25);

    QColor disabledBg = surfaceAltColor();
    if (m_pImpl->m_currentTheme == Theme::Dark) {
        disabledBg = disabledBg.darker(135);
    } else {
        disabledBg = disabledBg.lighter(104);
    }

    QString transition = getTransitionStyle(
        "background-color, border-color, box-shadow", animationFast());

    return QString(R"(
        QPushButton {
            background-color: %1;
            border: 1px solid %2;
            border-radius: %3px;
            color: %4;
            font-weight: 500;
            padding: 6px 14px;
            min-width: %5px;
            min-height: %6px;
            %7
        }
        QPushButton:hover {
            background-color: %8;
            border-color: %9;
        }
        QPushButton:pressed {
            background-color: %10;
            border-color: %9;
        }
        QPushButton:disabled {
            background-color: %11;
            border-color: %12;
            color: %13;
        }
        QPushButton:focus {
            border: 2px solid %9;
            padding: 5px 13px;
            box-shadow: 0 0 0 3px %14;
        }
    )")
        .arg(elevatedSurfaceColor().name())
        .arg(mutedBorderColor().name())
        .arg(radiusLG())
        .arg(textColor().name())
        .arg(buttonMinWidth())
        .arg(buttonHeight())
        .arg(transition)
        .arg(surfaceAltColor().name())
        .arg(accentColor().name())
        .arg(pressedColor().name())
        .arg(disabledBg.name())
        .arg(mutedBorderColor().name())
        .arg(textSecondaryColor().name())
        .arg(focusShadow.name(QColor::HexArgb));
}

QColor StyleManager::primaryColor() const { return m_pImpl->m_primaryColor; }
QColor StyleManager::secondaryColor() const {
    return m_pImpl->m_secondaryColor;
}
QColor StyleManager::backgroundColor() const {
    return m_pImpl->m_backgroundColor;
}
QColor StyleManager::surfaceColor() const { return m_pImpl->m_surfaceColor; }
QColor StyleManager::surfaceAltColor() const {
    return m_pImpl->m_surfaceAltColor;
}
QColor StyleManager::elevatedSurfaceColor() const {
    return m_pImpl->m_elevatedSurfaceColor;
}
QColor StyleManager::textColor() const { return m_pImpl->m_textColor; }
QColor StyleManager::textSecondaryColor() const {
    return m_pImpl->m_textSecondaryColor;
}
QColor StyleManager::borderColor() const { return m_pImpl->m_borderColor; }
QColor StyleManager::mutedBorderColor() const {
    return m_pImpl->m_mutedBorderColor;
}
QColor StyleManager::hoverColor() const { return m_pImpl->m_hoverColor; }
QColor StyleManager::pressedColor() const { return m_pImpl->m_pressedColor; }
QColor StyleManager::accentColor() const { return m_pImpl->m_accentColor; }
QColor StyleManager::focusColor() const { return m_pImpl->m_focusColor; }
QColor StyleManager::overlayColor() const { return m_pImpl->m_overlayColor; }

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

QFont StyleManager::headingFont() const {
    QFont font("Segoe UI", 11);
    font.setWeight(QFont::DemiBold);
    return font;
}

QFont StyleManager::captionFont() const {
    QFont font("Segoe UI", 8);
    font.setWeight(QFont::Medium);
    return font;
}

QFont StyleManager::monospaceFont() const {
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(9);
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
        /* Accessible invalid state for inputs */
        QStatusBar QLineEdit[invalid="true"] {
            border-color: %7;
            background-color: rgba(255, 0, 0, 0.06);
        }
    )")
        .arg(surfaceColor().name())
        .arg(borderColor().name())
        .arg(textColor().name())
        .arg(textSecondaryColor().name())
        .arg(backgroundColor().name())
        .arg(accentColor().name())
        .arg(errorColor().name());
}

QString StyleManager::getPDFViewerStyleSheet() const {
    return QString(R"(
        QScrollArea {
            background-color: %1;
            border: none;
        }
        QScrollArea > QWidget > QWidget {
            background-color: %1;
        }
        QLabel#pdfPage {
            background-color: white;
            border: 1px solid %2;
            border-radius: 4px;
            margin: 8px;
        }
    )")
        .arg(QColor(240, 240, 240).name())
        .arg(borderColor().name());
}

QString StyleManager::getScrollBarStyleSheet() const {
    return createScrollBarStyle();
}

QString StyleManager::getQssStyleSheet() const {
    return m_pImpl->getQssStyleSheet();
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
        .arg(surfaceAltColor().name())
        .arg(mutedBorderColor().name())
        .arg(textSecondaryColor().name())
        .arg(secondaryColor().name());
}

QString StyleManager::createInputStyle() const {
    QColor focusShadow = focusColor();
    focusShadow.setAlphaF(0.2);

    QColor disabledBg = surfaceAltColor();
    QColor disabledText = textSecondaryColor();
    QColor disabledBorder = mutedBorderColor();

    if (m_pImpl->m_currentTheme == Theme::Dark) {
        disabledBg = disabledBg.darker(140);
        disabledBorder = disabledBorder.darker(120);
        disabledText = disabledText.darker(110);
    } else {
        disabledBg = disabledBg.lighter(105);
    }

    QString transition = getTransitionStyle(
        "background-color, border-color, box-shadow", animationFast());

    return QString(R"(
        background-color: %1;
        border: 1px solid %2;
        border-radius: %3px;
        color: %4;
        padding: 6px 10px;
        selection-background-color: %5;
        selection-color: %6;
        %7
    )")
               .arg(elevatedSurfaceColor().name())
               .arg(mutedBorderColor().name())
               .arg(radiusMD())
               .arg(textColor().name())
               .arg(accentColor().name())
               .arg(backgroundColor().name())
               .arg(transition) +
           QString(R"(
        QLineEdit:hover {
            background-color: %1;
            border-color: %2;
        }
        QLineEdit:focus {
            border: 2px solid %3;
            padding: 5px 9px;
            box-shadow: 0 0 0 3px %4;
        }
        QLineEdit:disabled {
            color: %5;
            background-color: %6;
            border-color: %7;
        }
    )")
               .arg(surfaceAltColor().name())
               .arg(accentColor().name())
               .arg(accentColor().name())
               .arg(focusShadow.name(QColor::HexArgb))
               .arg(disabledText.name())
               .arg(disabledBg.name())
               .arg(disabledBorder.name());
}

QString StyleManager::createCardStyle() const {
    return QString(R"(
        background-color: %1;
        border: 1px solid %2;
        border-radius: %3px;
        padding: %4px;
    )")
        .arg(elevatedSurfaceColor().name())
        .arg(mutedBorderColor().name())
        .arg(radiusLG())
        .arg(spacingSM());
}

QString StyleManager::createBadgeStyle() const {
    return QString(R"(
        background-color: %1;
        color: %2;
        border-radius: %3px;
        padding: 2px 8px;
        font-size: 11px;
        font-weight: 600;
    )")
        .arg(surfaceAltColor().name())
        .arg(textSecondaryColor().name())
        .arg(radiusLG());
}

QString StyleManager::createToggleButtonStyle() const {
    QColor focusShadow = focusColor();
    focusShadow.setAlphaF(0.25);

    QString transition = getTransitionStyle(
        "background-color, color, border-color", animationFast());

    return QString(R"(
        QPushButton {
            background-color: %1;
            border: 1px solid %2;
            border-radius: %3px;
            color: %4;
            text-align: left;
            padding: 8px 12px;
            font-weight: 600;
            %5
        }
        QPushButton:hover {
            background-color: %6;
            border-color: %7;
        }
        QPushButton:checked {
            background-color: %8;
            border-color: %7;
            color: %9;
        }
        QPushButton:focus {
            box-shadow: 0 0 0 3px %10;
        }
    )")
        .arg(surfaceAltColor().name())
        .arg(mutedBorderColor().name())
        .arg(radiusMD())
        .arg(textColor().name())
        .arg(transition)
        .arg(surfaceColor().name())
        .arg(accentColor().name())
        .arg(accentColor().lighter(115).name())
        .arg(backgroundColor().name())
        .arg(focusShadow.name(QColor::HexArgb));
}

QString StyleManager::createMessageLabelStyle(const QColor& background,
                                              const QColor& text) const {
    return QString(R"(
        background-color: %1;
        color: %2;
        padding: 8px 16px;
        border-radius: %3px;
        font-weight: 600;
        letter-spacing: 0.2px;
    )")
        .arg(background.name(QColor::HexArgb))
        .arg(text.name())
        .arg(radiusLG());
}

// QSS file loading implementation
QString StyleManagerImpl::loadQssFile(const QString& resourcePath) {
    QFile file(resourcePath);

    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        Logger::instance().error(
            "[managers] Failed to open QSS file: {} - Error: {}",
            resourcePath.toStdString(), file.errorString().toStdString());
        return QString();
    }

    QTextStream stream(&file);
    QString content = stream.readAll();
    file.close();

    Logger::instance().info(
        "[managers] Successfully loaded QSS file: {} ({} bytes)",
        resourcePath.toStdString(), content.size());

    return content;
}

void StyleManagerImpl::loadQssFiles() {
    Logger::instance().info(
        "[managers] Loading QSS theme files from resources...");

    // Load light theme QSS
    m_cachedLightQss = loadQssFile(":/styles/light.qss");
    if (m_cachedLightQss.isEmpty()) {
        Logger::instance().warning(
            "[managers] Light theme QSS file is empty or failed to load");
    }

    // Load dark theme QSS
    m_cachedDarkQss = loadQssFile(":/styles/dark.qss");
    if (m_cachedDarkQss.isEmpty()) {
        Logger::instance().warning(
            "[managers] Dark theme QSS file is empty or failed to load");
    }

    // Load thumbnails QSS (theme-independent)
    m_cachedThumbnailsQss = loadQssFile(":/styles/thumbnails.qss");
    if (m_cachedThumbnailsQss.isEmpty()) {
        Logger::instance().warning(
            "[managers] Thumbnails QSS file is empty or failed to load");
    }

    // Load common QSS (shared refinements)
    m_cachedCommonQss = loadQssFile(":/styles/common.qss");
    if (m_cachedCommonQss.isEmpty()) {
        Logger::instance().warning(
            "[managers] Common QSS file is empty or failed to load");
    }

    Logger::instance().info("[managers] QSS theme files loading completed");
}

QString StyleManagerImpl::getQssStyleSheet() const {
    QString qssContent;

    // Add theme-specific QSS
    if (m_currentTheme == Theme::Light) {
        qssContent = m_cachedLightQss;
    } else {
        qssContent = m_cachedDarkQss;
    }

    // Append common QSS (shared styles)
    if (!m_cachedCommonQss.isEmpty()) {
        qssContent += "\n\n/* Common Styles */\n";
        qssContent += m_cachedCommonQss;
    }

    // Append thumbnails QSS (theme-independent)
    if (!m_cachedThumbnailsQss.isEmpty()) {
        qssContent += "\n\n/* Thumbnail Styles */\n";
        qssContent += m_cachedThumbnailsQss;
    }

    return qssContent;
}

// Semantic Colors Implementation
QColor StyleManager::successColor() const {
    if (m_pImpl->m_currentTheme == Theme::Light) {
        return QColor(16, 124, 16);  // Green for success
    }
    return QColor(76, 175, 80);  // Lighter green for dark theme
}

QColor StyleManager::warningColor() const {
    if (m_pImpl->m_currentTheme == Theme::Light) {
        return QColor(255, 152, 0);  // Orange for warning
    }
    return QColor(255, 183, 77);  // Lighter orange for dark theme
}

QColor StyleManager::errorColor() const {
    if (m_pImpl->m_currentTheme == Theme::Light) {
        return QColor(211, 47, 47);  // Red for error
    }
    return QColor(244, 67, 54);  // Lighter red for dark theme
}

QColor StyleManager::infoColor() const {
    if (m_pImpl->m_currentTheme == Theme::Light) {
        return QColor(2, 136, 209);  // Blue for info
    }
    return QColor(41, 182, 246);  // Lighter blue for dark theme
}

// Animation Helpers Implementation
QString StyleManager::getTransitionStyle(const QString& property, int duration,
                                         const QString& easing) const {
    // CSS transitions are not supported in Qt's offscreen platform and cause
    // crashes Return empty string in offscreen mode to avoid "Unknown property
    // transition" warnings
    if (QGuiApplication::platformName() == "offscreen") {
        return QString();
    }
    return QString("transition: %1 %2ms %3;")
        .arg(property)
        .arg(duration)
        .arg(easing);
}
