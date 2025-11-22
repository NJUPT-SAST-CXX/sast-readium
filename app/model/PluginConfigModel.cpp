#include "PluginConfigModel.h"

#include <QJsonArray>
#include <QJsonDocument>

PluginConfigModel::PluginConfigModel(PluginManager* manager,
                                     const QString& pluginName, QObject* parent)
    : QAbstractTableModel(parent),
      m_pluginManager(manager),
      m_pluginName(pluginName),
      m_isModified(false),
      m_logger("PluginConfigModel") {
    if (!m_pluginManager) {
        m_logger.error("PluginConfigModel created with null PluginManager");
        return;
    }

    if (!m_pluginName.isEmpty()) {
        loadConfiguration();
    }
}

PluginConfigModel::~PluginConfigModel() = default;

int PluginConfigModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return m_entries.size();
}

int PluginConfigModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return ColumnCount;
}

QVariant PluginConfigModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_entries.size()) {
        return QVariant();
    }

    const ConfigEntry& entry = m_entries[index.row()];

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
            case KeyColumn:
                return entry.key;
            case ValueColumn:
                return entry.value;
            case TypeColumn:
                return entry.type;
            case DescriptionColumn:
                return entry.description;
            default:
                return QVariant();
        }
    }

    if (role == Qt::ToolTipRole) {
        if (!entry.description.isEmpty()) {
            return entry.description;
        }
        return QString("%1: %2").arg(entry.key, entry.type);
    }

    if (role == Qt::FontRole && entry.isReadOnly) {
        QFont font;
        font.setItalic(true);
        return font;
    }

    return QVariant();
}

QVariant PluginConfigModel::headerData(int section, Qt::Orientation orientation,
                                       int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant();
    }

    switch (section) {
        case KeyColumn:
            return tr("Key");
        case ValueColumn:
            return tr("Value");
        case TypeColumn:
            return tr("Type");
        case DescriptionColumn:
            return tr("Description");
        default:
            return QVariant();
    }
}

Qt::ItemFlags PluginConfigModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    Qt::ItemFlags flags = QAbstractTableModel::flags(index);

    // Only the Value column is editable, and only for non-readonly entries
    if (index.column() == ValueColumn && !m_entries[index.row()].isReadOnly) {
        flags |= Qt::ItemIsEditable;
    }

    return flags;
}

bool PluginConfigModel::setData(const QModelIndex& index, const QVariant& value,
                                int role) {
    if (!index.isValid() || index.row() >= m_entries.size() ||
        index.column() != ValueColumn || role != Qt::EditRole) {
        return false;
    }

    ConfigEntry& entry = m_entries[index.row()];

    if (entry.isReadOnly) {
        m_logger.warning(QString("Attempt to modify read-only config entry: %1")
                             .arg(entry.key));
        return false;
    }

    // Validate the new value
    if (!isValidValue(entry.type, value)) {
        m_logger.error(
            QString("Invalid value type for key %1: expected %2, got %3")
                .arg(entry.key, entry.type,
                     QString::fromLatin1(value.typeName())));
        emit errorOccurred(tr("Invalid value type for %1").arg(entry.key));
        return false;
    }

    // Convert and set the value
    QVariant oldValue = entry.value;
    entry.value = convertValue(entry.type, value);

    if (entry.value != oldValue) {
        m_isModified = true;
        emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
        emit valueChanged(entry.key, oldValue, entry.value);
        emit configurationChanged();

        m_logger.info(QString("Configuration value changed: %1 = %2")
                          .arg(entry.key, entry.value.toString()));
    }

    return true;
}

void PluginConfigModel::setPluginName(const QString& pluginName) {
    if (m_pluginName == pluginName) {
        return;
    }

    m_pluginName = pluginName;
    loadConfiguration();
}

void PluginConfigModel::loadConfiguration() {
    if (!m_pluginManager || m_pluginName.isEmpty()) {
        return;
    }

    m_logger.info(
        QString("Loading configuration for plugin: %1").arg(m_pluginName));

    beginResetModel();

    m_entries.clear();
    m_originalConfig = m_pluginManager->getPluginConfiguration(m_pluginName);
    buildConfigEntries();
    m_isModified = false;

    endResetModel();

    emit configurationLoaded();
}

bool PluginConfigModel::saveConfiguration() {
    if (!m_pluginManager || m_pluginName.isEmpty()) {
        return false;
    }

    m_logger.info(
        QString("Saving configuration for plugin: %1").arg(m_pluginName));

    QJsonObject config = getConfiguration();

    // Validate configuration
    QString validationError = validateConfiguration();
    if (!validationError.isEmpty()) {
        m_logger.error(QString("Configuration validation failed: %1")
                           .arg(validationError));
        emit errorOccurred(validationError);
        return false;
    }

    // Save to plugin manager
    m_pluginManager->setPluginConfiguration(m_pluginName, config);

    m_originalConfig = config;
    m_isModified = false;

    emit configurationSaved();

    return true;
}

void PluginConfigModel::resetToDefaults() {
    if (m_originalConfig.isEmpty()) {
        return;
    }

    m_logger.info("Resetting configuration to defaults");

    setConfiguration(m_originalConfig);
    m_isModified = false;
}

