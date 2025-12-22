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
    m_formFieldStorage.clear();
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

    m_formFieldStorage = m_currentPage->formFields();
    m_formFields.clear();
    for (const auto& fieldPtr : m_formFieldStorage) {
        m_formFields.append(fieldPtr.get());
    }
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

void FormFieldManager::createFieldWidget(Poppler::FormField* field) {
    if (!field)
        return;

    QWidget* widget = nullptr;
    switch (field->type()) {
        case Poppler::FormField::FormText:
            widget = createTextFieldWidget(
                static_cast<Poppler::FormFieldText*>(field));
            break;
        case Poppler::FormField::FormChoice:
            widget = createChoiceFieldWidget(
                static_cast<Poppler::FormFieldChoice*>(field));
            break;
        case Poppler::FormField::FormButton:
            widget = createButtonFieldWidget(
                static_cast<Poppler::FormFieldButton*>(field));
            break;
        default:
            break;
    }

    if (widget) {
        m_fieldWidgets[field] = widget;
    }
}

QWidget* FormFieldManager::createTextFieldWidget(
    Poppler::FormFieldText* field) {
    if (!field)
        return nullptr;
    auto* lineEdit = new QLineEdit();
    lineEdit->setText(field->text());
    return lineEdit;
}

QWidget* FormFieldManager::createChoiceFieldWidget(
    Poppler::FormFieldChoice* field) {
    if (!field)
        return nullptr;
    auto* comboBox = new QComboBox();
    for (int i = 0; i < field->choices().size(); ++i) {
        comboBox->addItem(field->choices().at(i));
    }
    return comboBox;
}

QWidget* FormFieldManager::createButtonFieldWidget(
    Poppler::FormFieldButton* field) {
    if (!field)
        return nullptr;

    if (field->buttonType() == Poppler::FormFieldButton::CheckBox) {
        auto* checkBox = new QCheckBox();
        checkBox->setChecked(field->state());
        return checkBox;
    } else if (field->buttonType() == Poppler::FormFieldButton::Radio) {
        auto* radioButton = new QRadioButton();
        radioButton->setChecked(field->state());
        return radioButton;
    }
    return nullptr;
}

QVariant FormFieldManager::getFieldValue(Poppler::FormField* field) const {
    if (!field)
        return QVariant();

    switch (field->type()) {
        case Poppler::FormField::FormText: {
            auto* textField = static_cast<Poppler::FormFieldText*>(field);
            return textField->text();
        }
        case Poppler::FormField::FormChoice: {
            auto* choiceField = static_cast<Poppler::FormFieldChoice*>(field);
            return QVariant::fromValue(choiceField->currentChoices());
        }
        case Poppler::FormField::FormButton: {
            auto* buttonField = static_cast<Poppler::FormFieldButton*>(field);
            return buttonField->state();
        }
        default:
            return QVariant();
    }
}

void FormFieldManager::renderFormFields(QPainter& painter, double scaleFactor) {
    for (Poppler::FormField* field : m_formFields) {
        if (!field)
            continue;

        switch (field->type()) {
            case Poppler::FormField::FormText:
                renderTextField(painter,
                                static_cast<Poppler::FormFieldText*>(field),
                                scaleFactor);
                break;
            case Poppler::FormField::FormChoice:
                renderChoiceField(painter,
                                  static_cast<Poppler::FormFieldChoice*>(field),
                                  scaleFactor);
                break;
            case Poppler::FormField::FormButton:
                renderButtonField(painter,
                                  static_cast<Poppler::FormFieldButton*>(field),
                                  scaleFactor);
                break;
            default:
                break;
        }
    }
}

void FormFieldManager::renderTextField(QPainter& painter,
                                       Poppler::FormFieldText* field,
                                       double scale) {
    if (!field)
        return;
    QRectF rect = field->rect();
    rect = QRectF(rect.x() * scale, rect.y() * scale, rect.width() * scale,
                  rect.height() * scale);
    painter.setPen(QPen(Qt::black));
    painter.setBrush(QBrush(Qt::white));
    painter.drawRect(rect);
    painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, field->text());
}

void FormFieldManager::renderChoiceField(QPainter& painter,
                                         Poppler::FormFieldChoice* field,
                                         double scale) {
    if (!field)
        return;
    QRectF rect = field->rect();
    rect = QRectF(rect.x() * scale, rect.y() * scale, rect.width() * scale,
                  rect.height() * scale);
    painter.setPen(QPen(Qt::black));
    painter.setBrush(QBrush(Qt::white));
    painter.drawRect(rect);
}

void FormFieldManager::renderButtonField(QPainter& painter,
                                         Poppler::FormFieldButton* field,
                                         double scale) {
    if (!field)
        return;
    QRectF rect = field->rect();
    rect = QRectF(rect.x() * scale, rect.y() * scale, rect.width() * scale,
                  rect.height() * scale);
    painter.setPen(QPen(Qt::black));
    if (field->state()) {
        painter.setBrush(QBrush(Qt::darkGray));
    } else {
        painter.setBrush(QBrush(Qt::white));
    }
    painter.drawRect(rect);
}

void FormFieldManager::setFieldValue(Poppler::FormField* field,
                                     const QVariant& value) {
    if (!field)
        return;

    switch (field->type()) {
        case Poppler::FormField::FormText: {
            auto* textField = static_cast<Poppler::FormFieldText*>(field);
            textField->setText(value.toString());
            break;
        }
        case Poppler::FormField::FormButton: {
            auto* buttonField = static_cast<Poppler::FormFieldButton*>(field);
            buttonField->setState(value.toBool());
            break;
        }
        default:
            break;
    }

    m_fieldValues[field] = value;
    emit formFieldValueChanged(field, value);
    emit formDataChanged();
}

bool FormFieldManager::saveFormData() {
    // TODO: Implement form data persistence
    return true;
}

bool FormFieldManager::loadFormData() {
    // TODO: Implement form data loading
    return true;
}

void FormFieldManager::setFocusedField(Poppler::FormField* field) {
    if (m_focusedField != field) {
        m_focusedField = field;
        emit focusChanged(field);
    }
}

void FormFieldManager::clearFocus() {
    if (m_focusedField) {
        m_focusedField = nullptr;
        emit focusChanged(nullptr);
    }
}

void FormFieldManager::handleTabNavigation(bool forward) {
    if (m_formFields.isEmpty())
        return;

    int currentIndex = m_formFields.indexOf(m_focusedField);
    int nextIndex;

    if (forward) {
        nextIndex = (currentIndex + 1) % m_formFields.size();
    } else {
        nextIndex =
            (currentIndex - 1 + m_formFields.size()) % m_formFields.size();
    }

    setFocusedField(m_formFields.at(nextIndex));
}
