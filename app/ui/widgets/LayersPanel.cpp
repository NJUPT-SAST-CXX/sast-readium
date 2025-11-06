#include "LayersPanel.h"

// ElaWidgetTools
#include "ElaText.h"
#include "ElaTreeView.h"

// Qt
#include <QHBoxLayout>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QVBoxLayout>

// Logging
#include "logging/SimpleLogging.h"

// ============================================================================
// Constructor and Destructor
// ============================================================================

LayersPanel::LayersPanel(QWidget* parent)
    : QWidget(parent),
      m_document(nullptr),
      m_optContentModel(nullptr),
      m_layerCount(0) {
    SLOG_INFO("LayersPanel: Constructor started");

    // Create layers model
    m_layersModel = new QStandardItemModel(this);
    m_layersModel->setHorizontalHeaderLabels(QStringList() << tr("Layer Name"));

    setupUi();
    showEmptyState();

    SLOG_INFO("LayersPanel: Constructor completed");
}

LayersPanel::~LayersPanel() { SLOG_INFO("LayersPanel: Destructor called"); }

// ============================================================================
// UI Setup
// ============================================================================

void LayersPanel::setupUi() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(15, 15, 15, 15);
    m_mainLayout->setSpacing(10);

    // Title and count
    QHBoxLayout* headerLayout = new QHBoxLayout();

    m_titleLabel = new ElaText(tr("Layers"), this);
    m_titleLabel->setTextPixelSize(16);
    headerLayout->addWidget(m_titleLabel);

    m_countLabel = new ElaText(tr("(0)"), this);
    m_countLabel->setStyleSheet("ElaText { color: #666666; }");
    headerLayout->addWidget(m_countLabel);

    headerLayout->addStretch();

    m_mainLayout->addLayout(headerLayout);

    // Layers tree view
    m_layersTree = new ElaTreeView(this);
    m_layersTree->setModel(m_layersModel);
    m_layersTree->setHeaderHidden(false);
    m_layersTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(m_layersModel, &QStandardItemModel::itemChanged, this,
            &LayersPanel::onLayerItemChanged);
    m_mainLayout->addWidget(m_layersTree);

    // Empty state label
    m_emptyLabel = new ElaText(tr("This document has no layers"), this);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setStyleSheet("ElaText { color: #999999; padding: 20px; }");
    m_mainLayout->addWidget(m_emptyLabel);
}

// ============================================================================
// Document Management
// ============================================================================

void LayersPanel::setDocument(Poppler::Document* document) {
    if (!document) {
        SLOG_WARNING("LayersPanel::setDocument: Null document provided");
        clearDocument();
        return;
    }

    SLOG_INFO("LayersPanel: Setting document");

    m_document = document;

    // Get optional content model from document
    m_optContentModel = document->optionalContentModel();

    // Update UI
    updateLayersTree();

    emit documentChanged();
}

void LayersPanel::clearDocument() {
    SLOG_INFO("LayersPanel: Clearing document");

    m_document = nullptr;
    m_optContentModel = nullptr;
    m_layerCount = 0;

    // Clear layers model
    m_layersModel->clear();
    m_layersModel->setHorizontalHeaderLabels(QStringList() << tr("Layer Name"));

    // Show empty state
    showEmptyState();
}

bool LayersPanel::hasDocument() const { return m_document != nullptr; }

int LayersPanel::getLayerCount() const { return m_layerCount; }

// ============================================================================
// UI Update Methods
// ============================================================================

void LayersPanel::updateLayersTree() {
    if (!m_document) {
        showEmptyState();
        return;
    }

    // Clear existing items
    m_layersModel->clear();
    m_layersModel->setHorizontalHeaderLabels(QStringList() << tr("Layer Name"));
    m_layerCount = 0;

    if (m_optContentModel) {
        // Populate from optional content model
        populateLayersFromOptContent();
    }

    SLOG_INFO("LayersPanel: Found " + QString::number(m_layerCount) +
              " layers");

    // Update count label
    m_countLabel->setText(QString("(%1)").arg(m_layerCount));

    if (m_layerCount == 0) {
        showEmptyState();
    } else {
        showLayersTree();
    }
}

void LayersPanel::showEmptyState() {
    m_layersTree->setVisible(false);
    m_emptyLabel->setVisible(true);
    m_countLabel->setText(tr("(0)"));
}

void LayersPanel::showLayersTree() {
    m_layersTree->setVisible(true);
    m_emptyLabel->setVisible(false);
    m_layersTree->expandAll();
}

// ============================================================================
// Slots
// ============================================================================

void LayersPanel::onLayerItemChanged(QStandardItem* item) {
    if (!item) {
        return;
    }

    // Get layer name and visibility state
    QString layerName = item->text();
    bool isVisible = (item->checkState() == Qt::Checked);

    SLOG_INFO("LayersPanel: Layer '" + layerName + "' visibility changed to " +
              QString(isVisible ? "visible" : "hidden"));

    emit layerVisibilityChanged(layerName, isVisible);

    // Note: Actual layer visibility toggling would require integration with
    // Poppler's optional content API, which may vary by version.
    // For now, we emit the signal for potential handling by the viewer.
}

// ============================================================================
// Helper Methods
// ============================================================================

void LayersPanel::populateLayersFromOptContent() {
    if (!m_optContentModel) {
        SLOG_WARNING("LayersPanel: No optional content model available");
        return;
    }

    try {
        // Get the root index of the optional content model (use invalid
        // QModelIndex as parent for root)
        QModelIndex rootIndex = QModelIndex();

        // Iterate through the optional content model
        int rowCount = m_optContentModel->rowCount(rootIndex);

        for (int i = 0; i < rowCount; ++i) {
            QModelIndex index = m_optContentModel->index(i, 0, rootIndex);

            if (!index.isValid()) {
                continue;
            }

            // Get layer name
            QString layerName =
                m_optContentModel->data(index, Qt::DisplayRole).toString();

            // Get visibility state (Qt::CheckStateRole)
            QVariant checkStateVariant =
                m_optContentModel->data(index, Qt::CheckStateRole);
            bool isVisible = (checkStateVariant.toInt() == Qt::Checked);

            // Add layer item
            addLayerItem(m_layersModel->invisibleRootItem(), layerName,
                         isVisible);
            m_layerCount++;
        }

        SLOG_INFO("LayersPanel: Populated " + QString::number(m_layerCount) +
                  " layers from optional content model");

    } catch (const std::exception& e) {
        SLOG_ERROR("LayersPanel: Error populating layers: " +
                   QString(e.what()));
    }
}

void LayersPanel::addLayerItem(QStandardItem* parent, const QString& layerName,
                               bool isVisible) {
    QStandardItem* item = new QStandardItem(layerName);
    item->setCheckable(true);
    item->setCheckState(isVisible ? Qt::Checked : Qt::Unchecked);
    item->setEditable(false);

    parent->appendRow(item);
}
