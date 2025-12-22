#pragma once

#include <QHash>
#include <QPointer>
#include <QWidget>

#include "model/PluginConfigModel.h"

// Forward declarations
class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QScrollArea;
class QLabel;
class ElaText;
class ElaLineEdit;
class ElaSpinBox;
class ElaDoubleSpinBox;
class ElaToggleSwitch;
class ElaComboBox;
class ElaSlider;
class ElaPushButton;
class ElaScrollPageArea;

/**
 * @brief PluginConfigWidget - Dynamic plugin configuration editor
 *
 * This widget generates appropriate editing controls based on the
 * configuration schema. It supports:
 * - Boolean toggles (ElaToggleSwitch)
 * - Integer/double inputs (ElaSpinBox/ElaDoubleSpinBox/ElaSlider)
 * - String inputs (ElaLineEdit)
 * - Enum selections (ElaComboBox)
 * - File/path selections (ElaLineEdit + browse button)
 * - Color pickers
 * - Grouped configuration sections
 */
class PluginConfigWidget : public QWidget {
    Q_OBJECT

public:
    explicit PluginConfigWidget(QWidget* parent = nullptr);
    ~PluginConfigWidget() override;

    /**
     * @brief Set the configuration model to display/edit
     * @param model The PluginConfigModel to use
     */
    void setModel(PluginConfigModel* model);

    /**
     * @brief Get the current model
     * @return Pointer to the model
     */
    PluginConfigModel* model() const { return m_model; }

    /**
     * @brief Rebuild the UI from the model
     */
    void rebuildUI();

    /**
     * @brief Show only required configuration entries
     * @param requiredOnly If true, only show required entries
     */
    void setRequiredOnly(bool requiredOnly);
    bool isRequiredOnly() const { return m_requiredOnly; }

    /**
     * @brief Show/hide advanced groups
     * @param showAdvanced If true, show advanced groups
     */
    void setShowAdvanced(bool showAdvanced);
    bool showsAdvanced() const { return m_showAdvanced; }

    /**
     * @brief Get validation errors for current values
     * @return List of validation error messages
     */
    QStringList getValidationErrors() const;

    /**
     * @brief Check if all required fields are filled
     * @return True if all required fields have values
     */
    bool isValid() const;

    /**
     * @brief Apply current values to the model
     */
    void applyToModel();

    /**
     * @brief Reset all values to defaults
     */
    void resetToDefaults();

signals:
    /**
     * @brief Emitted when any configuration value changes
     */
    void configurationChanged();

    /**
     * @brief Emitted when validation state changes
     * @param isValid True if all values are valid
     */
    void validationStateChanged(bool isValid);

protected:
    void changeEvent(QEvent* event) override;

public slots:
    /**
     * @brief Update validation error display for all entries
     */
    void updateValidationDisplay();

    /**
     * @brief Filter entries by search text
     * @param searchText Text to filter by (empty shows all)
     */
    void setSearchFilter(const QString& searchText);

private slots:
    void onBoolValueChanged(bool value);
    void onIntValueChanged(int value);
    void onDoubleValueChanged(double value);
    void onStringValueChanged(const QString& value);
    void onEnumValueChanged(int index);
    void onPathBrowseClicked();
    void onColorPickerClicked();

private:
    void setupUI();
    void clearUI();
    void retranslateUI();

    // Widget creation helpers
    QWidget* createEditorForEntry(const PluginConfigModel::ConfigEntry& entry);
    QWidget* createBoolEditor(const PluginConfigModel::ConfigEntry& entry);
    QWidget* createIntEditor(const PluginConfigModel::ConfigEntry& entry);
    QWidget* createDoubleEditor(const PluginConfigModel::ConfigEntry& entry);
    QWidget* createStringEditor(const PluginConfigModel::ConfigEntry& entry);
    QWidget* createTextAreaEditor(const PluginConfigModel::ConfigEntry& entry);
    QWidget* createEnumEditor(const PluginConfigModel::ConfigEntry& entry);
    QWidget* createPathEditor(const PluginConfigModel::ConfigEntry& entry);
    QWidget* createFileEditor(const PluginConfigModel::ConfigEntry& entry);
    QWidget* createColorEditor(const PluginConfigModel::ConfigEntry& entry);

    // Group creation
    QWidget* createGroupWidget(
        const PluginConfigModel::ConfigGroup& group,
        const QVector<PluginConfigModel::ConfigEntry>& entries);

    // Value management
    void updateModelValue(const QString& key, const QVariant& value);
    QVariant getEditorValue(const QString& key) const;

    // UI Components
    QVBoxLayout* m_mainLayout;
    QScrollArea* m_scrollArea;
    QWidget* m_scrollContent;
    QVBoxLayout* m_contentLayout;

    // Model
    QPointer<PluginConfigModel> m_model;

    // Editor tracking
    QHash<QString, QWidget*> m_editors;     // key -> editor widget
    QHash<QString, QLabel*> m_labels;       // key -> label widget
    QHash<QString, QLabel*> m_errorLabels;  // key -> error indicator label

    // State
    bool m_requiredOnly;
    bool m_showAdvanced;
    bool m_rebuildingUI;
    QString m_searchFilter;
};
