#include "RecentFileListWidget.h"
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include "../../managers/FileTypeIconManager.h"
#include "../../managers/RecentFilesManager.h"
#include "../../managers/StyleManager.h"
#include "ElaComboBox.h"
#include "ElaLineEdit.h"
#include "ElaMenu.h"
#include "ElaPushButton.h"
#include "ElaText.h"
#include "ElaToolButton.h"

// Static const member definitions
const int RecentFileItemWidget::ITEM_HEIGHT_DETAILED;
const int RecentFileItemWidget::ITEM_HEIGHT_COMPACT;
const int RecentFileItemWidget::PADDING;
const int RecentFileItemWidget::SPACING;
const int RecentFileItemWidget::ICON_SIZE_DETAILED;
const int RecentFileItemWidget::ICON_SIZE_COMPACT;

const int RecentFileListWidget::MAX_VISIBLE_ITEMS;
const int RecentFileListWidget::REFRESH_DELAY;

// RecentFileItemWidget Implementation
RecentFileItemWidget::RecentFileItemWidget(const RecentFileInfo& fileInfo,
                                           RecentFileViewMode viewMode,
                                           QWidget* parent)
    : QFrame(parent),
      m_fileInfo(fileInfo),
      m_viewMode(viewMode),
      m_mainLayout(nullptr),
      m_infoLayout(nullptr),
      m_fileIconLabel(nullptr),
      m_fileNameLabel(nullptr),
      m_filePathLabel(nullptr),
      m_lastOpenedLabel(nullptr),
      m_fileSizeLabel(nullptr),
      m_pageCountLabel(nullptr),
      m_thumbnailLabel(nullptr),
      m_removeButton(nullptr),
      m_pinButton(nullptr),
      m_isHovered(false),
      m_isPressed(false),
      m_hoverAnimation(nullptr),
      m_pressAnimation(nullptr),
      m_opacityEffect(nullptr),
      m_currentOpacity(1.0) {
    setObjectName("RecentFileItemWidget");
    setFixedHeight(viewMode == RecentFileViewMode::Compact
                       ? ITEM_HEIGHT_COMPACT
                       : ITEM_HEIGHT_DETAILED);
    setFrameShape(QFrame::NoFrame);
    setCursor(Qt::PointingHandCursor);

    setupUI();
    setupAnimations();
    updateDisplay();
    applyTheme();
}

RecentFileItemWidget::~RecentFileItemWidget() = default;

void RecentFileItemWidget::updateFileInfo(const RecentFileInfo& fileInfo) {
    m_fileInfo = fileInfo;
    updateDisplay();
}

