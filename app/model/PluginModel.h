#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QVariant>

#include "../logging/SimpleLogging.h"
#include "../plugin/PluginManager.h"

/**
 * @brief Model for managing plugin metadata and state
 *
 * This model provides a Qt Model/View compatible interface for displaying
 * and managing plugins in the SAST Readium application. It integrates with
 * the PluginManager and provides automatic updates when plugins are loaded,
 * unloaded, or their state changes.
 *
 * Features:
 * - Qt Model/View architecture integration
 * - Automatic synchronization with PluginManager
 * - Search and filtering capabilities
 * - Custom data roles for extended plugin information
 * - Event-driven updates via EventBus
 */
class PluginModel : public QAbstractListModel {
    Q_OBJECT

public:
    /**
     * Custom data roles for plugin information
     */
    enum PluginDataRole {
        NameRole = Qt::UserRole + 1,
        VersionRole,
        DescriptionRole,
        AuthorRole,
        FilePathRole,
        DependenciesRole,
        SupportedTypesRole,
        FeaturesRole,
        IsLoadedRole,
        IsEnabledRole,
        LoadTimeRole,
        ErrorsRole,
        ConfigurationRole,
        PluginTypeRole,  // "Document", "UI", "Specialized"
        StatusTextRole,  // Human-readable status string
        IconRole         // Plugin icon (if available)
    };
    Q_ENUM(PluginDataRole)

    explicit PluginModel(PluginManager* manager, QObject* parent = nullptr);
    ~PluginModel() override;

    Q_DISABLE_COPY_MOVE(PluginModel)

    // QAbstractItemModel interface
    [[nodiscard]] int rowCount(
        const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index,
                                int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    // Plugin operations
    Q_INVOKABLE bool loadPlugin(int row);
    Q_INVOKABLE bool unloadPlugin(int row);
    Q_INVOKABLE bool enablePlugin(int row);
    Q_INVOKABLE bool disablePlugin(int row);
    Q_INVOKABLE bool reloadPlugin(int row);

    // Query operations
    [[nodiscard]] Q_INVOKABLE QString getPluginName(int row) const;
    [[nodiscard]] Q_INVOKABLE bool isPluginLoaded(int row) const;
    [[nodiscard]] Q_INVOKABLE bool isPluginEnabled(int row) const;
    [[nodiscard]] Q_INVOKABLE PluginMetadata getPluginMetadata(int row) const;

    // Filtering and search
    Q_INVOKABLE void setFilterText(const QString& filter);
    Q_INVOKABLE void setShowOnlyLoaded(bool onlyLoaded);
    Q_INVOKABLE void setShowOnlyEnabled(bool onlyEnabled);
    Q_INVOKABLE void clearFilters();

    [[nodiscard]] QString filterText() const { return m_filterText; }
    [[nodiscard]] bool showOnlyLoaded() const { return m_showOnlyLoaded; }
    [[nodiscard]] bool showOnlyEnabled() const { return m_showOnlyEnabled; }

    // Model refresh
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void rescanPlugins();

    // Utility methods
    [[nodiscard]] int findPluginRow(const QString& pluginName) const;
    [[nodiscard]] QStringList getAllPluginNames() const;
    [[nodiscard]] int loadedPluginCount() const;
    [[nodiscard]] int enabledPluginCount() const;

signals:
    void pluginLoadStateChanged(const QString& pluginName, bool isLoaded);
    void pluginEnableStateChanged(const QString& pluginName, bool isEnabled);
    void pluginErrorOccurred(const QString& pluginName, const QString& error);
    void filterChanged();
    void modelRefreshed();

private slots:
    void onPluginLoaded(const QString& pluginName);
    void onPluginUnloaded(const QString& pluginName);
    void onPluginEnabled(const QString& pluginName);
    void onPluginDisabled(const QString& pluginName);
    void onPluginError(const QString& pluginName, const QString& error);
    void onPluginsScanned(int count);

private:
    void connectToPluginManager();
    void buildPluginList();
    void applyFilters();
    [[nodiscard]] bool matchesFilter(const PluginMetadata& metadata) const;
    [[nodiscard]] QString getStatusText(const PluginMetadata& metadata) const;
    [[nodiscard]] QString getPluginType(const PluginMetadata& metadata) const;

    QPointer<PluginManager> m_pluginManager;
    QStringList m_pluginNames;     // Filtered list of plugin names
    QStringList m_allPluginNames;  // Unfiltered list of all plugins
    QHash<QString, PluginMetadata> m_metadataCache;

    // Filtering state
    QString m_filterText;
    bool m_showOnlyLoaded;
    bool m_showOnlyEnabled;

    // Logging
    SastLogging::CategoryLogger m_logger{"PluginModel"};
};
