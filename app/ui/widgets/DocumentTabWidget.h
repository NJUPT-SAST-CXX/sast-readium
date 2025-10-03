#pragma once

#include <QApplication>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QPushButton>
#include <QStyle>
#include <QTabBar>
#include <QTabWidget>
#include "../../model/DocumentModel.h"

class DocumentTabBar : public QTabBar {
    Q_OBJECT

public:
    DocumentTabBar(QWidget* parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    QPoint dragStartPosition;
    bool dragInProgress;

signals:
    void tabMoveRequested(int from, int to);
};

class DocumentTabWidget : public QTabWidget {
    Q_OBJECT

public:
    DocumentTabWidget(QWidget* parent = nullptr);

    // 标签页管理
    int addDocumentTab(const QString& fileName, const QString& filePath);
    void removeDocumentTab(int index);
    void updateTabText(int index, const QString& fileName);
    void setCurrentTab(int index);
    void setTabLoadingState(int index, bool loading);

    // 拖拽支持
    void moveTab(int from, int to);

    // 获取标签页信息
    QString getTabFilePath(int index) const;
    int getTabCount() const;

protected:
    QWidget* createTabWidget(const QString& fileName, const QString& filePath);
    void setupTabBar();

private slots:
    void onTabCloseRequested(int index);
    void onTabMoveRequested(int from, int to);

private:
    DocumentTabBar* customTabBar;
    QHash<int, QString> tabFilePaths;  // 存储每个标签页对应的文件路径

signals:
    void tabCloseRequested(int index);
    void tabSwitched(int index);
    void tabMoved(int from, int to);
    void allTabsClosed();
};
