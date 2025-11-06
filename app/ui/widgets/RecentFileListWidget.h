#pragma once

#include <QDateTime>
#include <QEnterEvent>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>

#include <QComboBox>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

class RecentFilesManager;
#include "../../managers/RecentFilesManager.h"

// Forward declarations for Ela widgets
class ElaText;
class ElaPushButton;
class ElaToolButton;
class ElaComboBox;
class ElaLineEdit;

/**
 * View mode for recent file list
 */
enum class RecentFileViewMode : std::uint8_t {
    Compact,  // Show only filename, icon, and date
    Detailed  // Show full information including path, size, page count
};

/**
 * 最近文件条目组件
 * 显示单个最近文件的信息，支持点击打开
 */
class RecentFileItemWidget : public QFrame {
    Q_OBJECT

public:
    explicit RecentFileItemWidget(
        const RecentFileInfo& fileInfo,
        RecentFileViewMode viewMode = RecentFileViewMode::Detailed,
        QWidget* parent = nullptr);
    ~RecentFileItemWidget();

    // 文件信息
    const RecentFileInfo& fileInfo() const { return m_fileInfo; }
    void updateFileInfo(const RecentFileInfo& fileInfo);

    // View mode
    void setViewMode(RecentFileViewMode mode);
    RecentFileViewMode viewMode() const { return m_viewMode; }

    // 主题支持
    void applyTheme();

signals:
    void clicked(const QString& filePath);
    void removeRequested(const QString& filePath);
    void openInNewTabRequested(const QString& filePath);
    void clearAllRecentRequested();
    void pinToggleRequested(const QString& filePath);
    void openContainingFolderRequested(const QString& filePath);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onRemoveClicked();

private:
    void setupUI();
    void setupAnimations();
    void updateDisplay();
    void setHovered(bool hovered);
    void startHoverAnimation(bool hovered);
    void startPressAnimation();
    void showContextMenu(const QPoint& globalPos);
    void updateLayoutForViewMode();

    RecentFileInfo m_fileInfo;
    RecentFileViewMode m_viewMode;

    // UI组件
    QHBoxLayout* m_mainLayout;
    QVBoxLayout* m_infoLayout;
    QLabel* m_fileIconLabel;  // File type icon
    ElaText* m_fileNameLabel;
    ElaText* m_filePathLabel;
    ElaText* m_lastOpenedLabel;
    ElaText* m_fileSizeLabel;   // For detailed view
    ElaText* m_pageCountLabel;  // For detailed view
    QLabel* m_thumbnailLabel;   // For detailed view (optional)
    ElaPushButton* m_removeButton;
    ElaPushButton* m_pinButton;  // Pin/Unpin button

    // 状态
    bool m_isHovered;
    bool m_isPressed;

    // 动画效果
    QPropertyAnimation* m_hoverAnimation;
    QPropertyAnimation* m_pressAnimation;
    QGraphicsOpacityEffect* m_opacityEffect;
    qreal m_currentOpacity;

    // Enhanced 样式常量 with modern card design
    static const int ITEM_HEIGHT_DETAILED = 80;  // Height for detailed view
    static const int ITEM_HEIGHT_COMPACT = 48;   // Height for compact view
    static const int PADDING = 16;  // Enhanced padding for modern card look
    static const int SPACING = 4;   // Improved spacing between elements
    static const int ICON_SIZE_DETAILED = 40;  // Icon size for detailed view
    static const int ICON_SIZE_COMPACT = 32;   // Icon size for compact view
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

    // View mode management
    void setViewMode(RecentFileViewMode mode);
    RecentFileViewMode viewMode() const { return m_viewMode; }

    // Sorting
    void setSortOrder(RecentFilesManager::SortOrder order);
    RecentFilesManager::SortOrder sortOrder() const { return m_sortOrder; }

    // Search/Filter
    void setSearchFilter(const QString& filter);
    QString searchFilter() const { return m_searchFilter; }

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
    void addFileItem(const RecentFileInfo& fileInfo);
    void removeFileItem(const QString& filePath);
    void updateEmptyState();
    void scheduleRefresh();

    // 管理器
    RecentFilesManager* m_recentFilesManager;

    // UI组件
    QVBoxLayout* m_mainLayout;
    QWidget* m_toolbarWidget;
    QHBoxLayout* m_toolbarLayout;
    ElaToolButton* m_viewModeButton;
    ElaComboBox* m_sortComboBox;
    ElaLineEdit* m_searchLineEdit;
    ElaToolButton* m_clearAllButton;
    QScrollArea* m_scrollArea;
    QWidget* m_contentWidget;
    QVBoxLayout* m_contentLayout;
    ElaText* m_emptyLabel;

    // 文件条目
    QList<RecentFileItemWidget*> m_fileItems;

    // 刷新定时器
    QTimer* m_refreshTimer;

    // 状态
    bool m_isInitialized;
    RecentFileViewMode m_viewMode;
    RecentFilesManager::SortOrder m_sortOrder;
    QString m_searchFilter;

    // 样式常量
    static const int MAX_VISIBLE_ITEMS = 50;  // Increased for better usability
    static const int REFRESH_DELAY = 100;     // ms
};
