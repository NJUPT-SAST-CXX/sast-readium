#include "DocumentTabWidget.h"
#include <QApplication>
#include <QDebug>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>

// DocumentTabBar Implementation
DocumentTabBar::DocumentTabBar(QWidget* parent)
    : QTabBar(parent), dragInProgress(false) {
    setAcceptDrops(true);
    setMovable(true);
}

void DocumentTabBar::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        dragStartPosition = event->pos();
    }
    QTabBar::mousePressEvent(event);
}

void DocumentTabBar::mouseMoveEvent(QMouseEvent* event) {
    if (!(event->buttons() & Qt::LeftButton)) {
        QTabBar::mouseMoveEvent(event);
        return;
    }

    if ((event->pos() - dragStartPosition).manhattanLength() <
        QApplication::startDragDistance()) {
        QTabBar::mouseMoveEvent(event);
        return;
    }

    int tabIndex = tabAt(dragStartPosition);
    if (tabIndex == -1) {
        QTabBar::mouseMoveEvent(event);
        return;
    }

    QDrag* drag = new QDrag(this);
    QMimeData* mimeData = new QMimeData;
    mimeData->setData("application/x-tab-index", QByteArray::number(tabIndex));
    drag->setMimeData(mimeData);

    dragInProgress = true;
    Qt::DropAction dropAction = drag->exec(Qt::MoveAction);
    dragInProgress = false;

    QTabBar::mouseMoveEvent(event);
}

void DocumentTabBar::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasFormat("application/x-tab-index")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void DocumentTabBar::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData()->hasFormat("application/x-tab-index")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void DocumentTabBar::dropEvent(QDropEvent* event) {
    if (!event->mimeData()->hasFormat("application/x-tab-index")) {
        event->ignore();
        return;
    }

    int fromIndex = event->mimeData()->data("application/x-tab-index").toInt();
    int toIndex = tabAt(event->position().toPoint());

    if (toIndex == -1) {
        toIndex = count() - 1;
    }

    if (fromIndex != toIndex && fromIndex >= 0 && toIndex >= 0) {
        emit tabMoveRequested(fromIndex, toIndex);
    }

    event->acceptProposedAction();
}

// DocumentTabWidget Implementation
DocumentTabWidget::DocumentTabWidget(QWidget* parent) : QTabWidget(parent) {
    setupTabBar();
    setTabsClosable(true);
    setMovable(true);

    connect(this, &QTabWidget::tabCloseRequested, this,
            &DocumentTabWidget::onTabCloseRequested);
    connect(this, &QTabWidget::currentChanged, this,
            &DocumentTabWidget::tabSwitched);
}

void DocumentTabWidget::setupTabBar() {
    customTabBar = new DocumentTabBar(this);
    setTabBar(customTabBar);

    connect(customTabBar, &DocumentTabBar::tabMoveRequested, this,
            &DocumentTabWidget::onTabMoveRequested);
}

int DocumentTabWidget::addDocumentTab(const QString& fileName,
                                      const QString& filePath) {
    QWidget* tabContent = createTabWidget(fileName, filePath);
    int index = addTab(tabContent, fileName);

    tabFilePaths[index] = filePath;

    // 设置工具提示显示完整路径
    setTabToolTip(index, filePath);

    return index;
}

void DocumentTabWidget::removeDocumentTab(int index) {
    if (index >= 0 && index < count()) {
        tabFilePaths.remove(index);

        // 更新其他标签页的索引映射
        QHash<int, QString> newTabFilePaths;
        for (auto it = tabFilePaths.begin(); it != tabFilePaths.end(); ++it) {
            int oldIndex = it.key();
            if (oldIndex > index) {
                newTabFilePaths[oldIndex - 1] = it.value();
            } else if (oldIndex < index) {
                newTabFilePaths[oldIndex] = it.value();
            }
        }
        tabFilePaths = newTabFilePaths;

        removeTab(index);

        if (count() == 0) {
            emit allTabsClosed();
        }
    }
}

void DocumentTabWidget::updateTabText(int index, const QString& fileName) {
    if (index >= 0 && index < count()) {
        setTabText(index, fileName);
    }
}

void DocumentTabWidget::setCurrentTab(int index) {
    if (index >= 0 && index < count()) {
        setCurrentIndex(index);
    }
}

void DocumentTabWidget::setTabLoadingState(int index, bool loading) {
    if (index >= 0 && index < count()) {
        QString currentText = tabText(index);

        if (loading) {
            // 如果不是已经显示加载状态，添加加载标识
            if (!currentText.contains("(加载中...)")) {
                setTabText(index, currentText + " (加载中...)");
            }
        } else {
            // 移除加载状态标识
            QString newText = currentText;
            newText.remove(" (加载中...)");
            setTabText(index, newText);
        }
    }
}

void DocumentTabWidget::moveTab(int from, int to) {
    if (from == to || from < 0 || to < 0 || from >= count() || to >= count()) {
        return;
    }

    // 保存标签页信息
    QString text = tabText(from);
    QString toolTip = tabToolTip(from);
    QWidget* widget = this->widget(from);
    QString filePath = tabFilePaths.value(from);

    // 移除原标签页
    removeTab(from);

    // 在新位置插入
    insertTab(to, widget, text);
    setTabToolTip(to, toolTip);

    // 更新文件路径映射
    tabFilePaths.remove(from);
    tabFilePaths[to] = filePath;

    // 设置为当前标签页
    setCurrentIndex(to);
}

QString DocumentTabWidget::getTabFilePath(int index) const {
    return tabFilePaths.value(index, QString());
}

int DocumentTabWidget::getTabCount() const { return count(); }

QWidget* DocumentTabWidget::createTabWidget(const QString& fileName,
                                            const QString& filePath) {
    QWidget* widget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(widget);

    QLabel* label = new QLabel(QString("PDF内容: %1").arg(fileName), widget);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);

    return widget;
}

void DocumentTabWidget::onTabCloseRequested(int index) {
    emit tabCloseRequested(index);
}

void DocumentTabWidget::onTabMoveRequested(int from, int to) {
    moveTab(from, to);
    emit tabMoved(from, to);
}
