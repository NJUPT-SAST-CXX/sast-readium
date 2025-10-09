#pragma once

#include <QAction>
#include <QButtonGroup>
#include <QComboBox>
#include <QLabel>
#include <QSpinBox>
#include <QToolBar>
#include <QToolButton>
#include "../../controller/tool.hpp"

class ToolBar : public QToolBar {
    Q_OBJECT

public:
    ToolBar(QWidget* parent = nullptr);

    // 状态更新接口
    void updatePageInfo(int currentPage, int totalPages);
    void updateZoomLevel(double zoomFactor);
    void setActionsEnabled(bool enabled);

signals:
    void actionTriggered(ActionMap action);
    void pageJumpRequested(int pageNumber);

private slots:
    void onPageSpinBoxChanged(int pageNumber);
    void onViewModeChanged();

private:
    void setupFileActions();
    void setupNavigationActions();
    void setupZoomActions();
    void setupViewActions();
    void setupRotationActions();
    void setupThemeActions();
    void createSeparator();
    void applyToolBarStyle();

    // 文件操作
    QAction* openAction;
    QAction* openFolderAction;
    QAction* saveAction;

    // 导航操作
    QAction* firstPageAction;
    QAction* prevPageAction;
    QSpinBox* pageSpinBox;
    QLabel* pageCountLabel;
    QAction* nextPageAction;
    QAction* lastPageAction;

    // 缩放操作
    QAction* zoomInAction;
    QAction* zoomOutAction;
    QAction* fitWidthAction;
    QAction* fitPageAction;
    QAction* fitHeightAction;

    // 视图操作
    QAction* toggleSidebarAction;
    QComboBox* viewModeCombo;

    // 旋转操作
    QAction* rotateLeftAction;
    QAction* rotateRightAction;

    // 主题操作
    QAction* themeToggleAction;
};
