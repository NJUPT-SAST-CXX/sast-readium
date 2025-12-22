#include "AnnotationIntegrationHelper.h"

AnnotationIntegrationHelper::AnnotationIntegrationHelper(QWidget* parent)
    : QObject(parent), m_parentWidget(parent), m_hasDocument(false) {}

bool AnnotationIntegrationHelper::initialize() { return true; }

bool AnnotationIntegrationHelper::hasDocument() const { return m_hasDocument; }

void AnnotationIntegrationHelper::clearDocument() { m_hasDocument = false; }

bool AnnotationIntegrationHelper::handleMousePress(const QPointF& /*point*/,
                                                   int /*page*/,
                                                   double /*scale*/) {
    return false;
}

bool AnnotationIntegrationHelper::handleMouseMove(const QPointF& /*point*/,
                                                  double /*scale*/) {
    return false;
}

bool AnnotationIntegrationHelper::handleMouseRelease(const QPointF& /*point*/,
                                                     double /*scale*/) {
    return false;
}
