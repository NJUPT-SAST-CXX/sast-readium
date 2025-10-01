#include "StateManager.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QTimer>
#include <QDateTime>

// ============================================================================
// State Implementation
// ============================================================================

State::State(const QJsonObject& data) : m_data(data) {}

QVariant State::get(const QString& path) const {
    QJsonValue value = getValue(path);
    return value.toVariant();
}

QJsonObject State::getObject(const QString& path) const {
    QJsonValue value = getValue(path);
    return value.toObject();
}

QJsonValue State::getValue(const QString& path) const {
    QStringList parts = path.split('.', Qt::SkipEmptyParts);
    return getValueByPath(m_data, parts);
}

bool State::has(const QString& path) const {
    QStringList parts = path.split('.', Qt::SkipEmptyParts);
    QJsonValue value = getValueByPath(m_data, parts);
    return !value.isUndefined() && !value.isNull();
}

State State::set(const QString& path, const QVariant& value) const {
    QStringList parts = path.split('.', Qt::SkipEmptyParts);
    QJsonValue jsonValue = QJsonValue::fromVariant(value);
    QJsonObject newData = setValueByPath(m_data, parts, jsonValue);
    return State(newData);
}

State State::merge(const QJsonObject& data) const {
    QJsonObject newData = m_data;
    for (auto it = data.begin(); it != data.end(); ++it) {
        newData[it.key()] = it.value();
    }
    return State(newData);
}

State State::remove(const QString& path) const {
    QStringList parts = path.split('.', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return *this;
    }
    
    QJsonObject newData = m_data;
    if (parts.size() == 1) {
        newData.remove(parts[0]);
    } else {
        // Navigate to parent and remove the key
        QJsonObject* current = &newData;
        for (int i = 0; i < parts.size() - 1; ++i) {
            if (!current->contains(parts[i])) {
                return *this;
            }
            QJsonValue val = (*current)[parts[i]];
            if (!val.isObject()) {
                return *this;
            }
            QJsonObject obj = val.toObject();
            (*current)[parts[i]] = obj;
            current = &obj;
        }
        current->remove(parts.last());
    }
    
    return State(newData);
}

QString State::toString() const {
    QJsonDocument doc(m_data);
    return doc.toJson(QJsonDocument::Compact);
}

bool State::operator==(const State& other) const {
    return m_data == other.m_data;
}

bool State::operator!=(const State& other) const {
    return !(*this == other);
}

QJsonValue State::getValueByPath(const QJsonObject& obj, const QStringList& path) const {
    if (path.isEmpty()) {
        return QJsonValue(obj);
    }
    
    QJsonValue current = obj;
    for (const QString& key : path) {
        if (!current.isObject()) {
            return QJsonValue();
        }
        QJsonObject currentObj = current.toObject();
        if (!currentObj.contains(key)) {
            return QJsonValue();
        }
        current = currentObj[key];
    }
    
    return current;
}

QJsonObject State::setValueByPath(const QJsonObject& obj, const QStringList& path, const QJsonValue& value) const {
    if (path.isEmpty()) {
        return obj;
    }
    
    QJsonObject result = obj;
    if (path.size() == 1) {
        result[path[0]] = value;
        return result;
    }
    
    QString firstKey = path[0];
    QStringList remainingPath = path.mid(1);
    
    QJsonObject nested;
    if (result.contains(firstKey) && result[firstKey].isObject()) {
        nested = result[firstKey].toObject();
    }
    
    nested = setValueByPath(nested, remainingPath, value);
    result[firstKey] = nested;
    
    return result;
}

// ============================================================================
// StateChange Implementation
// ============================================================================

StateChange::StateChange(const State& oldState, const State& newState, const QString& reason)
    : m_oldState(oldState)
    , m_newState(newState)
    , m_reason(reason)
    , m_timestamp(QDateTime::currentMSecsSinceEpoch())
{}

QStringList StateChange::changedPaths() const {
    // Simple implementation - compare JSON objects
    QStringList paths;
    // This is a simplified version - a full implementation would recursively compare
    return paths;
}

bool StateChange::hasChanged(const QString& path) const {
    return m_oldState.get(path) != m_newState.get(path);
}

QVariant StateChange::oldValue(const QString& path) const {
    return m_oldState.get(path);
}

QVariant StateChange::newValue(const QString& path) const {
    return m_newState.get(path);
}

// ============================================================================
// StateManager Implementation
// ============================================================================

