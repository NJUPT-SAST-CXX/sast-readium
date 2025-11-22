#include "UIStateManager.h"
#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMainWindow>
#include <QMetaProperty>
#include <QMutex>
#include <QMutexLocker>
#include <QSettings>
#include <QSplitter>
#include <QStandardPaths>
#include <QTimer>
#include "../../logging/SimpleLogging.h"

// UIStateManager Implementation

UIStateManager::UIStateManager()
    : QObject(nullptr),
      m_autosaveTimer(new QTimer(this)),
      m_autosaveEnabled(true),
      m_batchUpdateMode(false),
      m_compressionEnabled(true),
      m_encryptionEnabled(false),
      m_maxStateAgeDays(30),
      m_logger("UIStateManager") {
    // Set default state file path
    QString configDir =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    m_stateFilePath = QDir(configDir).filePath("ui_state.json");

    // Setup autosave timer
    m_autosaveTimer->setSingleShot(false);
    connect(m_autosaveTimer, &QTimer::timeout, this,
            &UIStateManager::onAutosaveTimer);

    // Load existing state
    loadStateFromFile();

    m_logger.info("UIStateManager initialized with state file: " +
                  m_stateFilePath);
}

UIStateManager::~UIStateManager() {
    if (m_autosaveEnabled) {
        saveStateToFile();
    }
    m_logger.debug("UIStateManager destroyed");
}

UIStateManager& UIStateManager::instance() {
    static UIStateManager instance;
    return instance;
}

// Core state management methods

void UIStateManager::setState(const QString& key, const QVariant& value,
                              StateScope scope, StatePriority priority,
                              const QString& component) {
    QMutexLocker locker(&m_stateMutex);

    if (key.isEmpty() || !isValidStateValue(value)) {
        m_logger.warning("Invalid state key or value: " + key);
        return;
    }

    QString fullKey = generateStateKey(key, scope);
    StateInfo info(key, value, scope, priority, component);

    m_states[fullKey] = info;

    if (!m_batchUpdateMode) {
        emit stateChanged(key, value, scope);
    }

    m_logger.debug(
        "State set: " + key + " = " + value.toString() +
        " (scope: " + QString::number(static_cast<int>(scope)) +
        ", priority: " + QString::number(static_cast<int>(priority)) + ")");
}

QVariant UIStateManager::getState(const QString& key,
                                  const QVariant& defaultValue,
                                  StateScope scope) const {
    QMutexLocker locker(&m_stateMutex);

    QString fullKey = generateStateKey(key, scope);
    auto it = m_states.find(fullKey);

    if (it != m_states.end()) {
        return it->value;
    }

    return defaultValue;
}

bool UIStateManager::hasState(const QString& key, StateScope scope) const {
    QMutexLocker locker(&m_stateMutex);

    QString fullKey = generateStateKey(key, scope);
    return m_states.contains(fullKey);
}

void UIStateManager::removeState(const QString& key, StateScope scope) {
    QMutexLocker locker(&m_stateMutex);

    QString fullKey = generateStateKey(key, scope);
    if (m_states.remove(fullKey) > 0) {
        m_logger.debug("State removed: " + key);
        if (!m_batchUpdateMode) {
            emit stateChanged(key, QVariant(), scope);
        }
    }
}

void UIStateManager::clearScope(StateScope scope) {
    QMutexLocker locker(&m_stateMutex);

    QString prefix = getScopePrefix(scope);
    auto it = m_states.begin();
    int removedCount = 0;

    while (it != m_states.end()) {
        if (it.key().startsWith(prefix)) {
            it = m_states.erase(it);
            removedCount++;
        } else {
            ++it;
        }
    }

    m_logger.info("Cleared " + QString::number(removedCount) +
                  " states from scope " +
                  QString::number(static_cast<int>(scope)));
}

// Component state management methods

