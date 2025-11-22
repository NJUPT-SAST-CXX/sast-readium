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
 * - Configuration validation
 * - Undo/redo support via ConfigurePluginCommand
 * - Real-time configuration updates
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
     * Configuration entry structure
     */
    struct ConfigEntry {
        QString key;
        QVariant value;
        QString type;  // "bool", "int", "double", "string", "object", "array"
        QString description;  // Optional description
        bool isReadOnly;      // Cannot be edited

        ConfigEntry() : isReadOnly(false) {}

        ConfigEntry(const QString& k, const QVariant& v,
                    const QString& t = "string", const QString& desc = "",
                    bool readOnly = false)
            : key(k),
              value(v),
              type(t),
              description(desc),
              isReadOnly(readOnly) {}
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

    // Entry management
    Q_INVOKABLE bool addEntry(const QString& key, const QVariant& value,
                              const QString& type = "string",
                              const QString& description = "");
    Q_INVOKABLE bool removeEntry(int row);
    Q_INVOKABLE bool removeEntry(const QString& key);

    // Query operations
    [[nodiscard]] Q_INVOKABLE bool hasKey(const QString& key) const;
    [[nodiscard]] Q_INVOKABLE QVariant getValue(const QString& key) const;
    [[nodiscard]] Q_INVOKABLE QString getType(const QString& key) const;
    Q_INVOKABLE bool setValue(const QString& key, const QVariant& value);

    // Validation
    [[nodiscard]] bool isValidValue(const QString& type,
                                    const QVariant& value) const;
    [[nodiscard]] QString validateConfiguration() const;

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
    void addEntryFromJson(const QString& key, const QVariant& value);
    [[nodiscard]] QString detectType(const QVariant& value) const;
    [[nodiscard]] int findEntryRow(const QString& key) const;
    [[nodiscard]] QVariant convertValue(const QString& type,
                                        const QVariant& value) const;

    QPointer<PluginManager> m_pluginManager;
    QString m_pluginName;
    QVector<ConfigEntry> m_entries;
    QJsonObject m_originalConfig;  // For detecting modifications
    bool m_isModified;

    // Logging
    SastLogging::CategoryLogger m_logger{"PluginConfigModel"};
};
