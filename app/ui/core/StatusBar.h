#pragma once

#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QStatusBar>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <memory>
#include "../../factory/WidgetFactory.h"

class ExpandableInfoPanel : public QWidget {
    Q_OBJECT

public:
    ExpandableInfoPanel(const QString& title, QWidget* parent = nullptr);
    ~ExpandableInfoPanel() override;
    void setContentWidget(QWidget* widget);
    void setExpanded(bool expanded, bool animated = true);
    bool isExpanded() const { return m_expanded; }

signals:
    void expandedChanged(bool expanded);

private:
    QPushButton* m_toggleButton;
    QWidget* m_contentWidget;
    QFrame* m_contentFrame;
    QPropertyAnimation* m_animation;
    bool m_expanded;

    void updateToggleButton();
    void applyTheme();
};

/**
 * @brief Enhanced status bar with document information, progress tracking, and
 * expandable panels
 *
 * @details This status bar provides comprehensive document status information
 * including:
 * - File name, page info, and zoom level
 * - Document metadata (title, author, subject, keywords, dates)
 * - Document statistics (word count, character count, reading time)
 * - Security information (encryption, permissions)
 * - Search results display
 * - Loading progress tracking
 * - Quick action buttons
 *
 * **Minimal Mode:**
 * The status bar supports a minimal mode (enabled via constructor parameter)
 * designed for:
 * - Headless testing environments without Qt platform plugins
 * - Unit testing where UI widgets are not needed
 * - Reduced memory footprint scenarios
 *
 * When minimal mode is enabled:
 * - All widget pointers are initialized to nullptr
 * - All public methods perform null checks and return early if widgets don't
 * exist
 * - No UI elements are created or displayed
 * - The status bar acts as a no-op interface for testing purposes
 *
 * @note All public methods are safe to call in minimal mode - they will
 * gracefully handle nullptr widgets and return without error.
 *
 * @see ExpandableInfoPanel for the collapsible panel implementation
 */
class StatusBar : public QStatusBar {
    Q_OBJECT
public:
    /**
     * @brief Construct a new Status Bar object
     * @param parent Parent widget (optional)
     * @param minimalMode If true, creates a minimal status bar without UI
     * widgets for testing (default: false)
     */
    explicit StatusBar(QWidget* parent = nullptr, bool minimalMode = false);

    /**
     * @brief Construct a new Status Bar object using WidgetFactory
     * @param factory Widget factory for creating UI components
     * @param parent Parent widget (optional)
     */
    StatusBar(WidgetFactory* factory, QWidget* parent = nullptr);

    /**
     * @brief Destroy the Status Bar object and clean up resources
     */
    ~StatusBar();

    // 状态信息更新接口
    void setDocumentInfo(const QString& fileName, int currentPage,
                         int totalPages, double zoomLevel);
    void setPageInfo(int current, int total);
    void setZoomLevel(int percent);
    void setZoomLevel(double percent);
    void setFileName(const QString& fileName);
    void setMessage(const QString& message);

    // 扩展的文档元数据
    void setDocumentMetadata(const QString& title, const QString& author,
                             const QString& subject, const QString& keywords,
                             const QDateTime& created,
                             const QDateTime& modified);
    void setDocumentStatistics(int wordCount, int charCount, int pageCount);
    void setDocumentSecurity(bool encrypted, bool copyAllowed,
                             bool printAllowed);

    // 消息显示增强
    void setErrorMessage(const QString& message, int timeout = 5000);
    void setSuccessMessage(const QString& message, int timeout = 3000);
    void setWarningMessage(const QString& message, int timeout = 4000);

    // 搜索结果
    void setSearchResults(int currentMatch, int totalMatches);
    void clearSearchResults();

    // 页码输入功能
    void enablePageInput(bool enabled);
    void setPageInputRange(int min, int max);

    // 可展开面板控制
    void setCompactMode(bool compact);
    void expandAllPanels();
    void collapseAllPanels();

    // 清空状态信息
    void clearDocumentInfo();