void UIStateManager::registerComponent(QWidget* widget,
                                       const QString& componentId) {
    if (!widget) {
        m_logger.warning("Cannot register null widget");
        return;
    }

    QString id = componentId.isEmpty() ? widget->objectName() : componentId;
    if (id.isEmpty()) {
        id =
            QString("widget_%1").arg(reinterpret_cast<quintptr>(widget), 0, 16);
    }

    m_registeredComponents[widget] = id;

    // Connect to destroyed signal for cleanup
    connect(widget, &QObject::destroyed, this,
            &UIStateManager::onComponentDestroyed);

    m_logger.debug("Component registered: " + id + " (" +
                   widget->metaObject()->className() + ")");
}

void UIStateManager::unregisterComponent(QWidget* widget) {
    if (!widget)
        return;

    auto it = m_registeredComponents.find(widget);
    if (it != m_registeredComponents.end()) {
        QString componentId = it.value();
        m_registeredComponents.erase(it);
        m_componentStates.remove(componentId);

        disconnect(widget, &QObject::destroyed, this,
                   &UIStateManager::onComponentDestroyed);

        m_logger.debug("Component unregistered: " + componentId);
    }
}

void UIStateManager::saveComponentState(QWidget* widget) {
    if (!widget)
        return;

    auto it = m_registeredComponents.find(widget);
    if (it == m_registeredComponents.end()) {
        m_logger.debug("Widget not registered, auto-registering: " +
                       QString(widget->metaObject()->className()));
        registerComponent(widget, widget->objectName());
        it = m_registeredComponents.find(widget);
    }

    if (it != m_registeredComponents.end()) {
        QString componentId = it.value();
        QVariantMap state = captureWidgetState(widget);
        m_componentStates[componentId] = state;

        // Also save to persistent storage
        QString stateKey = QString("component/%1").arg(componentId);
        setState(stateKey, state, StateScope::User, StatePriority::Normal,
                 componentId);

        m_logger.debug("Component state saved: " + componentId + " (" +
                       QString::number(state.size()) + " properties)");
    }
}

void UIStateManager::restoreComponentState(QWidget* widget) {
    if (!widget)
        return;

    auto it = m_registeredComponents.find(widget);
    if (it == m_registeredComponents.end()) {
        m_logger.debug("Widget not registered for state restoration: " +
                       QString(widget->metaObject()->className()));
        return;
    }

    QString componentId = it.value();

    // Try to get state from memory first, then from persistent storage
    QVariantMap state;
    if (m_componentStates.contains(componentId)) {
        state = m_componentStates[componentId];
    } else {
        QString stateKey = QString("component/%1").arg(componentId);
        QVariant stateVariant =
            getState(stateKey, QVariant(), StateScope::User);
        if (stateVariant.isValid() && stateVariant.canConvert<QVariantMap>()) {
            state = stateVariant.toMap();
        }
    }

    if (!state.isEmpty()) {
        applyWidgetState(widget, state);
        m_logger.debug("Component state restored: " + componentId + " (" +
                       QString::number(state.size()) + " properties)");
    }
}

void UIStateManager::saveAllComponentStates() {
    int savedCount = 0;
    for (auto it = m_registeredComponents.begin();
         it != m_registeredComponents.end(); ++it) {
        if (it.key()) {  // Check if widget still exists
            saveComponentState(it.key());
            savedCount++;
        }
    }
    m_logger.info("Saved state for " + QString::number(savedCount) +
                  " components");
}

void UIStateManager::restoreAllComponentStates() {
    int restoredCount = 0;
    for (auto it = m_registeredComponents.begin();
         it != m_registeredComponents.end(); ++it) {
        if (it.key()) {  // Check if widget still exists
            restoreComponentState(it.key());
            restoredCount++;
        }
    }
    m_logger.info("Restored state for " + QString::number(restoredCount) +
                  " components");
}

// Window and splitter state management

