#ifndef PLUGINMANAGERPAGE_H
#define PLUGINMANAGERPAGE_H

#include <QStandardItemModel>
#include <QString>
#include "ElaScrollPage.h"

// Forward declarations - ElaWidgetTools
class ElaTableView;
class ElaPushButton;
class ElaLineEdit;
class ElaComboBox;
class ElaText;
class ElaCheckBox;

// Forward declarations - Qt
class QVBoxLayout;
class QHBoxLayout;
class QLabel;
class QSplitter;
class QTextEdit;

// Forward declarations - Business Logic
class PluginManager;

/**
 * @brief PluginManagerPage - Plugin management page
 *
 * Provides plugin management interface:
 * - Plugin list view with details
 * - Enable/disable plugins
 * - Install/uninstall plugins
 * - Plugin configuration
 * - Plugin information display
 */
class PluginManagerPage : public ElaScrollPage {
    Q_OBJECT

public:
    explicit PluginManagerPage(QWidget* parent = nullptr);
    ~PluginManagerPage() override;

    // Plugin management
    void refreshPluginList();
    void loadPluginSettings();
    void savePluginSettings();

signals:
    void pluginStateChanged(const QString& pluginName, bool enabled);
    void pluginInstalled(const QString& pluginPath);
    void pluginUninstalled(const QString& pluginName);

protected:
    void changeEvent(QEvent* event) override;

private slots:
    // UI event handlers
    void onPluginSelectionChanged();
    void onEnableDisableClicked();
    void onInstallClicked();
    void onUninstallClicked();
    void onConfigureClicked();
    void onRefreshClicked();
    void onFilterTextChanged(const QString& text);
    void onFilterCategoryChanged(int index);

    // PluginManager event handlers
    void onPluginLoaded(const QString& pluginName);
    void onPluginUnloaded(const QString& pluginName);
    void onPluginEnabled(const QString& pluginName);
    void onPluginDisabled(const QString& pluginName);
    void onPluginError(const QString& pluginName, const QString& error);

private:
    // UI initialization
    void setupUi();
    void setupToolbar();
    void setupPluginListView();
    void setupPluginDetails();
    void connectSignals();
    void retranslateUi();

    // Plugin list management
    void populatePluginList();
    void updatePluginDetails(const QString& pluginName);
    void clearPluginDetails();
    void applyFilter();

    // Helper methods
    QIcon getPluginIcon(const QString& pluginName) const;
    QString getPluginStatusText(const QString& pluginName) const;
    QColor getPluginStatusColor(const QString& pluginName) const;

    // ========================================================================
    // UI Components
    // ========================================================================

    // Toolbar
    QWidget* m_toolbarWidget;
    QHBoxLayout* m_toolbarLayout;
    ElaLineEdit* m_searchEdit;
    ElaComboBox* m_filterCombo;
    ElaPushButton* m_refreshBtn;
    ElaPushButton* m_installBtn;

    // Plugin list
    ElaTableView* m_pluginTableView;
    QStandardItemModel* m_pluginListModel;

    // Plugin details panel
    QWidget* m_detailsWidget;
    QVBoxLayout* m_detailsLayout;
    ElaText* m_pluginNameLabel;
    ElaText* m_pluginVersionLabel;
    ElaText* m_pluginAuthorLabel;
    ElaText* m_pluginStatusLabel;
    QTextEdit* m_pluginDescriptionEdit;
    QTextEdit* m_pluginDependenciesEdit;
    QTextEdit* m_pluginFeaturesEdit;

    // Action buttons
    QHBoxLayout* m_actionButtonsLayout;
    ElaPushButton* m_enableDisableBtn;
    ElaPushButton* m_uninstallBtn;
    ElaPushButton* m_configureBtn;

    // Main splitter
    QSplitter* m_mainSplitter;

    // ========================================================================
    // Business Logic
    // ========================================================================
    PluginManager* m_pluginManager;

    // ========================================================================
    // State
    // ========================================================================
    QString m_selectedPluginName;
    QString m_filterText;
    int m_filterCategory;  // 0=All, 1=Enabled, 2=Disabled, 3=Loaded, 4=Error

    // Column indices for table view
    enum ColumnIndex {
        ColumnName = 0,
        ColumnVersion = 1,
        ColumnStatus = 2,
        ColumnAuthor = 3,
        ColumnDescription = 4,
        ColumnCount
    };
};

#endif  // PLUGINMANAGERPAGE_H