StateManager::StateManager(QObject* parent)
    : QObject(parent)
    , m_logger("StateManager")
{
    m_logger.debug("StateManager initialized");
}

StateManager::~StateManager() {
    if (m_autoSaveTimer) {
        m_autoSaveTimer->stop();
        delete m_autoSaveTimer;
    }
}

StateManager& StateManager::instance() {
    static StateManager instance;
    return instance;
}

State StateManager::currentState() const {
    QMutexLocker locker(&m_mutex);
    return m_currentState;
}

QVariant StateManager::get(const QString& path) const {
    QMutexLocker locker(&m_mutex);
    return m_currentState.get(path);
}

bool StateManager::has(const QString& path) const {
    QMutexLocker locker(&m_mutex);
    return m_currentState.has(path);
}

void StateManager::set(const QString& path, const QVariant& value, const QString& reason) {
    QMutexLocker locker(&m_mutex);
    State newState = m_currentState.set(path, value);
    locker.unlock();
    
    setState(newState, reason.isEmpty() ? QString("Set %1").arg(path) : reason);
}

void StateManager::merge(const QJsonObject& data, const QString& reason) {
    QMutexLocker locker(&m_mutex);
    State newState = m_currentState.merge(data);
    locker.unlock();
    
    setState(newState, reason.isEmpty() ? "Merge" : reason);
}

void StateManager::remove(const QString& path, const QString& reason) {
    QMutexLocker locker(&m_mutex);
    State newState = m_currentState.remove(path);
    locker.unlock();
    
    setState(newState, reason.isEmpty() ? QString("Remove %1").arg(path) : reason);
}

void StateManager::reset(const State& newState, const QString& reason) {
    setState(newState, reason);
}

void StateManager::subscribe(const QString& path, QObject* subscriber, StateObserver observer) {
    QMutexLocker locker(&m_mutex);
    
    Subscription sub;
    sub.path = path;
    sub.subscriber = subscriber;
    sub.observer = observer;
    
    m_subscriptions.append(sub);
    
    // Connect to destroyed signal to auto-cleanup
    connect(subscriber, &QObject::destroyed, this, &StateManager::onSubscriberDestroyed, Qt::UniqueConnection);
    
    m_logger.debug(QString("Subscribed to path: %1").arg(path));
}

void StateManager::subscribe(QObject* subscriber, StateObserver observer) {
    subscribe("*", subscriber, observer);
}

void StateManager::unsubscribe(const QString& path, QObject* subscriber) {
    QMutexLocker locker(&m_mutex);
    
    m_subscriptions.erase(
        std::remove_if(m_subscriptions.begin(), m_subscriptions.end(),
            [&path, subscriber](const Subscription& sub) {
                return sub.path == path && sub.subscriber == subscriber;
            }),
        m_subscriptions.end()
    );
}

void StateManager::unsubscribeAll(QObject* subscriber) {
    QMutexLocker locker(&m_mutex);

    m_subscriptions.erase(
        std::remove_if(m_subscriptions.begin(), m_subscriptions.end(),
            [subscriber](const Subscription& sub) {
                return sub.subscriber == subscriber;
            }),
        m_subscriptions.end()
    );
}

void StateManager::addMiddleware(StateMiddleware middleware) {
    QMutexLocker locker(&m_mutex);
    m_middleware.append(middleware);
    m_logger.debug("Middleware added");
}

void StateManager::removeMiddleware(StateMiddleware middleware) {
    // Note: Function comparison is not straightforward in C++
    // This is a simplified implementation
    m_logger.warning("removeMiddleware not fully implemented - function comparison limitation");
}

void StateManager::enableHistory(int maxSize) {
    QMutexLocker locker(&m_mutex);
    m_historyEnabled = true;
    m_maxHistorySize = maxSize;
    m_logger.debug(QString("History enabled with max size: %1").arg(maxSize));
}

void StateManager::disableHistory() {
    QMutexLocker locker(&m_mutex);
    m_historyEnabled = false;
    m_logger.debug("History disabled");
}

bool StateManager::canUndo() const {
    QMutexLocker locker(&m_mutex);
    return m_historyEnabled && m_historyIndex > 0;
}

bool StateManager::canRedo() const {
    QMutexLocker locker(&m_mutex);
    return m_historyEnabled && m_historyIndex < m_history.size() - 1;
}