void UIStateManager::saveWindowState(QMainWindow* window) {
    if (!window)
        return;

    QString windowKey = QString("window/%1").arg(window->objectName());
    if (windowKey == "window/") {
        windowKey = "window/main";
    }

    // Save geometry
    setState(windowKey + "/geometry", window->saveGeometry(), StateScope::User,
             StatePriority::High);

    // Save window state (toolbars, docks, etc.)
    setState(windowKey + "/state", window->saveState(), StateScope::User,
             StatePriority::High);

    // Save window flags and properties
    setState(windowKey + "/maximized", window->isMaximized(), StateScope::User);
    setState(windowKey + "/fullscreen", window->isFullScreen(),
             StateScope::User);

    m_logger.debug("Window state saved: " + windowKey);
}

void UIStateManager::restoreWindowState(QMainWindow* window) {
    if (!window)
        return;

    QString windowKey = QString("window/%1").arg(window->objectName());
    if (windowKey == "window/") {
        windowKey = "window/main";
    }

    // Restore geometry
    QVariant geometryData =
        getState(windowKey + "/geometry", QVariant(), StateScope::User);
    if (geometryData.isValid()) {
        window->restoreGeometry(geometryData.toByteArray());
    }

    // Restore window state
    QVariant stateData =
        getState(windowKey + "/state", QVariant(), StateScope::User);
    if (stateData.isValid()) {
        window->restoreState(stateData.toByteArray());
    }

    // Restore window flags
    bool wasMaximized =
        getState(windowKey + "/maximized", false, StateScope::User).toBool();
    bool wasFullscreen =
        getState(windowKey + "/fullscreen", false, StateScope::User).toBool();

    if (wasFullscreen) {
        window->showFullScreen();
    } else if (wasMaximized) {
        window->showMaximized();
    }

    m_logger.debug("Window state restored: " + windowKey);
}

void UIStateManager::saveGeometry(QWidget* widget, const QString& key) {
    if (!widget)
        return;

    QString geometryKey = key.isEmpty()
                              ? QString("geometry/%1").arg(widget->objectName())
                              : QString("geometry/%1").arg(key);

    setState(geometryKey, widget->saveGeometry(), StateScope::User,
             StatePriority::Normal);
    m_logger.debug("Geometry saved: " + geometryKey);
}

void UIStateManager::restoreGeometry(QWidget* widget, const QString& key) {
    if (!widget)
        return;

    QString geometryKey = key.isEmpty()
                              ? QString("geometry/%1").arg(widget->objectName())
                              : QString("geometry/%1").arg(key);

    QVariant geometryData = getState(geometryKey, QVariant(), StateScope::User);
    if (geometryData.isValid()) {
        widget->restoreGeometry(geometryData.toByteArray());
        m_logger.debug("Geometry restored: " + geometryKey);
    }
}

void UIStateManager::saveSplitterState(QSplitter* splitter,
                                       const QString& key) {
    if (!splitter)
        return;

    QString splitterKey =
        key.isEmpty() ? QString("splitter/%1").arg(splitter->objectName())
                      : QString("splitter/%1").arg(key);

    setState(splitterKey + "/sizes", QVariant::fromValue(splitter->sizes()),
             StateScope::User, StatePriority::Normal);
    setState(splitterKey + "/state", splitter->saveState(), StateScope::User,
             StatePriority::Normal);

    m_logger.debug("Splitter state saved: " + splitterKey);
}

void UIStateManager::restoreSplitterState(QSplitter* splitter,
                                          const QString& key) {
    if (!splitter)
        return;

    QString splitterKey =
        key.isEmpty() ? QString("splitter/%1").arg(splitter->objectName())
                      : QString("splitter/%1").arg(key);

    // Restore sizes
    QVariant sizesData =
        getState(splitterKey + "/sizes", QVariant(), StateScope::User);
    if (sizesData.isValid()) {
        QList<int> sizes = sizesData.value<QList<int>>();
        if (!sizes.isEmpty()) {
            splitter->setSizes(sizes);
        }
    }

    // Restore state
    QVariant stateData =
        getState(splitterKey + "/state", QVariant(), StateScope::User);
    if (stateData.isValid()) {
        splitter->restoreState(stateData.toByteArray());
    }

    m_logger.debug("Splitter state restored: " + splitterKey);
}

