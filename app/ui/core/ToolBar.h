#pragma once

#include <QAction>
#include <QButtonGroup>
#include <QComboBox>
#include <QDateTime>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QSlider>
#include <QSpinBox>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <memory>
#include "../../controller/tool.hpp"

class CollapsibleSection : public QWidget {
    Q_OBJECT

public:
    CollapsibleSection(const QString& title, QWidget* parent = nullptr);
    void setContentWidget(QWidget* widget);
    void setExpanded(bool expanded);
    bool isExpanded() const { return m_expanded; }

signals:
    void expandedChanged(bool expanded);

private:
    QToolButton* m_toggleButton;
    QWidget* m_contentWidget;
    QFrame* m_contentFrame;
    QPropertyAnimation* m_animation;
    bool m_expanded;

    void toggleExpanded();
};

class ToolBar : public QToolBar {
    Q_OBJECT

public:
    ToolBar(QWidget* parent = nullptr);

    // 状态更新接口
    void updatePageInfo(int currentPage, int totalPages);
    void updateZoomLevel(double zoomFactor);
    void updateDocumentInfo(const QString& fileName, qint64 fileSize,
                            const QDateTime& lastModified);
    void setActionsEnabled(bool enabled);
    void setCompactMode(bool compact);

protected:
    void changeEvent(QEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

signals:
    void actionTriggered(ActionMap action);
    void pageJumpRequested(int pageNumber);
    void zoomLevelChanged(int percentage);
    void sectionExpandChanged(const QString& sectionName, bool expanded);

private slots:
    void onPageSpinBoxChanged(int pageNumber);
    void onViewModeChanged();
    void onZoomSliderChanged(int value);
    void onSectionExpandChanged(bool expanded);

private:
    void setupFileSection();
    void setupNavigationSection();
    void setupZoomSection();
    void setupViewSection();
    void setupToolsSection();
    void setupQuickAccessBar();
    void applyEnhancedStyle();
    void retranslateUi();
    void createCollapsibleGroup(const QString& title, QWidget* content);

    // 文件操作组
    CollapsibleSection* m_fileSection;
    QAction* m_openAction;
    QAction* m_openFolderAction;
    QAction* m_saveAction;
    QAction* m_saveAsAction;
    QAction* m_printAction;
    QAction* m_emailAction;

    // 导航操作组
    CollapsibleSection* m_navigationSection;
    QAction* m_firstPageAction;
    QAction* m_prevPageAction;
    QSpinBox* m_pageSpinBox;
    QLabel* m_pageCountLabel;
    QAction* m_nextPageAction;
    QAction* m_lastPageAction;
    QSlider* m_pageSlider;
    QLabel* m_thumbnailPreview;

    // 缩放操作组
    CollapsibleSection* m_zoomSection;
    QAction* m_zoomInAction;
    QAction* m_zoomOutAction;
    QSlider* m_zoomSlider;
    QLabel* m_zoomValueLabel;
    QComboBox* m_zoomPresets;
    QAction* m_fitWidthAction;
    QAction* m_fitPageAction;
    QAction* m_fitHeightAction;

    // 视图操作组
    CollapsibleSection* m_viewSection;
    QAction* m_toggleSidebarAction;
    QAction* m_toggleFullscreenAction;
    QComboBox* m_viewModeCombo;
    QComboBox* m_layoutCombo;
    QAction* m_nightModeAction;
    QAction* m_readingModeAction;

    // 工具操作组
    CollapsibleSection* m_toolsSection;
    QAction* m_searchAction;
    QAction* m_annotateAction;
    QAction* m_highlightAction;
    QAction* m_bookmarkAction;
    QAction* m_snapshotAction;
    QAction* m_rotateLeftAction;
    QAction* m_rotateRightAction;

    // 快速访问栏
    QFrame* m_quickAccessBar;
    QAction* m_themeToggleAction;
    QAction* m_settingsAction;
    QAction* m_helpAction;

    // 文档信息显示
    QLabel* m_documentInfoLabel;
    QLabel* m_fileSizeLabel;
    QLabel* m_lastModifiedLabel;

    // 动画和效果
    QPropertyAnimation* m_hoverAnimation;
    QPropertyAnimation* m_expandAnimation;

    bool m_compactMode;
    bool m_isHovered;

    // 保留兼容性的别名
    QAction*& openAction = m_openAction;
    QAction*& openFolderAction = m_openFolderAction;
    QAction*& saveAction = m_saveAction;
    QAction*& firstPageAction = m_firstPageAction;
    QAction*& prevPageAction = m_prevPageAction;
    QSpinBox*& pageSpinBox = m_pageSpinBox;
    QLabel*& pageCountLabel = m_pageCountLabel;
    QAction*& nextPageAction = m_nextPageAction;
    QAction*& lastPageAction = m_lastPageAction;
    QAction*& zoomInAction = m_zoomInAction;
    QAction*& zoomOutAction = m_zoomOutAction;
    QAction*& fitWidthAction = m_fitWidthAction;
    QAction*& fitPageAction = m_fitPageAction;
    QAction*& fitHeightAction = m_fitHeightAction;
    QAction*& toggleSidebarAction = m_toggleSidebarAction;
    QComboBox*& viewModeCombo = m_viewModeCombo;
    QAction*& rotateLeftAction = m_rotateLeftAction;
    QAction*& rotateRightAction = m_rotateRightAction;
    QAction*& themeToggleAction = m_themeToggleAction;
};
