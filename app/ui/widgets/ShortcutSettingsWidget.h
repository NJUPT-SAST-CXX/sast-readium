#pragma once

#include <QHash>
#include <QKeySequence>
#include <QWidget>

// Forward declarations
class QVBoxLayout;
class QStandardItemModel;
class ElaTableView;
class ElaLineEdit;
class ElaPushButton;
class ElaComboBox;
class ElaText;
class KeyboardShortcutManager;

/**
 * @brief Widget for editing keyboard shortcuts
 *
 * Provides a comprehensive UI for viewing and editing keyboard shortcuts.
 * Uses ElaWidgetTools components for consistent styling.
 */
class ShortcutSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit ShortcutSettingsWidget(QWidget* parent = nullptr);
    ~ShortcutSettingsWidget() override;

    void loadShortcuts();
    void saveShortcuts();
    void resetToDefaults();

signals:
    void shortcutsChanged();

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void onShortcutCellClicked(int row, int column);
    void onFilterTextChanged(const QString& text);
    void onCategoryFilterChanged(int index);
    void onResetAllClicked();
    void onResetSelectedClicked();

private:
    void setupUi();
    void retranslateUi();
    void populateShortcutsTable();
    void filterShortcuts();
    bool isValidShortcut(const QKeySequence& keySequence);
    bool hasConflict(const QKeySequence& keySequence, int currentRow);

    // UI Components
    QVBoxLayout* m_mainLayout;
    ElaLineEdit* m_searchEdit;
    ElaComboBox* m_categoryFilter;
    ElaTableView* m_shortcutsTable;
    QStandardItemModel* m_shortcutsModel;
    ElaPushButton* m_resetAllBtn;
    ElaPushButton* m_resetSelectedBtn;

    // Data
    struct ShortcutEntry {
        QString actionId;
        QString description;
        QString category;
        QKeySequence defaultShortcut;
        QKeySequence currentShortcut;
    };
    QList<ShortcutEntry> m_shortcuts;
    QHash<QString, QKeySequence> m_modifiedShortcuts;
};