void StateManager::undo() {
    QMutexLocker locker(&m_mutex);

    if (!canUndo()) {
        m_logger.warning("Cannot undo - no history available");
        return;
    }

    m_historyIndex--;
    const StateChange& change = m_history[m_historyIndex];
    m_currentState = change.oldState();

    locker.unlock();

    emit stateChanged(change);
    emit historyChanged();

    m_logger.debug("Undo performed");
}

void StateManager::redo() {
    QMutexLocker locker(&m_mutex);

    if (!canRedo()) {
        m_logger.warning("Cannot redo - no future history available");
        return;
    }

    m_historyIndex++;
    const StateChange& change = m_history[m_historyIndex];
    m_currentState = change.newState();

    locker.unlock();

    emit stateChanged(change);
    emit historyChanged();

    m_logger.debug("Redo performed");
}

void StateManager::clearHistory() {
    QMutexLocker locker(&m_mutex);
    m_history.clear();
    m_historyIndex = -1;

    locker.unlock();
    emit historyChanged();

    m_logger.debug("History cleared");
}

bool StateManager::saveState(const QString& filePath) {
    QMutexLocker locker(&m_mutex);
    State state = m_currentState;
    locker.unlock();

    QJsonDocument doc(state.toJson());
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly)) {
        m_logger.error(QString("Failed to open file for writing: %1").arg(filePath));
        return false;
    }

    file.write(doc.toJson());
    file.close();

    m_logger.info(QString("State saved to: %1").arg(filePath));
    return true;
}

bool StateManager::loadState(const QString& filePath) {
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        m_logger.error(QString("Failed to open file for reading: %1").arg(filePath));
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        m_logger.error("Invalid JSON in state file");
        return false;
    }

    State newState(doc.object());
    setState(newState, "Load from file");

    m_logger.info(QString("State loaded from: %1").arg(filePath));
    return true;
}

void StateManager::setAutoSave(bool enabled, int intervalMs) {
    if (enabled) {
        if (!m_autoSaveTimer) {
            m_autoSaveTimer = new QTimer(this);
            connect(m_autoSaveTimer, &QTimer::timeout, this, &StateManager::onAutoSaveTimeout);
        }
        m_autoSaveTimer->setInterval(intervalMs);
        m_autoSaveTimer->start();
        m_logger.debug(QString("Auto-save enabled with interval: %1ms").arg(intervalMs));
    } else {
        if (m_autoSaveTimer) {
            m_autoSaveTimer->stop();
        }
        m_logger.debug("Auto-save disabled");
    }
}

void StateManager::createSnapshot(const QString& name) {
    QMutexLocker locker(&m_mutex);
    m_snapshots[name] = m_currentState;
    locker.unlock();

    emit snapshotCreated(name);
    m_logger.debug(QString("Snapshot created: %1").arg(name));
}

bool StateManager::restoreSnapshot(const QString& name) {
    QMutexLocker locker(&m_mutex);

    if (!m_snapshots.contains(name)) {
        m_logger.warning(QString("Snapshot not found: %1").arg(name));
        return false;
    }

    State snapshot = m_snapshots[name];
    locker.unlock();

    setState(snapshot, QString("Restore snapshot: %1").arg(name));
    emit snapshotRestored(name);

    m_logger.debug(QString("Snapshot restored: %1").arg(name));
    return true;
}

QStringList StateManager::snapshots() const {
    QMutexLocker locker(&m_mutex);
    return m_snapshots.keys();
}

void StateManager::deleteSnapshot(const QString& name) {
    QMutexLocker locker(&m_mutex);
    m_snapshots.remove(name);
    m_logger.debug(QString("Snapshot deleted: %1").arg(name));
}

QString StateManager::stateReport() const {
    QMutexLocker locker(&m_mutex);

    QString report;
    report += "=== State Manager Report ===\n";
    report += QString("Current State Size: %1 keys\n").arg(m_currentState.toJson().keys().size());
    report += QString("Subscriptions: %1\n").arg(m_subscriptions.size());
    report += QString("Middleware: %1\n").arg(m_middleware.size());
    report += QString("History Enabled: %1\n").arg(m_historyEnabled ? "Yes" : "No");
    report += QString("History Size: %1\n").arg(m_history.size());
    report += QString("History Index: %1\n").arg(m_historyIndex);
    report += QString("Snapshots: %1\n").arg(m_snapshots.size());
    report += QString("Debug Mode: %1\n").arg(m_debugMode ? "Yes" : "No");

    return report;
}

