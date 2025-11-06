#ifndef LAYERSPANEL_H
#define LAYERSPANEL_H

#include <poppler/qt6/poppler-qt6.h>
#include <QWidget>

// Forward declarations
class ElaText;
class ElaTreeView;
class QVBoxLayout;
class QStandardItemModel;
class QStandardItem;

/**
 * @brief LayersPanel - PDF layers (Optional Content Groups) display and control
 * panel
 *
 * Displays and manages PDF layers:
 * - Tree view of all optional content groups (OCGs)
 * - Checkboxes to toggle layer visibility
 * - Hierarchical display of layer groups
 * - Handles documents without layers gracefully
 * - Integrates with Poppler's optional content functionality
 */
class LayersPanel : public QWidget {
    Q_OBJECT

public:
    explicit LayersPanel(QWidget* parent = nullptr);
    ~LayersPanel() override;

    // Document management
    void setDocument(Poppler::Document* document);
    void clearDocument();
    bool hasDocument() const;

    // Layer count
    int getLayerCount() const;

signals:
    void documentChanged();
    void layerVisibilityChanged(const QString& layerName, bool visible);

private slots:
    void onLayerItemChanged(QStandardItem* item);

private:
    // UI Components
    QVBoxLayout* m_mainLayout;
    ElaText* m_titleLabel;
    ElaText* m_countLabel;
    ElaTreeView* m_layersTree;
    ElaText* m_emptyLabel;
    QStandardItemModel* m_layersModel;

    // Data
    Poppler::Document* m_document;
    Poppler::OptContentModel* m_optContentModel;
    int m_layerCount;

    // UI setup
    void setupUi();
    void updateLayersTree();
    void showEmptyState();
    void showLayersTree();

    // Helper methods
    void populateLayersFromOptContent();
    void addLayerItem(QStandardItem* parent, const QString& layerName,
                      bool isVisible);
};

#endif  // LAYERSPANEL_H