void RecentFileItemWidget::setViewMode(RecentFileViewMode mode) {
    if (m_viewMode == mode) {
        return;
    }

    m_viewMode = mode;
    setFixedHeight(mode == RecentFileViewMode::Compact ? ITEM_HEIGHT_COMPACT
                                                       : ITEM_HEIGHT_DETAILED);
    updateLayoutForViewMode();
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
            QString("QLabel, ElaText {"
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
            QString("QLabel, ElaText {"
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
            QString("QLabel, ElaText {"
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
            QString("QPushButton, ElaPushButton {"
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
                    "QPushButton:hover, ElaPushButton:hover {"
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
    } else if (event->button() == Qt::RightButton) {
        // Show context menu for additional actions
        showContextMenu(event->globalPosition().toPoint());
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

void RecentFileItemWidget::showContextMenu(const QPoint& globalPos) {
    ElaMenu contextMenu(this);
    contextMenu.setTitle(tr("Recent File Actions"));

    QAction* openAction = contextMenu.addAction(tr("Open"));
    openAction->setShortcut(QKeySequence::Open);
    openAction->setIcon(QIcon(":/icons/open"));

    QAction* openInNewTabAction = contextMenu.addAction(tr("Open in New Tab"));
    openInNewTabAction->setShortcut(QKeySequence("Ctrl+T"));
    openInNewTabAction->setIcon(QIcon(":/icons/new-tab"));

    contextMenu.addSeparator();

    // Pin/Unpin action
    QAction* pinAction = contextMenu.addAction(
        m_fileInfo.isPinned ? tr("Unpin from Top") : tr("Pin to Top"));
    pinAction->setIcon(
        QIcon(m_fileInfo.isPinned ? ":/icons/unpin" : ":/icons/pin"));

    // Open containing folder action
    QAction* openFolderAction =
        contextMenu.addAction(tr("Open Containing Folder"));
    openFolderAction->setIcon(QIcon(":/icons/folder"));

    contextMenu.addSeparator();

    QAction* removeAction = contextMenu.addAction(tr("Remove from Recent"));
    removeAction->setIcon(QIcon(":/icons/remove"));

    QAction* clearAllAction =
        contextMenu.addAction(tr("Clear All Recent Files"));
    clearAllAction->setIcon(QIcon(":/icons/clear-all"));

    // Connect actions
    connect(openAction, &QAction::triggered,
            [this]() { emit clicked(m_fileInfo.filePath); });

    connect(openInNewTabAction, &QAction::triggered,
            [this]() { emit openInNewTabRequested(m_fileInfo.filePath); });

    connect(pinAction, &QAction::triggered,
            [this]() { emit pinToggleRequested(m_fileInfo.filePath); });

    connect(openFolderAction, &QAction::triggered, [this]() {
        emit openContainingFolderRequested(m_fileInfo.filePath);
    });

    connect(removeAction, &QAction::triggered,
            [this]() { emit removeRequested(m_fileInfo.filePath); });

    connect(clearAllAction, &QAction::triggered,
            [this]() { emit clearAllRecentRequested(); });

    // Show the context menu
    contextMenu.exec(globalPos);
}

void RecentFileItemWidget::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            emit clicked(m_fileInfo.filePath);
            break;
        case Qt::Key_Delete:
        case Qt::Key_Backspace:
            emit removeRequested(m_fileInfo.filePath);
            break;
        case Qt::Key_Down:
            // Navigate to next item
            if (nextInFocusChain()) {
                nextInFocusChain()->setFocus();
            }
            break;
        case Qt::Key_Up:
            // Navigate to previous item
            if (previousInFocusChain()) {
                previousInFocusChain()->setFocus();
            }
            break;
        default:
            QFrame::keyPressEvent(event);
            break;
    }
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
    int iconSize = m_viewMode == RecentFileViewMode::Compact
                       ? ICON_SIZE_COMPACT
                       : ICON_SIZE_DETAILED;
    m_fileIconLabel = new QLabel();
    m_fileIconLabel->setObjectName("RecentFileIconLabel");
    m_fileIconLabel->setFixedSize(iconSize, iconSize);
    m_fileIconLabel->setScaledContents(true);
    m_fileIconLabel->setAlignment(Qt::AlignCenter);

    // 文件信息区域
    m_infoLayout = new QVBoxLayout();
    m_infoLayout->setContentsMargins(0, 0, 0, 0);
    m_infoLayout->setSpacing(4);

    m_fileNameLabel = new ElaText();
    m_fileNameLabel->setObjectName("RecentFileNameLabel");

    m_filePathLabel = new ElaText();
    m_filePathLabel->setObjectName("RecentFilePathLabel");

    m_lastOpenedLabel = new ElaText();
    m_lastOpenedLabel->setObjectName("RecentFileLastOpenedLabel");

    // Additional labels for detailed view
    m_fileSizeLabel = new ElaText();
    m_fileSizeLabel->setObjectName("RecentFileFileSizeLabel");

    m_pageCountLabel = new ElaText();
    m_pageCountLabel->setObjectName("RecentFilePageCountLabel");

    m_thumbnailLabel = new QLabel();
    m_thumbnailLabel->setObjectName("RecentFileThumbnailLabel");
    m_thumbnailLabel->setFixedSize(48, 64);
    m_thumbnailLabel->setScaledContents(true);
    m_thumbnailLabel->setVisible(
        false);  // Optional, shown in detailed view if available

    // Pin button
    m_pinButton = new ElaPushButton();
    m_pinButton->setObjectName("RecentFilePinButton");
    m_pinButton->setCursor(Qt::PointingHandCursor);
    m_pinButton->setToolTip(tr("Pin to top"));
    m_pinButton->setFixedSize(24, 24);
    m_pinButton->setVisible(false);  // Initially hidden, shown on hover
    connect(m_pinButton, &QPushButton::clicked, this,
            [this]() { emit pinToggleRequested(m_fileInfo.filePath); });

    // 移除按钮
    m_removeButton = new ElaPushButton("×");
    m_removeButton->setObjectName("RecentFileRemoveButton");
    m_removeButton->setCursor(Qt::PointingHandCursor);
    m_removeButton->setToolTip(tr("Remove from recent files"));
    m_removeButton->setVisible(false);  // Initially hidden, shown on hover
    connect(m_removeButton, &QPushButton::clicked, this,
            &RecentFileItemWidget::onRemoveClicked);

    // Layout assembly based on view mode
    updateLayoutForViewMode();
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

void RecentFileItemWidget::updateLayoutForViewMode() {
    // Clear existing layout
    while (m_mainLayout->count() > 0) {
        QLayoutItem* item = m_mainLayout->takeAt(0);
        if (item->widget()) {
            item->widget()->setParent(nullptr);
        } else if (item->layout()) {
            // Don't delete the info layout, we'll reuse it
        }
        delete item;
    }

    // Clear info layout
    while (m_infoLayout->count() > 0) {
        QLayoutItem* item = m_infoLayout->takeAt(0);
        if (item->widget()) {
            item->widget()->setParent(nullptr);
        }
        delete item;
    }

    // Update icon size
    int iconSize = m_viewMode == RecentFileViewMode::Compact
                       ? ICON_SIZE_COMPACT
                       : ICON_SIZE_DETAILED;
    if (m_fileIconLabel) {
        m_fileIconLabel->setFixedSize(iconSize, iconSize);
    }

    if (m_viewMode == RecentFileViewMode::Compact) {
        // Compact view: filename, icon, and date only
        m_infoLayout->addWidget(m_fileNameLabel);
        m_infoLayout->addWidget(m_lastOpenedLabel);

        m_mainLayout->addWidget(m_fileIconLabel, 0, Qt::AlignVCenter);
        m_mainLayout->addLayout(m_infoLayout, 1);
        m_mainLayout->addWidget(m_pinButton, 0, Qt::AlignVCenter);
        m_mainLayout->addWidget(m_removeButton, 0, Qt::AlignVCenter);

        // Hide detailed view components
        if (m_filePathLabel)
            m_filePathLabel->setVisible(false);
        if (m_fileSizeLabel)
            m_fileSizeLabel->setVisible(false);
        if (m_pageCountLabel)
            m_pageCountLabel->setVisible(false);
        if (m_thumbnailLabel)
            m_thumbnailLabel->setVisible(false);

    } else {
        // Detailed view: all information
        m_infoLayout->addWidget(m_fileNameLabel);
        m_infoLayout->addWidget(m_filePathLabel);

        // Create a horizontal layout for metadata (size, page count, date)
        QHBoxLayout* metadataLayout = new QHBoxLayout();
        metadataLayout->setSpacing(12);
        metadataLayout->addWidget(m_fileSizeLabel);
        metadataLayout->addWidget(m_pageCountLabel);
        metadataLayout->addWidget(m_lastOpenedLabel);
        metadataLayout->addStretch();

        m_infoLayout->addLayout(metadataLayout);

        m_mainLayout->addWidget(m_fileIconLabel, 0, Qt::AlignTop);
        m_mainLayout->addLayout(m_infoLayout, 1);
        // m_mainLayout->addWidget(m_thumbnailLabel, 0, Qt::AlignTop);  //
        // Optional thumbnail
        m_mainLayout->addWidget(m_pinButton, 0, Qt::AlignTop);
        m_mainLayout->addWidget(m_removeButton, 0, Qt::AlignTop);

        // Show detailed view components
        if (m_filePathLabel)
            m_filePathLabel->setVisible(true);
        if (m_fileSizeLabel)
            m_fileSizeLabel->setVisible(true);
        if (m_pageCountLabel)
            m_pageCountLabel->setVisible(true);
    }
}

void RecentFileItemWidget::updateDisplay() {
    if (!m_fileNameLabel || !m_filePathLabel || !m_lastOpenedLabel ||
        !m_fileIconLabel) {
        return;
    }

    // 更新文件类型图标
    int iconSize = m_viewMode == RecentFileViewMode::Compact
                       ? ICON_SIZE_COMPACT
                       : ICON_SIZE_DETAILED;
    QIcon fileIcon =
        FILE_ICON_MANAGER.getFileTypeIcon(m_fileInfo.filePath, iconSize);
    m_fileIconLabel->setPixmap(fileIcon.pixmap(iconSize, iconSize));

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

    // Add pin indicator to filename if pinned
    if (m_fileInfo.isPinned) {
        displayName = "📌 " + displayName;
    }
    m_fileNameLabel->setText(displayName);

    // 更新文件路径 - VSCode style: show directory path, not full path
    if (m_filePathLabel && m_viewMode == RecentFileViewMode::Detailed) {
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
    }

    // 更新文件大小
    if (m_fileSizeLabel && m_viewMode == RecentFileViewMode::Detailed) {
        QString sizeText;
        qint64 size = m_fileInfo.fileSize;
        if (size < 1024) {
            sizeText = QString("%1 B").arg(size);
        } else if (size < 1024 * 1024) {
            sizeText = QString("%1 KB").arg(size / 1024.0, 0, 'f', 1);
        } else if (size < 1024 * 1024 * 1024) {
            sizeText =
                QString("%1 MB").arg(size / (1024.0 * 1024.0), 0, 'f', 1);
        } else {
            sizeText = QString("%1 GB").arg(size / (1024.0 * 1024.0 * 1024.0),
                                            0, 'f', 2);
        }
        m_fileSizeLabel->setText(sizeText);
    }

    // 更新页数
    if (m_pageCountLabel && m_viewMode == RecentFileViewMode::Detailed) {
        if (m_fileInfo.pageCount > 0) {
            m_pageCountLabel->setText(tr("%1 pages").arg(m_fileInfo.pageCount));
        } else {
            m_pageCountLabel->setText("");
        }
    }

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

    // Update pin button
    if (m_pinButton) {
        m_pinButton->setText(m_fileInfo.isPinned ? "📌" : "📍");
        m_pinButton->setToolTip(m_fileInfo.isPinned ? tr("Unpin from top")
                                                    : tr("Pin to top"));
    }

    // 设置工具提示
    QString tooltip = QString("%1\n%2\nLast opened: %3")
                          .arg(m_fileInfo.fileName, m_fileInfo.filePath,
                               m_fileInfo.lastOpened.toString());
    if (m_fileInfo.pageCount > 0) {
        tooltip += QString("\nPages: %1").arg(m_fileInfo.pageCount);
    }
    setToolTip(tooltip);
}

void RecentFileItemWidget::setHovered(bool hovered) {
    if (m_isHovered == hovered) {
        return;
    }

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
    if (!m_hoverAnimation || !m_opacityEffect) {
        return;
    }

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
    if (!m_pressAnimation) {
        return;
    }

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
      m_toolbarWidget(nullptr),
      m_toolbarLayout(nullptr),
      m_viewModeButton(nullptr),
      m_sortComboBox(nullptr),
      m_searchLineEdit(nullptr),
      m_clearAllButton(nullptr),
      m_scrollArea(nullptr),
      m_contentWidget(nullptr),
      m_contentLayout(nullptr),
      m_emptyLabel(nullptr),
      m_refreshTimer(nullptr),
      m_isInitialized(false),
      m_viewMode(RecentFileViewMode::Detailed),
      m_sortOrder(RecentFilesManager::SortOrder::ByDate),
      m_searchFilter("") {
    setObjectName("RecentFileListWidget");

    setupUI();

    // 设置刷新定时器
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setSingleShot(true);
    m_refreshTimer->setInterval(REFRESH_DELAY);
    connect(m_refreshTimer, &QTimer::timeout, this,
            &RecentFileListWidget::onRefreshTimer);

    // Load view mode preference
    QSettings settings("SAST", "Readium-RecentFiles");
    int viewModeInt =
        settings
            .value("viewMode", static_cast<int>(RecentFileViewMode::Detailed))
            .toInt();
    m_viewMode = static_cast<RecentFileViewMode>(viewModeInt);

    m_isInitialized = true;
    updateEmptyState();
}

RecentFileListWidget::~RecentFileListWidget() {
    // Save view mode preference
    QSettings settings("SAST", "Readium-RecentFiles");
    settings.setValue("viewMode", static_cast<int>(m_viewMode));
}

void RecentFileListWidget::setRecentFilesManager(RecentFilesManager* manager) {
    if (m_recentFilesManager == manager) {
        return;
    }

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

    // 获取排序后的最近文件列表
    QList<RecentFileInfo> recentFiles =
        m_recentFilesManager->getSortedRecentFiles(m_sortOrder);

    // Apply search filter if set
    QList<RecentFileInfo> filteredFiles;
    if (!m_searchFilter.isEmpty()) {
        QString filterLower = m_searchFilter.toLower();
        for (const RecentFileInfo& fileInfo : recentFiles) {
            if (fileInfo.fileName.toLower().contains(filterLower) ||
                fileInfo.filePath.toLower().contains(filterLower)) {
                filteredFiles.append(fileInfo);
            }
        }
    } else {
        filteredFiles = recentFiles;
    }

    // 限制显示数量
    int maxItems = qMin(filteredFiles.size(), MAX_VISIBLE_ITEMS);

    // 添加文件条目
    for (int i = 0; i < maxItems; ++i) {
        const RecentFileInfo& fileInfo = filteredFiles[i];
        if (fileInfo.isValid()) {
            addFileItem(fileInfo);
        }
    }

    updateEmptyState();

    qDebug() << "RecentFileListWidget: List refreshed with"
             << m_fileItems.size() << "items (filtered from"
             << filteredFiles.size() << "total)";
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
    if (!m_isInitialized) {
        return;
    }

    qDebug() << "RecentFileListWidget: Applying theme...";

    StyleManager& styleManager = StyleManager::instance();

    // 更新空状态标签样式
    if (m_emptyLabel) {
        m_emptyLabel->setStyleSheet(
            QString("QLabel, ElaText {"
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

    // Create toolbar
    m_toolbarWidget = new QWidget();
    m_toolbarWidget->setObjectName("RecentFileListToolbar");
    m_toolbarLayout = new QHBoxLayout(m_toolbarWidget);
    m_toolbarLayout->setContentsMargins(8, 8, 8, 8);
    m_toolbarLayout->setSpacing(8);

    // View mode toggle button
    m_viewModeButton = new ElaToolButton();
    m_viewModeButton->setObjectName("ViewModeButton");
    m_viewModeButton->setToolTip(tr("Toggle view mode"));
    m_viewModeButton->setText(m_viewMode == RecentFileViewMode::Compact ? "📋"
                                                                        : "📄");
    m_viewModeButton->setCheckable(false);
    connect(m_viewModeButton, &QToolButton::clicked, this, [this]() {
        setViewMode(m_viewMode == RecentFileViewMode::Compact
                        ? RecentFileViewMode::Detailed
                        : RecentFileViewMode::Compact);
    });

    // Sort combo box
    m_sortComboBox = new ElaComboBox();
    m_sortComboBox->setObjectName("SortComboBox");
    m_sortComboBox->addItem(
        tr("Sort by Date"),
        static_cast<int>(RecentFilesManager::SortOrder::ByDate));
    m_sortComboBox->addItem(
        tr("Sort by Name"),
        static_cast<int>(RecentFilesManager::SortOrder::ByName));
    m_sortComboBox->addItem(
        tr("Sort by Type"),
        static_cast<int>(RecentFilesManager::SortOrder::ByFileType));
    m_sortComboBox->addItem(
        tr("Sort by Size"),
        static_cast<int>(RecentFilesManager::SortOrder::BySize));
    m_sortComboBox->setCurrentIndex(0);
    connect(m_sortComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                if (index >= 0) {
                    setSortOrder(static_cast<RecentFilesManager::SortOrder>(
                        m_sortComboBox->itemData(index).toInt()));
                }
            });

    // Search/filter line edit
    m_searchLineEdit = new ElaLineEdit();
    m_searchLineEdit->setObjectName("SearchLineEdit");
    m_searchLineEdit->setPlaceholderText(tr("Search files..."));
    m_searchLineEdit->setClearButtonEnabled(true);
    connect(m_searchLineEdit, &QLineEdit::textChanged, this,
            &RecentFileListWidget::setSearchFilter);

    // Clear all button
    m_clearAllButton = new ElaToolButton();
    m_clearAllButton->setObjectName("ClearAllButton");
    m_clearAllButton->setText(tr("Clear All"));
    m_clearAllButton->setToolTip(tr("Clear all recent files"));
    connect(m_clearAllButton, &QToolButton::clicked, this, [this]() {
        if (m_recentFilesManager) {
            m_recentFilesManager->clearRecentFiles();
        }
    });

    // Add widgets to toolbar
    m_toolbarLayout->addWidget(m_viewModeButton);
    m_toolbarLayout->addWidget(m_sortComboBox);
    m_toolbarLayout->addWidget(m_searchLineEdit, 1);  // Stretch factor 1
    m_toolbarLayout->addWidget(m_clearAllButton);

    m_mainLayout->addWidget(m_toolbarWidget);

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
    m_emptyLabel = new ElaText(tr("No recent files"));
    m_emptyLabel->setObjectName("RecentFileListEmptyLabel");
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setVisible(false);

    m_contentLayout->addWidget(m_emptyLabel);
    m_contentLayout->addStretch();

    m_scrollArea->setWidget(m_contentWidget);
    m_mainLayout->addWidget(m_scrollArea);
}

void RecentFileListWidget::addFileItem(const RecentFileInfo& fileInfo) {
    RecentFileItemWidget* item =
        new RecentFileItemWidget(fileInfo, m_viewMode, this);

    connect(item, &RecentFileItemWidget::clicked, this,
            &RecentFileListWidget::onItemClicked);
    connect(item, &RecentFileItemWidget::removeRequested, this,
            &RecentFileListWidget::onItemRemoveRequested);
    connect(item, &RecentFileItemWidget::pinToggleRequested, this,
            [this](const QString& filePath) {
                if (m_recentFilesManager) {
                    m_recentFilesManager->togglePinFile(filePath);
                }
            });
    connect(item, &RecentFileItemWidget::openContainingFolderRequested, this,
            [](const QString& filePath) {
                QFileInfo fileInfo(filePath);
                QString folderPath = fileInfo.absolutePath();
                QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
            });
    connect(item, &RecentFileItemWidget::clearAllRecentRequested, this,
            [this]() {
                if (m_recentFilesManager) {
                    m_recentFilesManager->clearRecentFiles();
                }
            });

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

void RecentFileListWidget::setViewMode(RecentFileViewMode mode) {
    if (m_viewMode == mode) {
        return;
    }

    m_viewMode = mode;

    // Update view mode button icon
    if (m_viewModeButton) {
        m_viewModeButton->setText(
            m_viewMode == RecentFileViewMode::Compact ? "📋" : "📄");
        m_viewModeButton->setToolTip(m_viewMode == RecentFileViewMode::Compact
                                         ? tr("Switch to detailed view")
                                         : tr("Switch to compact view"));
    }

    // Update all existing items
    for (RecentFileItemWidget* item : m_fileItems) {
        if (item) {
            item->setViewMode(m_viewMode);
        }
    }

    // Save preference
    QSettings settings("SAST", "Readium-RecentFiles");
    settings.setValue("viewMode", static_cast<int>(m_viewMode));
}

void RecentFileListWidget::setSortOrder(RecentFilesManager::SortOrder order) {
    if (m_sortOrder == order) {
        return;
    }

    m_sortOrder = order;
    refreshList();
}

void RecentFileListWidget::setSearchFilter(const QString& filter) {
    if (m_searchFilter == filter) {
        return;
    }

    m_searchFilter = filter;
    refreshList();
}

void RecentFileListWidget::scheduleRefresh() {
    if (m_refreshTimer && !m_refreshTimer->isActive()) {
        m_refreshTimer->start();
    }
}
