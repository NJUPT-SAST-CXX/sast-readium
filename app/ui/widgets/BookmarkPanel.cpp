#include "BookmarkPanel.h"

// Business Logic
#include "model/BookmarkModel.h"

// ElaWidgetTools
#include "ElaIcon.h"
#include "ElaListView.h"
#include "ElaPushButton.h"
#include "ElaToolButton.h"

// Qt
#include <QDateTime>
#include <QEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QUuid>
#include <QVBoxLayout>

// Logging
#include "logging/SimpleLogging.h"

// ============================================================================
// 构造和析构
// ============================================================================

BookmarkPanel::BookmarkPanel(QWidget* parent)
    : QWidget(parent),
      m_listView(nullptr),
      m_addBtn(nullptr),
      m_removeBtn(nullptr),
      m_clearBtn(nullptr),
      m_exportBtn(nullptr),
      m_importBtn(nullptr),
      m_model(nullptr),
      m_document(nullptr) {
    SLOG_INFO("BookmarkPanel: Constructor started");

    setupUi();
    connectSignals();

    SLOG_INFO("BookmarkPanel: Constructor completed");
}

BookmarkPanel::~BookmarkPanel() {
    SLOG_INFO("BookmarkPanel: Destructor called");
}

// ============================================================================
// UI 初始化
// ============================================================================

void BookmarkPanel::setupUi() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(5);

    // 工具栏
    QHBoxLayout* toolbarLayout = new QHBoxLayout();

    m_addBtn = new ElaToolButton(this);
    m_addBtn->setIcon(ElaIcon::getInstance()->getElaIcon(ElaIconType::Plus));
    m_addBtn->setToolTip(tr("Add bookmark"));
    toolbarLayout->addWidget(m_addBtn);

    m_removeBtn = new ElaToolButton(this);
    m_removeBtn->setIcon(
        ElaIcon::getInstance()->getElaIcon(ElaIconType::Minus));
    m_removeBtn->setToolTip(tr("Remove bookmark"));
    m_removeBtn->setEnabled(false);
    toolbarLayout->addWidget(m_removeBtn);

    m_clearBtn = new ElaToolButton(this);
    m_clearBtn->setIcon(ElaIcon::getInstance()->getElaIcon(ElaIconType::Trash));
    m_clearBtn->setToolTip(tr("Clear all bookmarks"));
    toolbarLayout->addWidget(m_clearBtn);

    toolbarLayout->addStretch();

    m_exportBtn = new ElaToolButton(this);
    m_exportBtn->setIcon(
        ElaIcon::getInstance()->getElaIcon(ElaIconType::FloppyDisk));
    m_exportBtn->setToolTip(tr("Export bookmarks"));
    toolbarLayout->addWidget(m_exportBtn);

    m_importBtn = new ElaToolButton(this);
    m_importBtn->setIcon(
        ElaIcon::getInstance()->getElaIcon(ElaIconType::FolderOpen));
    m_importBtn->setToolTip(tr("Import bookmarks"));
    toolbarLayout->addWidget(m_importBtn);

    layout->addLayout(toolbarLayout);

    // 书签列表
    m_listView = new ElaListView(this);
    layout->addWidget(m_listView);
}

