#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QColor>
#include <QDateTime>
#include <QMap>
#include <QStatusBar>
#include <QString>

// Forward declarations
class ElaText;
class QWidget;
class QHBoxLayout;
class ElaLineEdit;
class ElaProgressBar;
class QTimer;
class QPropertyAnimation;
class ElaToolButton;

/**
 * @brief ElaStatusBarWidget - 状态栏组件
 *
 * 功能：
 * - 显示文档基本信息（文件名、页码、缩放级别）
 * - 显示消息和进度
 * - 可展开面板显示详细信息
 *   - 文档信息面板（标题、作者、主题、关键词、创建日期等）
 *   - 统计信息面板（页数、文件大小、PDF 版本等）
 *   - 安全信息面板（加密、权限等）
 */
class StatusBar : public QStatusBar {
    Q_OBJECT

public:
    explicit StatusBar(QWidget* parent = nullptr);
    // Backward-compatibility: minimalMode constructor
    StatusBar(QWidget* parent, bool minimalMode);
    ~StatusBar() override;

    // ========================================================================
    // 基本信息显示
    // ========================================================================

    /**
     * @brief 设置文件名
     */
    void setFileName(const QString& fileName);

    /**
     * @brief 设置页面信息
     */
    void setPageInfo(int currentPage, int totalPages);

    /**
     * @brief 设置缩放级别
     */
    void setZoomLevel(double zoomFactor);

    /**
     * @brief 设置视图模式
     */
    void setViewMode(const QString& mode);

    // ========================================================================
    // 消息和进度
    // ========================================================================

    // 消息优先级
    enum class MessagePriority { Low = 0, Normal = 1, High = 2, Critical = 3 };

    /**
     * @brief 显示临时消息（带优先级）
     */
    void showMessage(const QString& message,
                     MessagePriority priority = MessagePriority::Normal,
                     int timeout = 3000);

    /**
     * @brief 显示错误消息
     */
    void setErrorMessage(const QString& message, int timeout = 5000);

    /**
     * @brief 显示成功消息
     */
    void setSuccessMessage(const QString& message, int timeout = 3000);

    /**
     * @brief 显示警告消息
     */
    void setWarningMessage(const QString& message, int timeout = 4000);

    /**
     * @brief 清除消息
     */
    void clearMessages(MessagePriority maxPriority = MessagePriority::Normal);

    /**
     * @brief 显示进度（带优先级）
     * @param message 进度消息
     * @param priority 优先级（默认5）
     */
    void showProgress(const QString& message, int priority = 5);

    /** Backward-compatibility helpers expected by some tests */
    void setMessage(const QString& message);           // -> showMessage
    void showLoadingProgress(const QString& message);  // -> showProgress
    void updateLoadingProgress(int progress);          // -> updateProgress
    void hideLoadingProgress();                        // -> hideProgress

    /**
     * @brief 更新进度
     * @param progress 进度值（0-100）
     * @param message 可选的进度消息
     */
    void updateProgress(int progress, const QString& message = QString());

    /**
     * @brief 隐藏进度
     */
    void hideProgress();

    // ========================================================================
    // 文档信息
    // ========================================================================

    /**
     * @brief 设置文档元数据
     */
    void setDocumentMetadata(const QMap<QString, QString>& metadata);

    /**
     * @brief 设置文档统计信息
     */
    void setDocumentStatistics(const QMap<QString, QString>& statistics);

    /**
     * @brief 设置文档安全信息
     */

    // -----------------------------------------------------------------------
    // Backward-compatibility API expected by legacy tests
    // -----------------------------------------------------------------------
    void setDocumentInfo(const QString& fileName, int currentPage,
                         int totalPages, double zoomLevel);
    void setDocumentMetadata(const QString& title, const QString& author,
                             const QString& subject, const QString& keywords,
                             const QDateTime& created,
                             const QDateTime& modified);
    void setDocumentStatistics(int wordCount, int charCount, int pageCount);
    void setDocumentSecurity(bool encrypted, bool copyAllowed,
                             bool printAllowed);
    void setSearchResults(int currentMatch, int totalMatches);
    void clearSearchResults();
    void enablePageInput(bool enabled);
    void setPageInputRange(int minPage, int maxPage);
    void setCompactMode(bool compact);
    void expandAllPanels();
    void collapseAllPanels();
    void setLoadingMessage(const QString& message);
    void clearDocumentInfo();

