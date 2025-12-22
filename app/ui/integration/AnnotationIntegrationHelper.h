#ifndef ANNOTATIONINTEGRATIONHELPER_H
#define ANNOTATIONINTEGRATIONHELPER_H

#include <QObject>
#include <QPointF>
#include <QWidget>

/**
 * @brief Helper class for annotation integration
 *
 * Provides integration between annotation system and PDF viewer.
 */
class AnnotationIntegrationHelper : public QObject {
    Q_OBJECT

public:
    explicit AnnotationIntegrationHelper(QWidget* parent = nullptr);
    ~AnnotationIntegrationHelper() override = default;

    bool initialize();
    bool hasDocument() const;
    void clearDocument();

    bool handleMousePress(const QPointF& point, int page, double scale);
    bool handleMouseMove(const QPointF& point, double scale);
    bool handleMouseRelease(const QPointF& point, double scale);

signals:
    void annotationSelected(int annotationId);
    void selectionCleared();
    void annotationsChanged();

private:
    QWidget* m_parentWidget;
    bool m_hasDocument;
};

#endif  // ANNOTATIONINTEGRATIONHELPER_H