void StateManager::onSubscriberDestroyed(QObject* obj) {
    unsubscribeAll(obj);
}

void StateManager::onAutoSaveTimeout() {
    if (!m_autoSavePath.isEmpty()) {
        saveState(m_autoSavePath);
    }
}

void StateManager::setState(const State& newState, const QString& reason) {
    QMutexLocker locker(&m_mutex);

    State oldState = m_currentState;

    // Apply middleware
    State processedState = applyMiddleware(oldState, newState);

    // Check if state actually changed
    if (processedState == oldState) {
        return;
    }

    emit beforeStateChange(oldState, processedState);

    m_currentState = processedState;

    // Add to history if enabled
    if (m_historyEnabled) {
        // Truncate future history if we're not at the end
        if (m_historyIndex < m_history.size() - 1) {
            m_history = m_history.mid(0, m_historyIndex + 1);
        }

        StateChange change(oldState, processedState, reason);
        m_history.append(change);
        m_historyIndex = m_history.size() - 1;

        // Limit history size
        if (m_history.size() > m_maxHistorySize) {
            m_history.removeFirst();
            m_historyIndex--;
        }

        emit historyChanged();
    }

    StateChange change(oldState, processedState, reason);

    locker.unlock();

    // Notify observers
    notifyObservers(change);

    // Emit signals
    emit stateChanged(change);

    if (m_debugMode) {
        m_logger.debug(QString("State changed: %1").arg(reason));
    }
}

void StateManager::notifyObservers(const StateChange& change) {
    QMutexLocker locker(&m_mutex);
    QList<Subscription> subs = m_subscriptions; // Copy to avoid issues
    locker.unlock();

    for (const Subscription& sub : subs) {
        if (!sub.subscriber || !sub.observer) {
            continue;
        }

        // Check if this subscription should be notified
        if (sub.path == "*" || change.hasChanged(sub.path)) {
            try {
                sub.observer(change);
            } catch (const std::exception& e) {
                m_logger.error(QString("Exception in state observer: %1").arg(e.what()));
            } catch (...) {
                m_logger.error("Unknown exception in state observer");
            }
        }
    }
}

State StateManager::applyMiddleware(const State& oldState, const State& newState) {
    State result = newState;

    for (const StateMiddleware& middleware : m_middleware) {
        try {
            result = middleware(oldState, result);
        } catch (const std::exception& e) {
            m_logger.error(QString("Exception in middleware: %1").arg(e.what()));
        } catch (...) {
            m_logger.error("Unknown exception in middleware");
        }
    }

    return result;
}

// ============================================================================
// StateStore Implementation
// ============================================================================

StateStore::StateStore(const State& initialState, QObject* parent)
    : QObject(parent)
    , m_state(initialState)
{}

void StateStore::addReducer(const QString& key, Reducer reducer) {
    m_reducers[key] = reducer;
}

void StateStore::removeReducer(const QString& key) {
    m_reducers.remove(key);
}

void StateStore::dispatch(const Action& action) {
    State newState = applyReducers(m_state, action);

    if (newState != m_state) {
        m_state = newState;
        emit stateChanged(m_state, action);
        notifyObservers(action);
    }

    emit actionDispatched(action);
}

void StateStore::dispatch(const QString& type, const QVariant& payload) {
    Action action;
    action.type = type;
    action.payload = payload;
    dispatch(action);
}

QVariant StateStore::get(const QString& path) const {
    return m_state.get(path);
}

void StateStore::subscribe(StoreObserver observer) {
    m_observers.append(observer);
}

void StateStore::unsubscribe(StoreObserver observer) {
    // Note: Function comparison is not straightforward in C++
    // This is a simplified implementation
}

State StateStore::applyReducers(const State& state, const Action& action) {
    State result = state;

    for (auto it = m_reducers.begin(); it != m_reducers.end(); ++it) {
        try {
            result = it.value()(result, action);
        } catch (const std::exception& e) {
            qWarning() << "Exception in reducer:" << it.key() << "-" << e.what();
        } catch (...) {
            qWarning() << "Unknown exception in reducer:" << it.key();
        }
    }

    return result;
}

void StateStore::notifyObservers(const Action& action) {
    for (const StoreObserver& observer : m_observers) {
        try {
            observer(m_state, action);
        } catch (const std::exception& e) {
            qWarning() << "Exception in store observer:" << e.what();
        } catch (...) {
            qWarning() << "Unknown exception in store observer";
        }
    }
}