    // 加载进度相关方法
    void showLoadingProgress(const QString& message = QString());
    void updateLoadingProgress(int progress);
    void setLoadingMessage(const QString& message);
    void hideLoadingProgress();

protected:
    void changeEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

signals:
    void pageJumpRequested(int pageNumber);
    void zoomLevelChangeRequested(double zoomLevel);
    void searchRequested(const QString& text);

private slots:
    void onPageInputReturnPressed();
    void onPageInputEditingFinished();
    void onPageInputTextChanged(const QString& text);
    void onZoomInputReturnPressed();
    void onSearchInputReturnPressed();
    void updateClock();
    void onMessageTimerTimeout();

private:
    void setupMainSection();
    void setupDocumentInfoPanel();
    void setupStatisticsPanel();
    void setupSecurityPanel();
    void setupQuickActionsPanel();
    void applyEnhancedStyle();
    void retranslateUi();
    QString formatFileSize(qint64 size) const;
    QString formatDateTime(const QDateTime& dateTime) const;
    QString formatFileName(const QString& fullPath) const;
    bool validateAndJumpToPage(const QString& input);
    void animateWidget(QWidget* widget, const QString& property,
                       const QVariant& start, const QVariant& end,
                       int duration = 200);
    void applyFieldStyles();
    void applyPanelTypography();
    void applyQuickActionStyles();
    void updateMessageAppearance(const QColor& background, const QColor& text);
    void setLineEditInvalid(QLineEdit* edit, bool invalid);
    void displayTransientMessage(const QString& text, int timeout,
                                 const QColor& background,
                                 const QColor& foreground);

    // 主要区域控件
    QFrame* m_mainSection;
    QLabel* m_fileNameLabel;
    QLabel* m_pageLabel;
    QLineEdit* m_pageInputEdit;
    QLabel* m_zoomLabel;
    QLineEdit* m_zoomInputEdit;
    QLabel* m_clockLabel;
    QTimer* m_clockTimer;

    // 消息显示
    QLabel* m_messageLabel;
    QTimer* m_messageTimer;
    QPropertyAnimation* m_messageAnimation;

    // 加载进度
    QProgressBar* m_loadingProgressBar;
    QLabel* m_loadingMessageLabel;
    QPropertyAnimation* m_progressAnimation;

    // 搜索
    QFrame* m_searchFrame;
    QLineEdit* m_searchInput;
    QLabel* m_searchResultsLabel;

    // 可展开面板
    ExpandableInfoPanel* m_documentInfoPanel;
    ExpandableInfoPanel* m_statisticsPanel;
    ExpandableInfoPanel* m_securityPanel;
    ExpandableInfoPanel* m_quickActionsPanel;

    // 文档信息控件
    QLabel* m_titleLabel;
    QLabel* m_authorLabel;
    QLabel* m_subjectLabel;
    QLabel* m_keywordsLabel;
    QLabel* m_createdLabel;
    QLabel* m_modifiedLabel;
    QLabel* m_fileSizeLabel;

    // 统计信息控件
    QLabel* m_wordCountLabel;
    QLabel* m_charCountLabel;
    QLabel* m_pageCountLabel;
    QLabel* m_avgWordsPerPageLabel;
    QLabel* m_readingTimeLabel;

    // 安全信息控件
    QLabel* m_encryptionLabel;
    QLabel* m_copyPermissionLabel;
    QLabel* m_printPermissionLabel;
    QLabel* m_modifyPermissionLabel;

    // 快速操作按钮
    QPushButton* m_bookmarkBtn;
    QPushButton* m_annotateBtn;
    QPushButton* m_shareBtn;
    QPushButton* m_exportBtn;

    // 状态变量
    int m_currentTotalPages;
    int m_currentPageNumber;
    QString m_currentFileName;
    bool m_compactMode;
    bool m_lastEncrypted = false;
    bool m_lastCopyAllowed = true;
    bool m_lastPrintAllowed = true;

    // 保留兼容性的别名
    QLabel*& fileNameLabel = m_fileNameLabel;
    QLabel*& pageLabel = m_pageLabel;
    QLineEdit*& pageInputEdit = m_pageInputEdit;
    QLabel*& zoomLabel = m_zoomLabel;
    QLabel* separatorLabel1;
    QLabel* separatorLabel2;
    QLabel* separatorLabel3;
    QProgressBar*& loadingProgressBar = m_loadingProgressBar;
    QLabel*& loadingMessageLabel = m_loadingMessageLabel;
    QPropertyAnimation*& progressAnimation = m_progressAnimation;
    int& currentTotalPages = m_currentTotalPages;
    int& currentPageNumber = m_currentPageNumber;
    QString& currentFileName = m_currentFileName;
};