void BookmarkPanel::connectSignals() {
    // Create and set the model first to ensure selectionModel() is available
    // Note: BookmarkModel now loads bookmarks asynchronously to prevent UI
    // freeze
    m_model = new BookmarkModel(this);
    m_listView->setModel(m_model);

    // 添加书签
    connect(m_addBtn, &ElaToolButton::clicked, this, []() {
        // 这个需要从外部触发，因为需要当前页码
        // 暂时不实现，等待外部调用 addBookmark
    });

    // 删除书签
    connect(m_removeBtn, &ElaToolButton::clicked, this, [this]() {
        QModelIndex index = m_listView->currentIndex();
        if (index.isValid() && m_model) {
            int pageNumber = m_model->data(index, Qt::UserRole).toInt();
            removeBookmark(pageNumber);
        }
    });

    // 清除所有书签
    connect(m_clearBtn, &ElaToolButton::clicked, this, [this]() {
        if (m_model) {
            QMessageBox::StandardButton reply = QMessageBox::question(
                this, tr("Clear Bookmarks"),
                tr("Are you sure you want to clear all bookmarks?"),
                QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::Yes) {
                m_model->clearAllBookmarks();
            }
        }
    });

    // 导出书签
    connect(m_exportBtn, &ElaToolButton::clicked, this, [this]() {
        QString filePath = QFileDialog::getSaveFileName(
            this, tr("Export Bookmarks"), QString(), tr("JSON Files (*.json)"));

        if (!filePath.isEmpty() && m_model) {
            m_model->exportBookmarks(filePath);
        }
    });

    // 导入书签
    connect(m_importBtn, &ElaToolButton::clicked, this, [this]() {
        QString filePath = QFileDialog::getOpenFileName(
            this, tr("Import Bookmarks"), QString(), tr("JSON Files (*.json)"));

        if (!filePath.isEmpty() && m_model) {
            m_model->importBookmarks(filePath);
        }
    });

    // 书签选择
    connect(m_listView, &ElaListView::clicked, this,
            [this](const QModelIndex& index) {
                if (index.isValid() && m_model) {
                    int pageNumber = m_model->data(index, Qt::UserRole).toInt();
                    emit bookmarkSelected(pageNumber);
                }
            });

    // 书签列表选择变化
    connect(m_listView->selectionModel(),
            &QItemSelectionModel::selectionChanged, this, [this]() {
                m_removeBtn->setEnabled(m_listView->currentIndex().isValid());
            });
}

// ============================================================================
// 文档管理
// ============================================================================

void BookmarkPanel::setDocument(std::shared_ptr<Poppler::Document> document) {
    SLOG_INFO("BookmarkPanel: Setting document");

    m_document = document;

    // Model is already created in connectSignals()
    // BookmarkModel 不需要设置文档，它管理所有文档的书签
}

void BookmarkPanel::clearDocument() {
    SLOG_INFO("BookmarkPanel: Clearing document");

    m_document.reset();

    // 不清除书签模型，因为它管理所有文档的书签
}

// ============================================================================
// 书签管理
// ============================================================================

void BookmarkPanel::addBookmark(int pageNumber, const QString& title) {
    if (!m_model || !m_document) {
        return;
    }

    // 创建书签对象
    Bookmark bookmark;
    bookmark.id = QUuid::createUuid().toString();
    bookmark.title = title.isEmpty() ? tr("Page %1").arg(pageNumber) : title;
    bookmark.documentPath = "";  // 需要从外部设置
    bookmark.pageNumber = pageNumber;
    bookmark.createdTime = QDateTime::currentDateTime();
    bookmark.lastAccessed = QDateTime::currentDateTime();

    m_model->addBookmark(bookmark);
    emit bookmarkAdded(pageNumber, bookmark.title);
}

void BookmarkPanel::removeBookmark(int pageNumber) {
    if (!m_model) {
        return;
    }

    // 需要找到对应的书签 ID
    // 这里简化处理，实际应该根据文档路径和页码查找
    emit bookmarkRemoved(pageNumber);
}

void BookmarkPanel::clearBookmarks() {
    if (m_model) {
        m_model->clearAllBookmarks();
    }
}

bool BookmarkPanel::exportBookmarks(const QString& filePath) {
    if (m_model) {
        return m_model->exportBookmarks(filePath);
    }
    return false;
}

bool BookmarkPanel::importBookmarks(const QString& filePath) {
    if (m_model) {
        return m_model->importBookmarks(filePath);
    }
    return false;
}

void BookmarkPanel::setBookmarkModel(BookmarkModel* model) {
    m_model = model;
    if (m_listView) {
        m_listView->setModel(m_model);
    }
}

// ============================================================================
// 事件处理
// ============================================================================

void BookmarkPanel::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}

void BookmarkPanel::retranslateUi() {
    SLOG_INFO("BookmarkPanel: Retranslating UI");

    m_addBtn->setToolTip(tr("Add bookmark"));
    m_removeBtn->setToolTip(tr("Remove bookmark"));
    m_clearBtn->setToolTip(tr("Clear all bookmarks"));
    m_exportBtn->setToolTip(tr("Export bookmarks"));
    m_importBtn->setToolTip(tr("Import bookmarks"));
}
