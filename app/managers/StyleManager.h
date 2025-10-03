#pragma once

#include <QColor>
#include <QFont>
#include <QObject>
#include <QString>
#include <memory>

enum class Theme { Light, Dark };

// Forward declaration
class StyleManagerImpl;

class StyleManager : public QObject {
    Q_OBJECT

public:
    static StyleManager& instance();

    // 主题管理
    void setTheme(Theme theme);
    Theme currentTheme() const;

    // 样式表获取
    QString getApplicationStyleSheet() const;
    QString getToolbarStyleSheet() const;
    QString getStatusBarStyleSheet() const;
    QString getPDFViewerStyleSheet() const;
    QString getButtonStyleSheet() const;
    QString getScrollBarStyleSheet() const;

    // 颜色获取
    QColor primaryColor() const;
    QColor secondaryColor() const;
    QColor backgroundColor() const;
    QColor surfaceColor() const;
    QColor textColor() const;
    QColor textSecondaryColor() const;
    QColor borderColor() const;
    QColor hoverColor() const;
    QColor pressedColor() const;
    QColor accentColor() const;

    // 字体获取
    QFont defaultFont() const;
    QFont titleFont() const;
    QFont buttonFont() const;

    // 样式创建方法
    QString createButtonStyle() const;
    QString createScrollBarStyle() const;

    // 尺寸常量
    int buttonHeight() const { return 32; }
    int buttonMinWidth() const { return 80; }
    int iconSize() const { return 16; }
    int spacing() const { return 8; }
    int margin() const { return 12; }
    int borderRadius() const { return 6; }

signals:
    void themeChanged(Theme theme);

private:
    StyleManager();
    ~StyleManager();
    StyleManager(const StyleManager&) = delete;
    StyleManager& operator=(const StyleManager&) = delete;

    std::unique_ptr<StyleManagerImpl> pImpl;
};

// 便捷宏
#define STYLE StyleManager::instance()
