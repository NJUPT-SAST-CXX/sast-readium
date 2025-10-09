#pragma once

#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT

#include <poppler-qt6.h>
#include <QFutureWatcher>
#include <QGraphicsEffect>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPixmap>
#include <QRubberBand>
#include <QTimer>
#include <QWheelEvent>

class QGraphicsPDFPageItem;
class QGraphicsPDFScene;

/**
 * Enhanced PDF page item using QGraphicsPixmapItem
 * Provides smooth scaling, rotation, and interaction
 */
class QGraphicsPDFPageItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT

public:
    explicit QGraphicsPDFPageItem(QGraphicsItem* parent = nullptr);

    void setPage(Poppler::Page* page, double scaleFactor = 1.0,
                 int rotation = 0);
    void setScaleFactor(double factor);
    void setRotation(int degrees);

    double getScaleFactor() const { return m_scaleFactor; }
    int getRotation() const { return m_rotation; }
    int getPageNumber() const { return m_pageNumber; }

    // Enhanced rendering
    void renderPageAsync();
    void renderPageSync();  // For testing purposes
    void setHighQualityRendering(bool enabled);

    // Search highlighting
    void setSearchResults(const QList<QRectF>& results);
    void clearSearchHighlights();
    void setCurrentSearchResult(int index);

    // Geometry
    QRectF boundingRect() const override;

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

private slots:
    void onRenderCompleted();

private:
    void renderPage();
    void drawSearchHighlights(QPainter* painter);

    Poppler::Page* m_page;
    double m_scaleFactor;
    int m_rotation;
    int m_pageNumber;
    bool m_highQualityEnabled;
    bool m_isRendering;

    QFutureWatcher<QPixmap>* m_renderWatcher;
    QTimer* m_renderTimer;

    // Search highlighting
    QList<QRectF> m_searchResults;
    int m_currentSearchResultIndex;
    QColor m_normalHighlightColor;
    QColor m_currentHighlightColor;
};

/**
 * Custom QGraphicsScene for PDF pages
 * Manages multiple pages and provides enhanced navigation
 */
class QGraphicsPDFScene : public QGraphicsScene {
    Q_OBJECT

public:
    explicit QGraphicsPDFScene(QObject* parent = nullptr);

    void setDocument(Poppler::Document* document);
    void clearDocument();

    void addPage(int pageNumber);
    void removePage(int pageNumber);
    void removeAllPages();

    QGraphicsPDFPageItem* getPageItem(int pageNumber) const;
    int getPageCount() const;

    // Layout management
    void setPageSpacing(int spacing);
    void setPageMargin(int margin);
    void updateLayout();

    // Rendering control
    void setHighQualityRendering(bool enabled);
    void setScaleFactor(double factor);
    void setRotation(int degrees);

signals:
    void pageClicked(int pageNumber, QPointF position);
    void scaleChanged(double scale);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

private:
    void layoutPages();

    Poppler::Document* m_document;
    QMap<int, QGraphicsPDFPageItem*> m_pageItems;

    int m_pageSpacing;
    int m_pageMargin;
    double m_scaleFactor;
    int m_rotation;
    bool m_highQualityEnabled;
};

/**
 * Enhanced PDF viewer using QGraphicsView
 * Provides smooth zooming, panning, and advanced interactions
 */
class QGraphicsPDFViewer : public QGraphicsView {
    Q_OBJECT

public:
    explicit QGraphicsPDFViewer(QWidget* parent = nullptr);
    ~QGraphicsPDFViewer();

    // Document management
    void setDocument(Poppler::Document* document);
    void clearDocument();

    // Navigation
    void goToPage(int pageNumber);
    void nextPage();
    void previousPage();
    void firstPage();
    void lastPage();

    // Zoom operations
    void zoomIn();
    void zoomOut();
    void zoomToFit();
    void zoomToWidth();
    void zoomToHeight();
    void setZoom(double factor);
    void resetZoom();

    // Rotation
    void rotateLeft();
    void rotateRight();
    void resetRotation();
    void setRotation(int degrees);

    // View modes
    enum ViewMode { SinglePage, ContinuousPage, FacingPages, ContinuousFacing };

    void setViewMode(ViewMode mode);
    ViewMode getViewMode() const { return m_viewMode; }

    // Enhanced features
    void setHighQualityRendering(bool enabled);
    void setPageSpacing(int spacing);
    void setPageMargin(int margin);
    void setSmoothScrolling(bool enabled);

    // Search functionality
    void setSearchResults(const QList<QPair<int, QList<QRectF>>>& results);
    void clearSearchHighlights();
    void goToSearchResult(int index);

    // Getters
    int getCurrentPage() const { return m_currentPage; }
    double getZoomFactor() const { return m_zoomFactor; }
    int getRotation() const { return m_rotation; }
    int getPageCount() const;
    bool hasDocument() const { return m_document != nullptr; }

signals:
    void documentChanged(bool hasDocument);
    void currentPageChanged(int pageNumber);
    void zoomChanged(double factor);
    void rotationChanged(int degrees);
    void pageClicked(int pageNumber, QPointF position);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onScenePageClicked(int pageNumber, QPointF position);
    void onSceneScaleChanged(double scale);
    void updateCurrentPage();

private:
    void setupView();
    void updateViewTransform();
    void updateVisiblePages();
    void centerOnPage(int pageNumber);
    void fitToView();
    void fitToWidth();
    void fitToHeight();

    QGraphicsPDFScene* m_scene;
    Poppler::Document* m_document;

    ViewMode m_viewMode;
    int m_currentPage;
    double m_zoomFactor;
    int m_rotation;

    // View settings
    bool m_highQualityEnabled;
    bool m_smoothScrollingEnabled;
    int m_pageSpacing;
    int m_pageMargin;

    // Interaction state
    bool m_isPanning;
    QPoint m_lastPanPoint;
    QRubberBand* m_rubberBand;
    QPoint m_rubberBandOrigin;

    // Performance optimization
    QTimer* m_updateTimer;
    QTimer* m_renderTimer;
};

#endif  // ENABLE_QGRAPHICS_PDF_SUPPORT