QJsonObject PluginConfigModel::getConfiguration() const {
    QJsonObject config;

    for (const ConfigEntry& entry : m_entries) {
        // Convert QVariant to appropriate JSON value
        if (entry.type == "bool") {
            config[entry.key] = entry.value.toBool();
        } else if (entry.type == "int") {
            config[entry.key] = entry.value.toInt();
        } else if (entry.type == "double") {
            config[entry.key] = entry.value.toDouble();
        } else if (entry.type == "string") {
            config[entry.key] = entry.value.toString();
        } else if (entry.type == "array") {
            config[entry.key] = entry.value.toJsonArray();
        } else if (entry.type == "object") {
            config[entry.key] = entry.value.toJsonObject();
        } else {
            config[entry.key] = entry.value.toString();
        }
    }

    return config;
}

void PluginConfigModel::setConfiguration(const QJsonObject& config) {
    beginResetModel();

    m_entries.clear();

    for (auto it = config.begin(); it != config.end(); ++it) {
        addEntryFromJson(it.key(), it.value());
    }

    m_isModified = true;

    endResetModel();

    emit configurationChanged();
}

void PluginConfigModel::buildConfigEntries() {
    for (auto it = m_originalConfig.begin(); it != m_originalConfig.end();
         ++it) {
        addEntryFromJson(it.key(), it.value());
    }
}

void PluginConfigModel::addEntryFromJson(const QString& key,
                                         const QVariant& value) {
    QString type = detectType(value);
    ConfigEntry entry(key, value, type, "", false);
    m_entries.append(entry);
}

QString PluginConfigModel::detectType(const QVariant& value) const {
    switch (value.typeId()) {
        case QMetaType::Bool:
            return "bool";
        case QMetaType::Int:
        case QMetaType::LongLong:
            return "int";
        case QMetaType::Double:
        case QMetaType::Float:
            return "double";
        case QMetaType::QString:
            return "string";
        case QMetaType::QJsonArray:
        case QMetaType::QVariantList:
            return "array";
        case QMetaType::QJsonObject:
        case QMetaType::QVariantMap:
            return "object";
        default:
            return "string";
    }
}

bool PluginConfigModel::addEntry(const QString& key, const QVariant& value,
                                 const QString& type,
                                 const QString& description) {
    if (key.isEmpty() || hasKey(key)) {
        m_logger.warning(
            QString("Cannot add entry: key %1 is empty or already exists")
                .arg(key));
        return false;
    }

    int row = m_entries.size();
    beginInsertRows(QModelIndex(), row, row);

    ConfigEntry entry(key, value, type, description, false);
    m_entries.append(entry);

    endInsertRows();

    m_isModified = true;
    emit entryAdded(key);
    emit configurationChanged();

    return true;
}

bool PluginConfigModel::removeEntry(int row) {
    if (row < 0 || row >= m_entries.size()) {
        return false;
    }

    QString key = m_entries[row].key;

    beginRemoveRows(QModelIndex(), row, row);
    m_entries.remove(row);
    endRemoveRows();

    m_isModified = true;
    emit entryRemoved(key);
    emit configurationChanged();

    return true;
}

bool PluginConfigModel::removeEntry(const QString& key) {
    int row = findEntryRow(key);
    if (row < 0) {
        return false;
    }
    return removeEntry(row);
}

bool PluginConfigModel::hasKey(const QString& key) const {
    return findEntryRow(key) >= 0;
}

QVariant PluginConfigModel::getValue(const QString& key) const {
    int row = findEntryRow(key);
    if (row < 0) {
        return QVariant();
    }
    return m_entries[row].value;
}

QString PluginConfigModel::getType(const QString& key) const {
    int row = findEntryRow(key);
    if (row < 0) {
        return QString();
    }
    return m_entries[row].type;
}

bool PluginConfigModel::setValue(const QString& key, const QVariant& value) {
    int row = findEntryRow(key);
    if (row < 0) {
        return false;
    }

    QModelIndex idx = index(row, ValueColumn);
    return setData(idx, value, Qt::EditRole);
}

int PluginConfigModel::findEntryRow(const QString& key) const {
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].key == key) {
            return i;
        }
    }
    return -1;
}

bool PluginConfigModel::isValidValue(const QString& type,
                                     const QVariant& value) const {
    if (type == "bool") {
        return value.canConvert<bool>();
    }
    if (type == "int") {
        return value.canConvert<int>();
    }
    if (type == "double") {
        return value.canConvert<double>();
    }
    if (type == "string") {
        return value.canConvert<QString>();
    }
    if (type == "array") {
        return value.canConvert<QJsonArray>() ||
               value.typeId() == QMetaType::QVariantList;
    }
    if (type == "object") {
        return value.canConvert<QJsonObject>() ||
               value.typeId() == QMetaType::QVariantMap;
    }
    return true;  // Unknown types accepted as-is
}

QString PluginConfigModel::validateConfiguration() const {
    // Basic validation - check for required types
    for (const ConfigEntry& entry : m_entries) {
        if (!isValidValue(entry.type, entry.value)) {
            return tr("Invalid value for key %1: expected type %2")
                .arg(entry.key, entry.type);
        }
    }

    return QString();  // No errors
}

QVariant PluginConfigModel::convertValue(const QString& type,
                                         const QVariant& value) const {
    if (type == "bool") {
        return value.toBool();
    }
    if (type == "int") {
        return value.toInt();
    }
    if (type == "double") {
        return value.toDouble();
    }
    if (type == "string") {
        return value.toString();
    }
    if (type == "array") {
        if (value.typeId() == QMetaType::QJsonArray) {
            return value;
        }
        return QJsonArray::fromVariantList(value.toList());
    }
    if (type == "object") {
        if (value.typeId() == QMetaType::QJsonObject) {
            return value;
        }
        return QJsonObject::fromVariantMap(value.toMap());
    }
    return value;
}
