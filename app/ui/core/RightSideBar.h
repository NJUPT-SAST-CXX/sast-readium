#pragma once

#include <QLabel>
#include <QPropertyAnimation>
#include <QSettings>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>

// Forward declaration
class DebugLogPanel;

class RightSideBar : public QWidget {
    Q_OBJECT
public:
    RightSideBar(QWidget* parent = nullptr);

    // 显示/隐藏控制
    bool isVisible() const;
    using QWidget::setVisible;  // Bring base class method into scope
    void setVisible(bool visible, bool animated = true);
    void toggleVisibility(bool animated = true);

    // 宽度管理
    int getPreferredWidth() const;
    void setPreferredWidth(int width);
    int getMinimumWidth() const { return minimumWidth; }
    int getMaximumWidth() const { return maximumWidth; }

    // 状态持久化
    void saveState();
    void restoreState();

public slots:
    void show(bool animated = true);
    void hide(bool animated = true);

signals:
    void visibilityChanged(bool visible);
    void widthChanged(int width);

private slots:
    void onAnimationFinished();

private:
    QTabWidget* tabWidget;
    QPropertyAnimation* animation;
    QSettings* settings;
    DebugLogPanel* debugLogPanel;

    bool isCurrentlyVisible;
    int preferredWidth;
    int lastWidth;

    static const int minimumWidth = 200;
    static const int maximumWidth = 400;
    static const int defaultWidth = 250;
    static const int animationDuration = 300;

    void initWindow();
    void initContent();
    void initAnimation();
    void initSettings();
    void applyTheme();

    QWidget* createPropertiesTab();
    QWidget* createToolsTab();
    QWidget* createDebugTab();
};
