#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <QDateTime>
#include <QObject>
#include <QString>
#include "ElaToolBar.h"  // From ElaWidgetTools library
#include "controller/tool.hpp"

// Forward declarations
class ElaToolButton;
class ElaLineEdit;
class ElaComboBox;
class ElaSlider;
class QSpinBox;
class ElaText;
class QWidget;

class QDateTime;

/**
 * @brief ElaToolBarWidget - 完整的工具栏实现
 *
 * 实现所有工具栏功能：
 * - 文件操作：打开、保存、打印
 * - 导航：首页、上一页、下一页、末页、页面跳转
 * - 缩放：放大、缩小、缩放滑块、预设缩放
 * - 视图：视图模式、旋转、全屏
 * - 工具：搜索、书签、注释
 */
class ToolBar : public ::ElaToolBar {
    Q_OBJECT

public:
    explicit ToolBar(const QString& title, QWidget* parent = nullptr);
    ~ToolBar() override;

    // 状态更新
    void updatePageInfo(int currentPage, int totalPages);
    void updateZoomLevel(double zoomFactor);
    void updateDocumentInfo(const QString& fileName, qint64 fileSize,
                            const QDateTime& lastModified);
    void setActionsEnabled(bool enabled);
    void setNavigationEnabled(bool canGoBack, bool canGoForward);
    void setCompactMode(bool compact);

signals:
    // 动作触发信号
    void actionTriggered(ActionMap action);

    // 导航信号
    void pageJumpRequested(int pageNumber);
    void goToFirstPageRequested();
    void goToPreviousPageRequested();
    void goToNextPageRequested();
    void goToLastPageRequested();
    void goBackRequested();
    void goForwardRequested();

    // 缩放信号
    void zoomLevelChanged(double zoomFactor);
    void zoomInRequested();
    void zoomOutRequested();
    void fitWidthRequested();
    void fitPageRequested();
    void fitHeightRequested();

    // 视图信号
    void viewModeChanged(int mode);
    void rotateLeftRequested();
    void rotateRightRequested();
    void fullScreenToggled();

    // 工具信号
    void searchRequested();
    void bookmarkToggled();
    void annotationModeToggled();
    void highlightRequested();
    void snapshotRequested();

    // 视图控制信号
    void toggleSidebarRequested();
    void nightModeToggled(bool enabled);
    void readingModeToggled(bool enabled);
    void layoutModeChanged(int mode);

protected:
    void changeEvent(QEvent* event) override;

private:
    // 工具栏区域设置
    void setupFileSection();
    void setupNavigationSection();
    void setupZoomSection();
    void setupViewSection();
    void setupToolsSection();
    void setupQuickAccessBar();
    void setupDocumentInfo();

    // UI 更新
    void retranslateUi();
    void updateButtonStates();
    void updateDocumentInfoDisplay();
    void syncActionToolTips();
    void setButtonTooltip(ElaToolButton* button, const QString& text,
                          const QString& shortcut = QString());

    // 辅助方法
    ElaToolButton* createToolButton(const QString& iconName,
                                    const QString& tooltip,
                                    const QString& shortcut = QString());
    void addSeparator();

    // ========================================================================
    // 文件操作区域
    // ========================================================================
    ElaToolButton* m_openBtn;
    ElaToolButton* m_openFolderBtn;
    ElaToolButton* m_saveBtn;
    ElaToolButton* m_saveAsBtn;
    ElaToolButton* m_printBtn;
    ElaToolButton* m_emailBtn;

    // ========================================================================
    // 导航区域
    // ========================================================================
    ElaToolButton* m_firstPageBtn;
    ElaToolButton* m_prevPageBtn;
    ElaToolButton* m_backBtn;
    ElaToolButton* m_forwardBtn;
    QSpinBox* m_pageSpinBox;
    ElaText* m_pageCountLabel;
    ElaToolButton* m_nextPageBtn;
    ElaToolButton* m_lastPageBtn;
    ElaSlider* m_pageSlider;      // Page slider for quick navigation
    QWidget* m_thumbnailPreview;  // Thumbnail preview widget

    // ========================================================================
    // 缩放区域
    // ========================================================================
    ElaToolButton* m_zoomOutBtn;
    ElaSlider* m_zoomSlider;
    ElaLineEdit* m_zoomInput;
    ElaText* m_zoomLabel;
    ElaToolButton* m_zoomInBtn;
    ElaComboBox* m_zoomPresets;
    ElaToolButton* m_fitWidthBtn;
    ElaToolButton* m_fitPageBtn;
    ElaToolButton* m_fitHeightBtn;

    // ========================================================================
    // 视图区域
    // ========================================================================
    ElaComboBox* m_viewModeCombo;
    ElaComboBox* m_layoutCombo;  // Layout mode (Vertical/Horizontal)
    ElaToolButton* m_rotateLeftBtn;
    ElaToolButton* m_rotateRightBtn;
    ElaToolButton* m_fullscreenBtn;
    ElaToolButton* m_toggleSidebarBtn;
    ElaToolButton* m_nightModeBtn;
    ElaToolButton* m_readingModeBtn;

    // ========================================================================
    // 工具区域
    // ========================================================================
    ElaToolButton* m_searchBtn;
    ElaToolButton* m_bookmarkBtn;
    ElaToolButton* m_annotationBtn;
    ElaToolButton* m_highlightBtn;
    ElaToolButton* m_snapshotBtn;

    // ========================================================================
    // 快速访问栏
    // ========================================================================
    ElaToolButton* m_themeToggleBtn;
    ElaToolButton* m_settingsBtn;
    ElaToolButton* m_helpBtn;

    // ========================================================================
    // 文档信息显示
    // ========================================================================
    ElaText* m_documentInfoLabel;  // File name display
    ElaText* m_fileSizeLabel;      // File size display
    ElaText* m_lastModifiedLabel;  // Last modified display

    // ========================================================================
    // 状态
    // ========================================================================
    int m_currentPage;
    int m_totalPages;
    double m_currentZoom;
    bool m_actionsEnabled;
    bool m_isUpdatingZoom;  // 防止循环更新
    bool m_isUpdatingPage;  // Prevent recursive page updates
    bool m_compactMode;     // Compact mode flag
    QString m_currentFileName;
    qint64 m_currentFileSize;
    QDateTime m_currentLastModified;
};

#endif  // ELATOOLBAR_H
