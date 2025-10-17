#pragma once

#include <QColor>
#include <QFont>
#include <QObject>
#include <QString>
#include <memory>

enum class Theme : std::uint8_t { Light, Dark };

// Forward declaration
class StyleManagerImpl;

class StyleManager : public QObject {
    Q_OBJECT

public:
    static StyleManager& instance();

    // 主题管理
    void setTheme(Theme theme);
    [[nodiscard]] Theme currentTheme() const;

    // 样式表获取
    [[nodiscard]] QString getApplicationStyleSheet() const;
    [[nodiscard]] QString getToolbarStyleSheet() const;
    [[nodiscard]] QString getStatusBarStyleSheet() const;
    [[nodiscard]] QString getPDFViewerStyleSheet() const;
    [[nodiscard]] QString getButtonStyleSheet() const;
    [[nodiscard]] QString getScrollBarStyleSheet() const;
    [[nodiscard]] QString getQssStyleSheet() const;  // Get QSS file content for current theme

    // 颜色获取
    [[nodiscard]] QColor primaryColor() const;
    [[nodiscard]] QColor secondaryColor() const;
    [[nodiscard]] QColor backgroundColor() const;
    [[nodiscard]] QColor surfaceColor() const;
    [[nodiscard]] QColor surfaceAltColor() const;
    [[nodiscard]] QColor elevatedSurfaceColor() const;
    [[nodiscard]] QColor textColor() const;
    [[nodiscard]] QColor textSecondaryColor() const;
    [[nodiscard]] QColor borderColor() const;
    [[nodiscard]] QColor mutedBorderColor() const;
    [[nodiscard]] QColor hoverColor() const;
    [[nodiscard]] QColor pressedColor() const;
    [[nodiscard]] QColor accentColor() const;
    [[nodiscard]] QColor focusColor() const;
    [[nodiscard]] QColor overlayColor() const;

    // 字体获取
    [[nodiscard]] QFont defaultFont() const;
    [[nodiscard]] QFont titleFont() const;
    [[nodiscard]] QFont buttonFont() const;
    [[nodiscard]] QFont headingFont() const;
    [[nodiscard]] QFont captionFont() const;
    [[nodiscard]] QFont monospaceFont() const;

    // 样式创建方法
    [[nodiscard]] QString createButtonStyle() const;
    [[nodiscard]] QString createScrollBarStyle() const;
    [[nodiscard]] QString createInputStyle() const;
    [[nodiscard]] QString createCardStyle() const;
    [[nodiscard]] QString createBadgeStyle() const;
    [[nodiscard]] QString createToggleButtonStyle() const;
    [[nodiscard]] QString createMessageLabelStyle(const QColor& background,
                                    const QColor& text) const;

    // 尺寸常量 (Legacy - use spacing scale below for new code)
    [[nodiscard]] int buttonHeight() const { return 32; }
    [[nodiscard]] int buttonMinWidth() const { return 80; }
    [[nodiscard]] int iconSize() const { return 16; }
    [[nodiscard]] int spacing() const { return 8; }
    [[nodiscard]] int margin() const { return 12; }
    [[nodiscard]] int borderRadius() const { return 6; }

    // Modern Spacing Scale (8pt grid system)
    [[nodiscard]] int spacingXS() const { return 4; }   // Extra small - tight spacing
    [[nodiscard]] int spacingSM() const { return 8; }   // Small - standard spacing
    [[nodiscard]] int spacingMD() const { return 16; }  // Medium - section spacing
    [[nodiscard]] int spacingLG() const { return 24; }  // Large - group spacing
    [[nodiscard]] int spacingXL() const { return 32; }  // Extra large - major sections
    [[nodiscard]] int spacingXXL() const { return 48; } // Extra extra large - page sections

    // Border Radius Scale
    [[nodiscard]] int radiusSM() const { return 4; }   // Small radius - subtle rounding
    [[nodiscard]] int radiusMD() const { return 6; }   // Medium radius - standard
    [[nodiscard]] int radiusLG() const { return 8; }   // Large radius - prominent
    [[nodiscard]] int radiusXL() const { return 12; }  // Extra large - cards/dialogs
    [[nodiscard]] int radiusFull() const { return 9999; } // Full radius - circular

    // Animation Duration Constants (milliseconds)
    [[nodiscard]] int animationFast() const { return 150; }    // Fast - micro-interactions
    [[nodiscard]] int animationNormal() const { return 250; }  // Normal - standard transitions
    [[nodiscard]] int animationSlow() const { return 400; }    // Slow - page transitions

    // Elevation/Shadow Levels
    [[nodiscard]] QString shadowSM() const { return "0 1px 3px rgba(0, 0, 0, 0.12)"; }
    [[nodiscard]] QString shadowMD() const { return "0 2px 6px rgba(0, 0, 0, 0.15)"; }
    [[nodiscard]] QString shadowLG() const { return "0 4px 12px rgba(0, 0, 0, 0.18)"; }
    [[nodiscard]] QString shadowXL() const { return "0 8px 24px rgba(0, 0, 0, 0.20)"; }

    // Semantic Colors
    [[nodiscard]] QColor successColor() const;
    [[nodiscard]] QColor warningColor() const;
    [[nodiscard]] QColor errorColor() const;
    [[nodiscard]] QColor infoColor() const;

    // Animation Helpers
    [[nodiscard]] QString getTransitionStyle(const QString& property = "all",
                               int duration = 250,
                               const QString& easing = "ease-in-out") const;
    [[nodiscard]] QString getHoverTransform() const { return "translateY(-1px)"; }
    [[nodiscard]] QString getPressedTransform() const { return "translateY(1px)"; }

    // Deleted copy/move operations (public for better error messages)
    StyleManager(const StyleManager&) = delete;
    StyleManager& operator=(const StyleManager&) = delete;
    StyleManager(StyleManager&&) = delete;
    StyleManager& operator=(StyleManager&&) = delete;

signals:
    void themeChanged(Theme theme);

private:
    StyleManager();
    ~StyleManager() override;

    std::unique_ptr<StyleManagerImpl> pImpl;
};

// 便捷宏
#define STYLE StyleManager::instance()
