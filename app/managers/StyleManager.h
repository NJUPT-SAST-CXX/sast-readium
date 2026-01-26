#pragma once

#include <QApplication>
#include <QColor>
#include <QFont>
#include <QObject>
#include <QString>
#include <QWidget>
#include <QList>

class QMainWindow;

enum class Theme { Light, Dark };

class StyleManager : public QObject {
    Q_OBJECT

public:
    static StyleManager& instance();

    // 主题管理
    void setTheme(Theme theme);
    void toggleTheme();  // 在亮色/暗色间切换
    void setLightTheme(); // 直接设置亮色主题
    void setDarkTheme();  // 直接设置暗色主题
    Theme currentTheme() const { return m_currentTheme; }

    // 统一主题应用
    void applyThemeStyleSheet(const QString& styleSheet);
    void forceApplyTheme(QWidget* widget, const QString& styleSheet);

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

    // 尺寸常量
    int buttonHeight() const { return 32; }
    int buttonMinWidth() const { return 80; }
    int iconSize() const { return 16; }
    int spacing() const { return 8; }
    int margin() const { return 12; }
    int borderRadius() const { return 6; }

signals:
    void themeChanged(Theme theme);
    void styleSheetApplied(); // 新增：样式表应用完成信号

private:
    StyleManager();
    ~StyleManager() = default;
    StyleManager(const StyleManager&) = delete;
    StyleManager& operator=(const StyleManager&) = delete;

    void updateColors();
    QString createButtonStyle() const;
    QString createScrollBarStyle() const;

    Theme m_currentTheme;

    // 颜色定义
    QColor m_primaryColor;
    QColor m_secondaryColor;
    QColor m_backgroundColor;
    QColor m_surfaceColor;
    QColor m_textColor;
    QColor m_textSecondaryColor;
    QColor m_borderColor;
    QColor m_hoverColor;
    QColor m_pressedColor;
    QColor m_accentColor;
};

// 便捷宏
#define STYLE StyleManager::instance()