// Batch operations and persistence

void UIStateManager::beginBatchUpdate() {
    m_batchUpdateMode = true;
    m_logger.debug("Batch update mode enabled");
}

void UIStateManager::endBatchUpdate() {
    m_batchUpdateMode = false;
    m_logger.debug("Batch update mode disabled");
}

void UIStateManager::enableAutosave(bool enabled, int intervalMs) {
    m_autosaveEnabled = enabled;
    if (enabled) {
        m_autosaveTimer->start(intervalMs);
        m_logger.info(
            QString("Autosave enabled with interval: %1ms").arg(intervalMs));
    } else {
        m_autosaveTimer->stop();
        m_logger.info("Autosave disabled");
    }
}

void UIStateManager::forceSave() {
    saveStateToFile();
    m_logger.debug("Force save completed");
}

void UIStateManager::forceRestore() {
    loadStateFromFile();
    m_logger.debug("Force restore completed");
}

// Helper methods

QString UIStateManager::generateStateKey(const QString& key,
                                         StateScope scope) const {
    return getScopePrefix(scope) + key;
}

QString UIStateManager::getScopePrefix(StateScope scope) const {
    switch (scope) {
        case StateScope::Session:
            return "session/";
        case StateScope::User:
            return "user/";
        case StateScope::Global:
            return "global/";
        case StateScope::Component:
            return "component/";
        default:
            return "user/";
    }
}

QVariantMap UIStateManager::captureWidgetState(QWidget* widget) {
    QVariantMap state;

    if (!widget)
        return state;

    // Basic properties
    state["geometry"] = widget->geometry();
    state["visible"] = widget->isVisible();
    state["enabled"] = widget->isEnabled();
    state["styleSheet"] = widget->styleSheet();

    // Widget-specific properties using meta-object system
    const QMetaObject* metaObj = widget->metaObject();
    for (int i = 0; i < metaObj->propertyCount(); ++i) {
        QMetaProperty prop = metaObj->property(i);
        if (prop.isReadable() && prop.isWritable() && prop.isStored()) {
            QString propName = prop.name();
            QVariant value = widget->property(propName.toUtf8());
            if (value.isValid() && isValidStateValue(value)) {
                state[propName] = value;
            }
        }
    }

    return state;
}

void UIStateManager::applyWidgetState(QWidget* widget,
                                      const QVariantMap& state) {
    if (!widget || state.isEmpty())
        return;

    // Apply basic properties
    if (state.contains("geometry")) {
        widget->setGeometry(state["geometry"].toRect());
    }
    if (state.contains("visible")) {
        widget->setVisible(state["visible"].toBool());
    }
    if (state.contains("enabled")) {
        widget->setEnabled(state["enabled"].toBool());
    }
    if (state.contains("styleSheet")) {
        widget->setStyleSheet(state["styleSheet"].toString());
    }

    // Apply other properties
    for (auto it = state.begin(); it != state.end(); ++it) {
        if (it.key() != "geometry" && it.key() != "visible" &&
            it.key() != "enabled" && it.key() != "styleSheet") {
            widget->setProperty(it.key().toUtf8(), it.value());
        }
    }
}

bool UIStateManager::isValidStateValue(const QVariant& value) {
    // Check if the value can be serialized
    return value.isValid() && !value.isNull() &&
           (value.canConvert<QString>() || value.canConvert<QByteArray>() ||
            value.canConvert<int>() || value.canConvert<double>() ||
            value.canConvert<bool>() || value.canConvert<QVariantMap>() ||
            value.canConvert<QVariantList>());
}

// Slots

void UIStateManager::onAutosaveTimer() {
    if (m_autosaveEnabled && !m_batchUpdateMode) {
        QElapsedTimer timer;
        timer.start();

        saveStateToFile();
        cleanupExpiredStates();

        qint64 elapsedMs = timer.elapsed();
        m_logger.debug("UIStateManager autosave completed in " +
                       QString::number(elapsedMs) +
                       " ms (states=" + QString::number(m_states.size()) + ")");
    }
}

