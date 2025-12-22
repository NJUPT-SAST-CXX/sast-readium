#pragma once

#include <QAbstractTableModel>
#include <QJsonObject>
#include <QPointer>
#include <QString>
#include <QVariant>
#include <QVector>

#include "../logging/SimpleLogging.h"
#include "../plugin/PluginManager.h"

/**
 * @brief Model for managing plugin configuration settings
 *
 * This model provides a table-based interface for viewing and editing
 * plugin configuration options. It supports JSON-based configuration
 * and provides validation and type conversion for configuration values.
 *
 * Features:
 * - Qt Model/View architecture integration
 * - JSON configuration management
 * - Type-aware value editing (bool, int, string, etc.)
 * - Configuration validation with schema support
 * - Undo/redo support via ConfigurePluginCommand
 * - Real-time configuration updates
 * - Configuration schema with constraints, groups, and descriptions
 */
class PluginConfigModel : public QAbstractTableModel {
    Q_OBJECT

public:
    /**
     * Table columns
     */
    enum Column {
        KeyColumn = 0,
        ValueColumn = 1,
        TypeColumn = 2,
        DescriptionColumn = 3,
        ColumnCount = 4
    };

    /**
     * Configuration entry structure with full schema support
     */
    struct ConfigEntry {
        QString key;     // Configuration key
        QVariant value;  // Current value
        QString type;    // "bool", "int", "double", "string", "enum", "path",
                         // "color", "object", "array"
        QString description;  // Human-readable description
        QString group;    // Configuration group (e.g., "general", "advanced")
        bool isRequired;  // Whether this config is required
        bool isReadOnly;  // Cannot be edited
        QVariant defaultValue;   // Default value
        QVariant minValue;       // Minimum value (for numeric types)
        QVariant maxValue;       // Maximum value (for numeric types)
        QStringList enumValues;  // Valid values for enum type
        QString placeholder;     // Placeholder text for input fields
        QString displayName;     // Display name (if different from key)
        int order;               // Display order within group

        ConfigEntry() : isRequired(false), isReadOnly(false), order(0) {}

        ConfigEntry(const QString& k, const QVariant& v,
                    const QString& t = "string", const QString& desc = "",
                    bool readOnly = false)
            : key(k),
              value(v),
              type(t),
              description(desc),
              group("general"),
              isRequired(false),
              isReadOnly(readOnly),
              defaultValue(v),
              order(0) {}
    };

    /**
     * Configuration group metadata
     */
    struct ConfigGroup {
        QString id;           // Group identifier
        QString displayName;  // Display name
        QString description;  // Group description
        QString icon;         // Icon name (optional)
        int order;            // Display order
        bool isCollapsible;   // Whether group can be collapsed
        bool isAdvanced;      // Whether this is an advanced group

        ConfigGroup() : order(0), isCollapsible(true), isAdvanced(false) {}

        ConfigGroup(const QString& id, const QString& name,
                    const QString& desc = "", int ord = 0)
            : id(id),
              displayName(name),
              description(desc),
              order(ord),
              isCollapsible(true),
              isAdvanced(false) {}
    };

    explicit PluginConfigModel(PluginManager* manager,
                               const QString& pluginName = QString(),
                               QObject* parent = nullptr);
    ~PluginConfigModel() override;

    Q_DISABLE_COPY_MOVE(PluginConfigModel)

    // QAbstractItemModel interface
    [[nodiscard]] int rowCount(
        const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] int columnCount(
        const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index,
                                int role = Qt::DisplayRole) const override;
    [[nodiscard]] QVariant headerData(
        int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value,
                 int role = Qt::EditRole) override;

    // Configuration management
    Q_INVOKABLE void setPluginName(const QString& pluginName);
    [[nodiscard]] QString pluginName() const { return m_pluginName; }

    Q_INVOKABLE void loadConfiguration();
    Q_INVOKABLE bool saveConfiguration();
    Q_INVOKABLE void resetToDefaults();

    // Configuration access
    [[nodiscard]] QJsonObject getConfiguration() const;
    void setConfiguration(const QJsonObject& config);

    // Schema management
    void setConfigSchema(const QJsonObject& schema);
    [[nodiscard]] QJsonObject getConfigSchema() const { return m_configSchema; }
    [[nodiscard]] bool hasSchema() const { return !m_configSchema.isEmpty(); }

    // Group management
    [[nodiscard]] QList<ConfigGroup> getGroups() const { return m_groups; }
    [[nodiscard]] QVector<ConfigEntry> getEntriesForGroup(
        const QString& groupId) const;
    [[nodiscard]] QStringList getGroupIds() const;
    void addGroup(const ConfigGroup& group);

    // Required configuration
    [[nodiscard]] QVector<ConfigEntry> getRequiredEntries() const;
    [[nodiscard]] bool hasRequiredUnset() const;
    [[nodiscard]] QStringList getRequiredUnsetKeys() const;

    // Entry management
    Q_INVOKABLE bool addEntry(const QString& key, const QVariant& value,
                              const QString& type = "string",
                              const QString& description = "");
    Q_INVOKABLE bool removeEntry(int row);
    Q_INVOKABLE bool removeEntry(const QString& key);

    // Entry access
    [[nodiscard]] const ConfigEntry& getEntry(int index) const;
    [[nodiscard]] int entryCount() const { return m_entries.size(); }

    // Query operations
    [[nodiscard]] Q_INVOKABLE bool hasKey(const QString& key) const;
    [[nodiscard]] Q_INVOKABLE QVariant getValue(const QString& key) const;
    [[nodiscard]] Q_INVOKABLE QString getType(const QString& key) const;
    Q_INVOKABLE bool setValue(const QString& key, const QVariant& value);

    // Validation
    [[nodiscard]] bool isValidValue(const QString& type,
                                    const QVariant& value) const;
    [[nodiscard]] bool validateEntry(const ConfigEntry& entry,
                                     const QVariant& value) const;
    [[nodiscard]] QString validateConfiguration() const;
    [[nodiscard]] QStringList validateAllEntries() const;

    // State
    [[nodiscard]] bool isModified() const { return m_isModified; }
    void setModified(bool modified) { m_isModified = modified; }

signals:
    void configurationChanged();
    void configurationSaved();
    void configurationLoaded();
    void entryAdded(const QString& key);
    void entryRemoved(const QString& key);
    void valueChanged(const QString& key, const QVariant& oldValue,
                      const QVariant& newValue);
    void errorOccurred(const QString& error);

private:
    void buildConfigEntries();
    void buildConfigEntriesFromSchema();
    void addEntryFromJson(const QString& key, const QVariant& value);
    void addEntryFromSchema(const QString& key, const QJsonObject& schema);
    void parseGroupsFromSchema(const QJsonObject& schema);
    [[nodiscard]] QString detectType(const QVariant& value) const;
    [[nodiscard]] int findEntryRow(const QString& key) const;
    [[nodiscard]] QVariant convertValue(const QString& type,
                                        const QVariant& value) const;
    [[nodiscard]] QVariant getDefaultForType(const QString& type) const;

    QPointer<PluginManager> m_pluginManager;
    QString m_pluginName;
    QVector<ConfigEntry> m_entries;
    QList<ConfigGroup> m_groups;
    QJsonObject m_originalConfig;  // For detecting modifications
    QJsonObject m_configSchema;    // Configuration schema
    bool m_isModified;

    // Logging
    SastLogging::CategoryLogger m_logger{"PluginConfigModel"};
};
