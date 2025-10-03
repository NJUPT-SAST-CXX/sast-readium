#include "RecentFileListWidget.h"
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>
#include "../../managers/FileTypeIconManager.h"
#include "../../managers/RecentFilesManager.h"
#include "../../managers/StyleManager.h"

// Static const member definitions
const int RecentFileItemWidget::ITEM_HEIGHT;
const int RecentFileItemWidget::PADDING;
const int RecentFileItemWidget::SPACING;

const int RecentFileListWidget::MAX_VISIBLE_ITEMS;
const int RecentFileListWidget::REFRESH_DELAY;

// RecentFileItemWidget Implementation
RecentFileItemWidget::RecentFileItemWidget(const RecentFileInfo& fileInfo,
                                           QWidget* parent)
    : QFrame(parent),
      m_fileInfo(fileInfo),
      m_mainLayout(nullptr),
      m_infoLayout(nullptr),
      m_fileIconLabel(nullptr),
      m_fileNameLabel(nullptr),
      m_filePathLabel(nullptr),
      m_lastOpenedLabel(nullptr),
      m_removeButton(nullptr),
      m_isHovered(false),
      m_isPressed(false),
      m_hoverAnimation(nullptr),
      m_pressAnimation(nullptr),
      m_opacityEffect(nullptr),
      m_currentOpacity(1.0) {
    setObjectName("RecentFileItemWidget");
    setFixedHeight(ITEM_HEIGHT);
    setFrameShape(QFrame::NoFrame);
    setCursor(Qt::PointingHandCursor);

    setupUI();
    setupAnimations();
    updateDisplay();
    applyTheme();
}

RecentFileItemWidget::~RecentFileItemWidget() {}

void RecentFileItemWidget::updateFileInfo(const RecentFileInfo& fileInfo) {
    m_fileInfo = fileInfo;
    updateDisplay();
}

void RecentFileItemWidget::applyTheme() {
    StyleManager& styleManager = StyleManager::instance();

    // VSCode-style base styling with subtle hover effect
    QString baseStyle = QString(
                            "RecentFileItemWidget {"
                            "    background-color: transparent;"
                            "    border: none;"
                            "    border-radius: 6px;"
                            "    padding: 8px 12px;"
                            "}"
                            "RecentFileItemWidget:hover {"
                            "    background-color: %1;"
                            "}")
                            .arg(styleManager.hoverColor().name());

    setStyleSheet(baseStyle);

    // VSCode-style file name label - prominent and clean
    if (m_fileNameLabel) {
        m_fileNameLabel->setStyleSheet(
            QString("QLabel {"
                    "    color: %1;"
                    "    font-size: 13px;"
                    "    font-weight: 500;"
                    "    margin: 0px;"
                    "    padding: 0px;"
                    "}")
                .arg(styleManager.textColor().name()));
    }

    // VSCode-style path label - smaller and muted
    if (m_filePathLabel) {
        m_filePathLabel->setStyleSheet(
            QString("QLabel {"
                    "    color: %1;"
                    "    font-size: 11px;"
                    "    font-weight: 400;"
                    "    margin: 0px;"
                    "    padding: 0px;"
                    "}")
                .arg(styleManager.textSecondaryColor().name()));
    }

    // VSCode-style time label - very small and subtle
    if (m_lastOpenedLabel) {
        m_lastOpenedLabel->setStyleSheet(
            QString("QLabel {"
                    "    color: %1;"
                    "    font-size: 10px;"
                    "    font-weight: 400;"
                    "    margin: 0px;"
                    "    padding: 0px;"
                    "}")
                .arg(styleManager.textSecondaryColor().name()));
    }

    // VSCode-style remove button - subtle and only visible on hover
    if (m_removeButton) {
        m_removeButton->setStyleSheet(
            QString("QPushButton {"
                    "    background-color: transparent;"
                    "    border: none;"
                    "    color: %1;"
                    "    font-size: 14px;"
                    "    font-weight: bold;"
                    "    width: 18px;"
                    "    height: 18px;"
                    "    border-radius: 9px;"
                    "    padding: 0px;"
                    "}"
                    "QPushButton:hover {"
                    "    background-color: %2;"
                    "    color: %3;"
                    "}")
                .arg(styleManager.textSecondaryColor().name())
                .arg(styleManager.pressedColor().name())
                .arg(styleManager.textColor().name()));
    }
}

void RecentFileItemWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_isPressed = true;
        startPressAnimation();
        update();
    }
    QFrame::mousePressEvent(event);
}

void RecentFileItemWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_isPressed) {
        m_isPressed = false;
        if (rect().contains(event->pos())) {
            emit clicked(m_fileInfo.filePath);
        }
        update();
    }
    QFrame::mouseReleaseEvent(event);
}

void RecentFileItemWidget::enterEvent(QEnterEvent* event) {
    setHovered(true);
    QFrame::enterEvent(event);
}

void RecentFileItemWidget::leaveEvent(QEvent* event) {
    setHovered(false);
    QFrame::leaveEvent(event);
}

void RecentFileItemWidget::paintEvent(QPaintEvent* event) {
    QFrame::paintEvent(event);

    if (m_isPressed) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        StyleManager& styleManager = StyleManager::instance();
        QColor pressedColor = styleManager.pressedColor();
        pressedColor.setAlpha(100);

        painter.fillRect(rect(), pressedColor);
    }
}

void RecentFileItemWidget::onRemoveClicked() {
    emit removeRequested(m_fileInfo.filePath);
}

void RecentFileItemWidget::setupUI() {
    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setContentsMargins(16, 12, 16, 12);
    m_mainLayout->setSpacing(12);

    // 文件类型图标
    m_fileIconLabel = new QLabel();
    m_fileIconLabel->setObjectName("RecentFileIconLabel");
    m_fileIconLabel->setFixedSize(32, 32);
    m_fileIconLabel->setScaledContents(true);
    m_fileIconLabel->setAlignment(Qt::AlignCenter);

    // 文件信息区域
    m_infoLayout = new QVBoxLayout();
    m_infoLayout->setContentsMargins(0, 0, 0, 0);
    m_infoLayout->setSpacing(4);

    m_fileNameLabel = new QLabel();
    m_fileNameLabel->setObjectName("RecentFileNameLabel");

    m_filePathLabel = new QLabel();
    m_filePathLabel->setObjectName("RecentFilePathLabel");

    m_lastOpenedLabel = new QLabel();
    m_lastOpenedLabel->setObjectName("RecentFileLastOpenedLabel");

    m_infoLayout->addWidget(m_fileNameLabel);
    m_infoLayout->addWidget(m_filePathLabel);
    m_infoLayout->addWidget(m_lastOpenedLabel);
    m_infoLayout->addStretch();

    // 移除按钮
    m_removeButton = new QPushButton("×");
    m_removeButton->setObjectName("RecentFileRemoveButton");
    m_removeButton->setCursor(Qt::PointingHandCursor);
    m_removeButton->setToolTip("Remove from recent files");
    m_removeButton->setVisible(false);  // Initially hidden, shown on hover
    connect(m_removeButton, &QPushButton::clicked, this,
            &RecentFileItemWidget::onRemoveClicked);

    // Layout assembly
    m_mainLayout->addWidget(m_fileIconLabel, 0, Qt::AlignTop);
    m_mainLayout->addLayout(m_infoLayout, 1);
    m_mainLayout->addWidget(m_removeButton, 0, Qt::AlignTop);
}

void RecentFileItemWidget::setupAnimations() {
    // Setup opacity effect for smooth animations
    m_opacityEffect = new QGraphicsOpacityEffect(this);
    m_opacityEffect->setOpacity(1.0);
    setGraphicsEffect(m_opacityEffect);

    // Hover animation
    m_hoverAnimation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_hoverAnimation->setDuration(200);
    m_hoverAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // Press animation
    m_pressAnimation = new QPropertyAnimation(this, "geometry", this);
    m_pressAnimation->setDuration(100);
    m_pressAnimation->setEasingCurve(QEasingCurve::OutQuad);
}