void UIStateManager::onComponentDestroyed(QObject* object) {
    QWidget* widget = qobject_cast<QWidget*>(object);
    if (widget) {
        unregisterComponent(widget);
    }
}

// File persistence methods

void UIStateManager::saveStateToFile() {
    QMutexLocker locker(&m_stateMutex);

    ensureStateDirectory();

    QJsonObject rootObj;
    QJsonObject statesObj;

    // Convert states to JSON
    for (auto it = m_states.begin(); it != m_states.end(); ++it) {
        const StateInfo& info = it.value();

        QJsonObject stateObj;
        stateObj["value"] = QJsonValue::fromVariant(info.value);
        stateObj["scope"] = static_cast<int>(info.scope);
        stateObj["priority"] = static_cast<int>(info.priority);
        stateObj["lastModified"] = info.lastModified.toString(Qt::ISODate);
        stateObj["component"] = info.component;

        statesObj[it.key()] = stateObj;
    }

    rootObj["states"] = statesObj;
    rootObj["version"] = "1.0";
    rootObj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QJsonDocument doc(rootObj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    QFile file(m_stateFilePath);
    if (file.open(QIODevice::WriteOnly)) {
        qint64 written = file.write(data);
        if (written == data.size()) {
            m_logger.debug("State saved to file: " + m_stateFilePath + " (" +
                           QString::number(data.size()) + " bytes)");
            emit stateSaved(StateScope::User, m_states.size());
        } else {
            m_logger.error("Failed to write complete state file");
            emit stateError("save", "Incomplete write operation");
        }
    } else {
        m_logger.error("Failed to open state file for writing: " +
                       file.errorString());
        emit stateError("save", file.errorString());
    }
}

void UIStateManager::loadStateFromFile() {
    QMutexLocker locker(&m_stateMutex);

    QFile file(m_stateFilePath);
    if (!file.exists()) {
        m_logger.debug("State file does not exist: " + m_stateFilePath);
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        m_logger.error("Failed to open state file for reading: " +
                       file.errorString());
        emit stateError("load", file.errorString());
        return;
    }

    QByteArray data = file.readAll();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        m_logger.error("Failed to parse state file: " +
                       parseError.errorString());
        emit stateError("load", parseError.errorString());
        return;
    }

    QJsonObject rootObj = doc.object();
    QJsonObject statesObj = rootObj["states"].toObject();

    int loadedCount = 0;
    for (auto it = statesObj.begin(); it != statesObj.end(); ++it) {
        QJsonObject stateObj = it.value().toObject();

        StateInfo info;
        info.key = it.key();
        info.value = stateObj["value"].toVariant();
        info.scope = static_cast<StateScope>(stateObj["scope"].toInt());
        info.priority =
            static_cast<StatePriority>(stateObj["priority"].toInt());
        info.lastModified = QDateTime::fromString(
            stateObj["lastModified"].toString(), Qt::ISODate);
        info.component = stateObj["component"].toString();

        if (isValidStateValue(info.value)) {
            m_states[it.key()] = info;
            loadedCount++;
        }
    }

    m_logger.info("State loaded from file: " + QString::number(loadedCount) +
                  " states");
    emit stateRestored(StateScope::User, loadedCount);
}

void UIStateManager::ensureStateDirectory() {
    QFileInfo fileInfo(m_stateFilePath);
    QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        if (dir.mkpath(".")) {
            m_logger.debug("Created state directory: " + dir.path());
        } else {
            m_logger.error("Failed to create state directory: " + dir.path());
        }
    }
}

