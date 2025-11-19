#pragma once

#include <poppler/qt6/poppler-form.h>
#include <poppler/qt6/poppler-qt6.h>
#include <QCheckBox>
#include <QComboBox>
#include <QHash>
#include <QLineEdit>
#include <QList>
#include <QObject>
#include <QPainter>
#include <QRadioButton>
#include <QString>
#include <QVariant>
#include <QWidget>
#include <vector>

class FormFieldManager : public QObject {
    Q_OBJECT

public:
    explicit FormFieldManager(QObject* parent = nullptr);
    ~FormFieldManager();

    void setPage(Poppler::Page* page, int pageNumber);
    void clearPage();
    bool hasFormFields() const { return !m_formFields.isEmpty(); }

    QList<Poppler::FormField*> getFormFields() const { return m_formFields; }
    Poppler::FormField* getFieldAtPoint(const QPointF& point) const;

    void renderFormFields(QPainter& painter, double scaleFactor);
    void setFieldValue(Poppler::FormField* field, const QVariant& value);
    QVariant getFieldValue(Poppler::FormField* field) const;

    bool saveFormData();
    bool loadFormData();

    void setFocusedField(Poppler::FormField* field);
    Poppler::FormField* getFocusedField() const { return m_focusedField; }
    void clearFocus();

    void handleTabNavigation(bool forward);
    int getFieldCount() const { return m_formFields.size(); }

signals:
    void formFieldClicked(Poppler::FormField* field);
    void formFieldValueChanged(Poppler::FormField* field,
                               const QVariant& value);
    void formDataChanged();
    void focusChanged(Poppler::FormField* field);

private:
    void extractFormFields();
    void createFieldWidget(Poppler::FormField* field);
    QWidget* createTextFieldWidget(Poppler::FormFieldText* field);
    QWidget* createChoiceFieldWidget(Poppler::FormFieldChoice* field);
    QWidget* createButtonFieldWidget(Poppler::FormFieldButton* field);
    void renderTextField(QPainter& painter, Poppler::FormFieldText* field,
                         double scale);
    void renderChoiceField(QPainter& painter, Poppler::FormFieldChoice* field,
                           double scale);
    void renderButtonField(QPainter& painter, Poppler::FormFieldButton* field,
                           double scale);
    bool isPointInField(const QPointF& point, Poppler::FormField* field) const;

    Poppler::Page* m_currentPage;
    int m_pageNumber;
    std::vector<std::unique_ptr<Poppler::FormField>> m_formFieldStorage;
    QList<Poppler::FormField*> m_formFields;
    QHash<Poppler::FormField*, QWidget*> m_fieldWidgets;
    QHash<Poppler::FormField*, QVariant> m_fieldValues;
    Poppler::FormField* m_focusedField;
    bool m_formFieldsExtracted;
};