void RecentFileItemWidget::updateDisplay() {
    if (!m_fileNameLabel || !m_filePathLabel || !m_lastOpenedLabel ||
        !m_fileIconLabel)
        return;

    // 更新文件类型图标
    QIcon fileIcon = FILE_ICON_MANAGER.getFileTypeIcon(m_fileInfo.filePath, 32);
    m_fileIconLabel->setPixmap(fileIcon.pixmap(32, 32));

    // 更新文件名 - VSCode style: just the filename without extension for
    // display
    QString displayName = m_fileInfo.fileName;
    if (displayName.isEmpty()) {
        QFileInfo fileInfo(m_fileInfo.filePath);
        displayName = fileInfo.baseName();  // Get filename without extension
        if (displayName.isEmpty()) {
            displayName = fileInfo.fileName();  // Fallback to full filename
        }
    } else {
        // Remove extension for cleaner display like VSCode
        QFileInfo fileInfo(displayName);
        displayName = fileInfo.baseName();
        if (displayName.isEmpty()) {
            displayName = m_fileInfo.fileName;
        }
    }
    m_fileNameLabel->setText(displayName);

    // 更新文件路径 - VSCode style: show directory path, not full path
    QString displayPath = m_fileInfo.filePath;
    QFileInfo fileInfo(displayPath);
    QString dirPath = fileInfo.absolutePath();

    // Shorten path like VSCode does
    if (dirPath.length() > 50) {
        QStringList pathParts =
            dirPath.split(QDir::separator(), Qt::SkipEmptyParts);
        if (pathParts.size() > 2) {
            displayPath =
                QString("...") + QDir::separator() + pathParts.takeLast();
            if (pathParts.size() > 0) {
                displayPath = QString("...") + QDir::separator() +
                              pathParts.takeLast() + QDir::separator() +
                              pathParts.takeLast();
            }
        } else {
            displayPath = dirPath;
        }
    } else {
        displayPath = dirPath;
    }
    m_filePathLabel->setText(displayPath);

    // 更新最后打开时间 - VSCode style: simpler format
    QString timeText;
    QDateTime now = QDateTime::currentDateTime();
    qint64 secondsAgo = m_fileInfo.lastOpened.secsTo(now);

    if (secondsAgo < 60) {
        timeText = "now";
    } else if (secondsAgo < 3600) {
        int minutes = secondsAgo / 60;
        timeText = QString("%1m ago").arg(minutes);
    } else if (secondsAgo < 86400) {
        int hours = secondsAgo / 3600;
        timeText = QString("%1h ago").arg(hours);
    } else if (secondsAgo < 604800) {
        int days = secondsAgo / 86400;
        timeText = QString("%1d ago").arg(days);
    } else {
        timeText = m_fileInfo.lastOpened.toString("MMM dd");
    }

    m_lastOpenedLabel->setText(timeText);

    // 设置工具提示
    setToolTip(QString("%1\n%2\nLast opened: %3")
                   .arg(m_fileInfo.fileName, m_fileInfo.filePath,
                        m_fileInfo.lastOpened.toString()));
}

void RecentFileItemWidget::setHovered(bool hovered) {
    if (m_isHovered == hovered)
        return;

    m_isHovered = hovered;

    // 显示/隐藏移除按钮
    if (m_removeButton) {
        m_removeButton->setVisible(hovered);
    }

    // Start hover animation
    startHoverAnimation(hovered);

    update();
}

void RecentFileItemWidget::startHoverAnimation(bool hovered) {
    if (!m_hoverAnimation || !m_opacityEffect)
        return;

    m_hoverAnimation->stop();

    if (hovered) {
        m_hoverAnimation->setStartValue(m_opacityEffect->opacity());
        m_hoverAnimation->setEndValue(0.9);
    } else {
        m_hoverAnimation->setStartValue(m_opacityEffect->opacity());
        m_hoverAnimation->setEndValue(1.0);
    }

    m_hoverAnimation->start();
}

void RecentFileItemWidget::startPressAnimation() {
    if (!m_pressAnimation)
        return;

    QRect currentGeometry = geometry();
    QRect pressedGeometry = currentGeometry.adjusted(2, 2, -2, -2);

    m_pressAnimation->stop();
    m_pressAnimation->setStartValue(currentGeometry);
    m_pressAnimation->setEndValue(pressedGeometry);
    m_pressAnimation->start();

    // Return to normal size after a short delay
    QTimer::singleShot(100, [this, currentGeometry]() {
        if (m_pressAnimation) {
            m_pressAnimation->setStartValue(geometry());
            m_pressAnimation->setEndValue(currentGeometry);
            m_pressAnimation->start();
        }
    });
}

