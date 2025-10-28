#include "FormFieldManager.h"
#include <QBrush>
#include <QFont>
#include <QPen>

FormFieldManager::FormFieldManager(QObject* parent)
    : QObject(parent),
      m_currentPage(nullptr),
      m_pageNumber(-1),
      m_focusedField(nullptr),
      m_formFieldsExtracted(false) {}

FormFieldManager::~FormFieldManager() {
    clearPage();
    qDeleteAll(m_fieldWidgets);
}

void FormFieldManager::setPage(Poppler::Page* page, int pageNumber) {
    if (m_currentPage == page && m_pageNumber == pageNumber)
        return;
    clearPage();
    m_currentPage = page;
    m_pageNumber = pageNumber;
    m_formFieldsExtracted = false;
    extractFormFields();
}

void FormFieldManager::clearPage() {
    m_formFields.clear();
    qDeleteAll(m_fieldWidgets);
    m_fieldWidgets.clear();
    m_fieldValues.clear();
    m_focusedField = nullptr;
    m_currentPage = nullptr;
    m_pageNumber = -1;
    m_formFieldsExtracted = false;
}

void FormFieldManager::extractFormFields() {
    if (!m_currentPage || m_formFieldsExtracted)
        return;

    m_formFields = m_currentPage->formFields();
    m_formFieldsExtracted = true;

    for (Poppler::FormField* field : m_formFields) {
        if (field) {
            createFieldWidget(field);
            QVariant value = getFieldValue(field);
            if (value.isValid()) {
                m_fieldValues[field] = value;
            }
        }
    }
}

Poppler::FormField* FormFieldManager::getFieldAtPoint(
    const QPointF& point) const {
    for (Poppler::FormField* field : m_formFields) {
        if (field && isPointInField(point, field)) {
            return field;
        }
    }
    return nullptr;
}

bool FormFieldManager::isPointInField(const QPointF& point,
                                      Poppler::FormField* field) const {
    if (!field)
        return false;
    QRectF rect = field->rect();
    return rect.contains(point);
}
