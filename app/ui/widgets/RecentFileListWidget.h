#pragma once

#include <QDateTime>
#include <QEnterEvent>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

class RecentFilesManager;
#include "../../managers/RecentFilesManager.h"

/**
 * 最近文件条目组件
 * 显示单个最近文件的信息，支持点击打开
 */
class RecentFileItemWidget : public QFrame {
    Q_OBJECT

public:
    explicit RecentFileItemWidget(const RecentFileInfo& fileInfo,
                                  QWidget* parent = nullptr);
    ~RecentFileItemWidget();

    // 文件信息
    const RecentFileInfo& fileInfo() const { return m_fileInfo; }
    void updateFileInfo(const RecentFileInfo& fileInfo);

    // 主题支持
    void applyTheme();

signals:
    void clicked(const QString& filePath);
    void removeRequested(const QString& filePath);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onRemoveClicked();

private:
    void setupUI();
    void setupAnimations();
    void updateDisplay();
    void setHovered(bool hovered);
    void startHoverAnimation(bool hovered);
    void startPressAnimation();

    RecentFileInfo m_fileInfo;

    // UI组件
    QHBoxLayout* m_mainLayout;
    QVBoxLayout* m_infoLayout;
    QLabel* m_fileIconLabel;  // File type icon
    QLabel* m_fileNameLabel;
    QLabel* m_filePathLabel;
    QLabel* m_lastOpenedLabel;
    QPushButton* m_removeButton;

    // 状态
    bool m_isHovered;
    bool m_isPressed;

    // 动画效果
    QPropertyAnimation* m_hoverAnimation;
    QPropertyAnimation* m_pressAnimation;
    QGraphicsOpacityEffect* m_opacityEffect;
    qreal m_currentOpacity;

    // Enhanced 样式常量 with modern card design
    static const int ITEM_HEIGHT =
        64;  // Increased height for icon and better spacing
    static const int PADDING = 16;  // Enhanced padding for modern card look
    static const int SPACING = 4;   // Improved spacing between elements
};

/**
 * 最近文件列表组件
 * 显示最近打开文件的列表，支持点击打开和移除
 */
class RecentFileListWidget : public QWidget {
    Q_OBJECT

public:
    explicit RecentFileListWidget(QWidget* parent = nullptr);
    ~RecentFileListWidget();

    // 设置管理器
    void setRecentFilesManager(RecentFilesManager* manager);

    // 列表管理
    void refreshList();
    void clearList();

    // 主题支持
    void applyTheme();

    // 状态查询
    bool isEmpty() const;
    int itemCount() const;

public slots:
    void onRecentFilesChanged();

signals:
    void fileClicked(const QString& filePath);
    void fileRemoveRequested(const QString& filePath);

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onItemClicked(const QString& filePath);
    void onItemRemoveRequested(const QString& filePath);
    void onRefreshTimer();

private:
    void setupUI();
    void setupConnections();
    void addFileItem(const RecentFileInfo& fileInfo);
    void removeFileItem(const QString& filePath);
    void updateEmptyState();
    void scheduleRefresh();

    // 管理器
    RecentFilesManager* m_recentFilesManager;

    // UI组件
    QVBoxLayout* m_mainLayout;
    QScrollArea* m_scrollArea;
    QWidget* m_contentWidget;
    QVBoxLayout* m_contentLayout;
    QLabel* m_emptyLabel;

    // 文件条目
    QList<RecentFileItemWidget*> m_fileItems;

    // 刷新定时器
    QTimer* m_refreshTimer;

    // 状态
    bool m_isInitialized;

    // 样式常量
    static const int MAX_VISIBLE_ITEMS = 10;
    static const int REFRESH_DELAY = 100;  // ms
};