// RecentFileListWidget Implementation
RecentFileListWidget::RecentFileListWidget(QWidget* parent)
    : QWidget(parent),
      m_recentFilesManager(nullptr),
      m_mainLayout(nullptr),
      m_scrollArea(nullptr),
      m_contentWidget(nullptr),
      m_contentLayout(nullptr),
      m_emptyLabel(nullptr),
      m_refreshTimer(nullptr),
      m_isInitialized(false) {
    setObjectName("RecentFileListWidget");

    setupUI();

    // 设置刷新定时器
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setSingleShot(true);
    m_refreshTimer->setInterval(REFRESH_DELAY);
    connect(m_refreshTimer, &QTimer::timeout, this,
            &RecentFileListWidget::onRefreshTimer);

    m_isInitialized = true;
    updateEmptyState();
}

RecentFileListWidget::~RecentFileListWidget() {}

void RecentFileListWidget::setRecentFilesManager(RecentFilesManager* manager) {
    if (m_recentFilesManager == manager)
        return;

    // 断开旧连接
    if (m_recentFilesManager) {
        disconnect(m_recentFilesManager, nullptr, this, nullptr);
    }

    m_recentFilesManager = manager;

    // 建立新连接
    if (m_recentFilesManager) {
        connect(m_recentFilesManager, &RecentFilesManager::recentFilesChanged,
                this, &RecentFileListWidget::onRecentFilesChanged);
    }

    // 刷新列表
    refreshList();
}

void RecentFileListWidget::refreshList() {
    if (!m_recentFilesManager) {
        clearList();
        return;
    }

    qDebug() << "RecentFileListWidget: Refreshing list...";

    // 清空现有列表
    clearList();

    // 获取最近文件列表
    QList<RecentFileInfo> recentFiles = m_recentFilesManager->getRecentFiles();

    // 限制显示数量
    int maxItems = qMin(recentFiles.size(), MAX_VISIBLE_ITEMS);

    // 添加文件条目
    for (int i = 0; i < maxItems; ++i) {
        const RecentFileInfo& fileInfo = recentFiles[i];
        if (fileInfo.isValid()) {
            addFileItem(fileInfo);
        }
    }

    updateEmptyState();

    qDebug() << "RecentFileListWidget: List refreshed with"
             << m_fileItems.size() << "items";
}

void RecentFileListWidget::clearList() {
    qDebug() << "RecentFileListWidget: Clearing list...";

    // 删除所有文件条目
    for (RecentFileItemWidget* item : m_fileItems) {
        if (item) {
            m_contentLayout->removeWidget(qobject_cast<QWidget*>(item));
            item->deleteLater();
        }
    }
    m_fileItems.clear();

    updateEmptyState();
}

void RecentFileListWidget::applyTheme() {
    if (!m_isInitialized)
        return;

    qDebug() << "RecentFileListWidget: Applying theme...";

    StyleManager& styleManager = StyleManager::instance();

    // 更新空状态标签样式
    if (m_emptyLabel) {
        m_emptyLabel->setStyleSheet(
            QString("QLabel {"
                    "    color: %1;"
                    "    font-size: 14px;"
                    "    margin: 20px;"
                    "}")
                .arg(styleManager.textSecondaryColor().name()));
    }

    // 更新滚动区域样式
    if (m_scrollArea) {
        m_scrollArea->setStyleSheet(
            QString("QScrollArea {"
                    "    background-color: transparent;"
                    "    border: none;"
                    "}"
                    "QScrollBar:vertical {"
                    "    background-color: %1;"
                    "    width: 8px;"
                    "    border-radius: 4px;"
                    "}"
                    "QScrollBar::handle:vertical {"
                    "    background-color: %2;"
                    "    border-radius: 4px;"
                    "    min-height: 20px;"
                    "}"
                    "QScrollBar::handle:vertical:hover {"
                    "    background-color: %3;"
                    "}")
                .arg(styleManager.surfaceColor().name())
                .arg(styleManager.borderColor().name())
                .arg(styleManager.textSecondaryColor().name()));
    }

    // 应用主题到所有文件条目
    for (RecentFileItemWidget* item : m_fileItems) {
        item->applyTheme();
    }
}