    void setDocumentSecurity(const QMap<QString, QString>& security);

    // ========================================================================
    // 面板控制
    // ========================================================================

    /**
     * @brief 显示文档信息面板
     */
    void showDocumentInfoPanel();

    /**
     * @brief 显示统计信息面板
     */
    void showStatisticsPanel();

    /**
     * @brief 显示安全信息面板
     */
    void showSecurityPanel();

    /**
     * @brief 隐藏所有面板
     */
    void hideAllPanels();

    // ========================================================================
    // 状态管理
    // ========================================================================

    /**
     * @brief 清除所有信息
     */
    void clearAll();

    /**
     * @brief 启用/禁用状态栏
     */
    void setEnabled(bool enabled);

signals:
    /**
     * @brief 面板显示状态改变
     */
    void panelVisibilityChanged(const QString& panelName, bool visible);

    /**
     * @brief 页面跳转请求
     */
    void pageJumpRequested(int pageNumber);

    /**
     * @brief 缩放级别改变请求
     */
    void zoomLevelChangeRequested(double zoomLevel);

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void onPageInputReturnPressed();
    void onZoomInputReturnPressed();
    void onMessageTimerTimeout();

private:
    // UI 组件 - 主要信息
    ElaText* m_fileNameLabel;
    ElaLineEdit* m_pageInputEdit;  // 交互式页面输入
    ElaText* m_pageInfoLabel;      // 页面信息显示（总页数）
    ElaLineEdit* m_zoomInputEdit;  // 交互式缩放输入
    ElaText* m_viewModeLabel;

    // 消息系统
    ElaText* m_messageLabel;
    QTimer* m_messageTimer;
    QPropertyAnimation* m_messageAnimation;
    MessagePriority m_currentMessagePriority;
    QTimer* m_messagePriorityTimer;

    // 进度系统
    // 搜索结果显示（兼容旧测试）
    ElaText* m_searchResultsLabel{nullptr};

    // 兼容页码范围控制
    int m_pageMinRange{1};
    int m_pageMaxRange{0};

    // 模式控制
    bool m_minimalMode{false};
    bool m_compactMode{false};

    ElaProgressBar* m_loadingProgressBar;
    ElaText* m_loadingMessageLabel;
    QPropertyAnimation* m_progressAnimation;
    bool m_progressVisible;
    int m_currentProgressPriority;

    // 面板按钮
    ElaToolButton* m_docInfoBtn;
    ElaToolButton* m_statisticsBtn;
    ElaToolButton* m_securityBtn;

    // 面板容器
    QWidget* m_docInfoPanel;
    QWidget* m_statisticsPanel;
    QWidget* m_securityPanel;

    // 数据
    QString m_fileName;
    int m_currentPage;
    int m_totalPages;
    double m_zoomFactor;
    QString m_viewMode;
    QMap<QString, QString> m_metadata;
    QMap<QString, QString> m_statistics;
    QMap<QString, QString> m_security;

    // 当前显示的面板
    QWidget* m_currentPanel;

    // 初始化方法
    void setupUi();
    void setupMainInfo();
    void setupPanelButtons();
    void setupPanels();
    void connectSignals();

    // 面板创建
    QWidget* createDocumentInfoPanel();
    QWidget* createStatisticsPanel();
    QWidget* createSecurityPanel();

    // 面板更新
    void updateDocumentInfoPanel();
    void updateStatisticsPanel();
    void updateSecurityPanel();

    // 辅助方法
    void showPanel(QWidget* panel);
    void hidePanel(QWidget* panel);
    void retranslateUi();
    void updateLabels();

    // 消息系统辅助方法
    void displayTransientMessage(const QString& text, int timeout,
                                 const QColor& background,
                                 const QColor& foreground);
    void updateMessageAppearance(const QColor& background, const QColor& text);

    // 输入验证
    bool validateAndJumpToPage(const QString& input);
    void setLineEditInvalid(ElaLineEdit* edit, bool invalid);
};

#endif  // STATUSBAR_H
