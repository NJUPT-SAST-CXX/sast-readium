#include "ThumbnailPanel.h"

// ElaWidgetTools
#include "ElaListView.h"

// Qt
#include <QSize>
#include <QVBoxLayout>

// Logging
#include "logging/SimpleLogging.h"

// Business logic
#include "delegate/ThumbnailDelegate.h"
#include "model/ThumbnailModel.h"

ThumbnailPanel::ThumbnailPanel(QWidget* parent)
    : QWidget(parent),
      m_listView(nullptr),
      m_model(nullptr),
      m_delegate(nullptr),
      m_document(nullptr),
      m_currentPage(1) {
    SLOG_INFO("ThumbnailPanel: Constructor started");

    setupUi();
    connectSignals();

    SLOG_INFO("ThumbnailPanel: Constructor completed");
}

ThumbnailPanel::~ThumbnailPanel() {
    SLOG_INFO("ThumbnailPanel: Destructor called");
}

void ThumbnailPanel::setupUi() {
    // Set transparent background to match Ela design
    setAutoFillBackground(false);
    setAttribute(Qt::WA_TranslucentBackground, false);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(5);

    // 缩略图列表视图
    m_listView = new ElaListView(this);
    m_listView->setViewMode(QListView::IconMode);
    m_listView->setFlow(QListView::TopToBottom);
    m_listView->setResizeMode(QListView::Adjust);
    m_listView->setSpacing(10);
    m_listView->setUniformItemSizes(true);

    // Set proper background for the list view
    m_listView->setAutoFillBackground(false);

    layout->addWidget(m_listView);
}

void ThumbnailPanel::connectSignals() {
    connect(m_listView, &ElaListView::clicked, this,
            [this](const QModelIndex& index) {
                if (index.isValid()) {
                    int pageNumber = index.row() + 1;
                    emit pageSelected(pageNumber);
                }
            });
}

void ThumbnailPanel::setDocument(std::shared_ptr<Poppler::Document> document) {
    SLOG_INFO("ThumbnailPanel: Setting document");

    m_document = document;

    if (!m_model) {
        m_model = new ThumbnailModel(this);
    }
    m_model->setDocument(document);

    if (!m_delegate) {
        m_delegate = new ThumbnailDelegate(this);
    }

    m_listView->setModel(m_model);
    m_listView->setItemDelegate(m_delegate);
}

void ThumbnailPanel::clearDocument() {
    SLOG_INFO("ThumbnailPanel: Clearing document");

    m_document.reset();

    if (m_model) {
        m_model->clearCache();
    }
}

void ThumbnailPanel::setCurrentPage(int pageNumber) {
    if (!m_model) {
        return;
    }

    m_currentPage = pageNumber;

    int index = pageNumber - 1;
    if (index >= 0 && index < m_model->rowCount()) {
        m_listView->setCurrentIndex(m_model->index(index, 0));
        m_listView->scrollTo(m_model->index(index, 0));
    }
}

void ThumbnailPanel::setThumbnailSize(int size) {
    if (m_model) {
        m_model->setThumbnailSize(QSize(size, size));
    }
}

void ThumbnailPanel::refresh() {
    if (m_model) {
        m_model->refreshAllThumbnails();
    }
}

void ThumbnailPanel::setThumbnailModel(ThumbnailModel* model) {
    m_model = model;
    if (m_listView) {
        m_listView->setModel(m_model);
    }
}

void ThumbnailPanel::setThumbnailDelegate(ThumbnailDelegate* delegate) {
    m_delegate = delegate;
    if (m_listView) {
        m_listView->setItemDelegate(m_delegate);
    }
}