bool RecentFileListWidget::isEmpty() const { return m_fileItems.isEmpty(); }

int RecentFileListWidget::itemCount() const { return m_fileItems.size(); }

void RecentFileListWidget::onRecentFilesChanged() {
    qDebug()
        << "RecentFileListWidget: Recent files changed, scheduling refresh...";
    scheduleRefresh();
}

void RecentFileListWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);

    // 确保内容宽度适应
    if (m_contentWidget) {
        m_contentWidget->setFixedWidth(event->size().width());
    }
}

void RecentFileListWidget::onItemClicked(const QString& filePath) {
    qDebug() << "RecentFileListWidget: Item clicked:" << filePath;
    emit fileClicked(filePath);
}

void RecentFileListWidget::onItemRemoveRequested(const QString& filePath) {
    qDebug() << "RecentFileListWidget: Remove requested for:" << filePath;

    // 从管理器中移除文件
    if (m_recentFilesManager) {
        m_recentFilesManager->removeRecentFile(filePath);
    }

    emit fileRemoveRequested(filePath);
}

void RecentFileListWidget::onRefreshTimer() { refreshList(); }

void RecentFileListWidget::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // 创建滚动区域 - VSCode style
    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setObjectName("RecentFileListScrollArea");

    // 创建内容容器 - VSCode style
    m_contentWidget = new QWidget();
    m_contentWidget->setObjectName("RecentFileListContentWidget");

    // VSCode-style layout with proper spacing
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(4, 4, 4,
                                        4);  // Small margins like VSCode
    m_contentLayout->setSpacing(1);          // Minimal spacing between items
    m_contentLayout->setAlignment(Qt::AlignTop);

    // VSCode-style empty state label
    m_emptyLabel = new QLabel("No recent files");
    m_emptyLabel->setObjectName("RecentFileListEmptyLabel");
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setVisible(false);

    m_contentLayout->addWidget(m_emptyLabel);
    m_contentLayout->addStretch();

    m_scrollArea->setWidget(m_contentWidget);
    m_mainLayout->addWidget(m_scrollArea);
}

void RecentFileListWidget::setupConnections() {
    // 连接已在setRecentFilesManager中处理
}

void RecentFileListWidget::addFileItem(const RecentFileInfo& fileInfo) {
    RecentFileItemWidget* item = new RecentFileItemWidget(fileInfo, this);

    connect(item, &RecentFileItemWidget::clicked, this,
            &RecentFileListWidget::onItemClicked);
    connect(item, &RecentFileItemWidget::removeRequested, this,
            &RecentFileListWidget::onItemRemoveRequested);

    // 插入到布局中（在空标签和弹性空间之前）
    int insertIndex = m_contentLayout->count() - 1;  // 在弹性空间之前
    if (m_emptyLabel && m_emptyLabel->isVisible()) {
        insertIndex = m_contentLayout->count() - 2;  // 在空标签和弹性空间之前
    }

    m_contentLayout->insertWidget(insertIndex, item);
    m_fileItems.append(item);

    // 应用主题
    item->applyTheme();
}

void RecentFileListWidget::removeFileItem(const QString& filePath) {
    for (int i = 0; i < m_fileItems.size(); ++i) {
        RecentFileItemWidget* item = m_fileItems[i];
        if (item->fileInfo().filePath == filePath) {
            m_contentLayout->removeWidget(qobject_cast<QWidget*>(item));
            m_fileItems.removeAt(i);
            item->deleteLater();
            break;
        }
    }

    updateEmptyState();
}

void RecentFileListWidget::updateEmptyState() {
    bool isEmpty = m_fileItems.isEmpty();

    if (m_emptyLabel) {
        m_emptyLabel->setVisible(isEmpty);
    }
}

void RecentFileListWidget::scheduleRefresh() {
    if (m_refreshTimer && !m_refreshTimer->isActive()) {
        m_refreshTimer->start();
    }
}