void UIStateManager::cleanupExpiredStates() {
    if (m_maxStateAgeDays <= 0)
        return;

    QMutexLocker locker(&m_stateMutex);
    QDateTime cutoffDate =
        QDateTime::currentDateTime().addDays(-m_maxStateAgeDays);

    auto it = m_states.begin();
    int removedCount = 0;

    while (it != m_states.end()) {
        if (it->lastModified < cutoffDate &&
            it->priority == StatePriority::Low) {
            it = m_states.erase(it);
            removedCount++;
        } else {
            ++it;
        }
    }

    if (removedCount > 0) {
        m_logger.info("Cleaned up " + QString::number(removedCount) +
                      " expired states");
    }
}

// ComponentStateGuard Implementation

ComponentStateGuard::ComponentStateGuard(QWidget* widget,
                                         const QString& componentId)
    : m_widget(widget), m_componentId(componentId), m_committed(false) {
    if (m_widget) {
        UIStateManager& manager = UIStateManager::instance();
        if (!componentId.isEmpty()) {
            manager.registerComponent(m_widget, componentId);
        }
        m_originalState = manager.captureWidgetState(m_widget);
    }
}

ComponentStateGuard::~ComponentStateGuard() {
    if (!m_committed && m_widget) {
        rollback();
    }
}

void ComponentStateGuard::commit() {
    if (m_widget) {
        UIStateManager::instance().saveComponentState(m_widget);
    }
    m_committed = true;
}

void ComponentStateGuard::rollback() {
    if (m_widget && !m_originalState.isEmpty()) {
        UIStateManager::instance().applyWidgetState(m_widget, m_originalState);
    }
    m_committed = true;
}

// StateBinding Implementation

StateBinding::StateBinding(QWidget* widget, const QString& stateKey,
                           const QString& property, QObject* parent)
    : QObject(parent),
      m_widget(widget),
      m_stateKey(stateKey),
      m_property(property.isEmpty() ? QString("value") : property),
      m_twoWay(false) {
    if (!m_widget) {
        return;
    }

    // Connect to state manager for state changes
    UIStateManager& manager = UIStateManager::instance();
    connect(&manager, &UIStateManager::stateChanged, this,
            &StateBinding::onStateChanged);

    // If two-way binding, monitor widget property changes
    if (m_twoWay && !m_property.isEmpty()) {
        // Find the property and connect to its notify signal if available
        const QMetaObject* metaObj = m_widget->metaObject();
        int propIndex =
            metaObj->indexOfProperty(m_property.toUtf8().constData());

        if (propIndex >= 0) {
            QMetaProperty metaProp = metaObj->property(propIndex);
            if (metaProp.hasNotifySignal()) {
                QMetaMethod signal = metaProp.notifySignal();
                QMetaMethod slot = metaObject()->method(
                    metaObject()->indexOfSlot("onWidgetPropertyChanged()"));
                connect(m_widget, signal, this, slot);
            }
        }
    }

    // Apply initial state value to widget
    QVariant initialValue = manager.getState(m_stateKey);
    if (initialValue.isValid()) {
        onStateChanged(m_stateKey, initialValue,
                       UIStateManager::StateScope::Session);
    }
}

void StateBinding::onStateChanged(const QString& key, const QVariant& value,
                                  UIStateManager::StateScope scope) {
    Q_UNUSED(scope)

    if (key != m_stateKey || !m_widget) {
        return;
    }

    // Apply transform if available
    QVariant transformedValue = value;
    if (m_toWidgetTransform) {
        transformedValue = m_toWidgetTransform(value);
    }

    // Set the widget property
    if (!m_property.isEmpty()) {
        m_widget->setProperty(m_property.toUtf8().constData(),
                              transformedValue);
    }
}

void StateBinding::onWidgetPropertyChanged() {
    if (!m_widget || !m_twoWay || m_property.isEmpty()) {
        return;
    }

    // Get current widget property value
    QVariant widgetValue = m_widget->property(m_property.toUtf8().constData());

    // Apply transform if available
    QVariant transformedValue = widgetValue;
    if (m_fromWidgetTransform) {
        transformedValue = m_fromWidgetTransform(widgetValue);
    }

    // Update state manager
    UIStateManager::instance().setState(m_stateKey, transformedValue);
}
