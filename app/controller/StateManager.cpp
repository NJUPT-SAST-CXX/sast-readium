#include "StateManager.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTimer>

// ============================================================================
// State Implementation
// ============================================================================

State::State(QJsonObject data) : m_data(std::move(data)) {}

QVariant State::get(const QString& path) const {
    QJsonValue value = getValue(path);
    return value.toVariant();
}

QJsonObject State::getObject(const QString& path) const {
    QJsonValue value = getValue(path);
    return value.toObject();
}

QJsonValue State::getValue(const QString& path) const {
    // Handle empty or invalid paths
    if (path.isEmpty() || path == "." || path == "..") {
        return QJsonValue();
    }

    QStringList parts = path.split('.', Qt::SkipEmptyParts);
    return getValueByPath(m_data, parts);
}

bool State::has(const QString& path) const {
    QStringList parts = path.split('.', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return false;
    }
    QJsonValue value = getValueByPath(m_data, parts);
    return !value.isUndefined() && !value.isNull();
}

State State::set(const QString& path, const QVariant& value) {
    // Reject empty or invalid paths
    if (path.isEmpty() || path == "." || path == "..") {
        return *this;
    }

    QStringList parts = path.split('.', Qt::SkipEmptyParts);
    QJsonValue jsonValue = QJsonValue::fromVariant(value);
    m_data = setValueByPath(m_data, parts, jsonValue);
    return *this;
}

State State::merge(const QJsonObject& data) {
    for (auto it = data.begin(); it != data.end(); ++it) {
        m_data[it.key()] = it.value();
    }
    return *this;
}

State State::remove(const QString& path) {
    QStringList parts = path.split('.', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return *this;
    }

    m_data = removeValueByPath(m_data, parts);
    return *this;
}

QString State::toString() const {
    QJsonDocument doc(m_data);
    return doc.toJson(QJsonDocument::Compact);
}

bool State::operator==(const State& other) const {
    return m_data == other.m_data;
}

bool State::operator!=(const State& other) const { return !(*this == other); }

QJsonValue State::getValueByPath(const QJsonObject& obj,
                                 const QStringList& path) const {
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

QJsonObject State::setValueByPath(const QJsonObject& obj,
                                  const QStringList& path,
                                  const QJsonValue& value) const {
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

QJsonObject State::removeValueByPath(const QJsonObject& obj,
                                     const QStringList& path) const {
    if (path.isEmpty()) {
        return obj;
    }

    QJsonObject result = obj;
    const QString& key = path.first();

    if (path.size() == 1) {
        result.remove(key);
        return result;
    }

    if (!result.contains(key) || !result.value(key).isObject()) {
        return result;
    }

    QJsonObject child = result.value(key).toObject();
    child = removeValueByPath(child, path.mid(1));

    if (child.isEmpty()) {
        result.remove(key);
    } else {
        result.insert(key, child);
    }

    return result;
}

// ============================================================================
// StateChange Implementation
// ============================================================================

StateChange::StateChange(const State& oldState, const State& newState,
                         const QString& reason)
    : m_oldState(oldState),
      m_newState(newState),
      m_reason(reason),
      m_timestamp(QDateTime::currentMSecsSinceEpoch()) {}

QStringList StateChange::changedPaths() const {
    QStringList paths;

    // Fast-path detection based on reason to avoid full JSON diff
    // Common cases: single-path set/remove operations
    if (!m_reason.isEmpty()) {
        if (m_reason.startsWith("Set ")) {
            QString p = m_reason.mid(4);
            if (!p.isEmpty()) {
                paths.append(p);
                return paths;
            }
        } else if (m_reason.startsWith("Remove ")) {
            QString p = m_reason.mid(7);
            if (!p.isEmpty()) {
                paths.append(p);
                return paths;
            }
        }
    }

    // Helper function for recursive comparison
    std::function<void(const QJsonValue&, const QJsonValue&, const QString&)>
        compareValues;

    compareValues = [&](const QJsonValue& value1, const QJsonValue& value2,
                        const QString& currentPath) {
        const auto type1 = value1.type();
        const auto type2 = value2.type();

        const bool isObject1 = type1 == QJsonValue::Object;
        const bool isObject2 = type2 == QJsonValue::Object;
        if (isObject1 || isObject2) {
            QJsonObject obj1 = value1.toObject();
            QJsonObject obj2 = value2.toObject();

            QSet<QString> allKeys;
            for (const QString& key : obj1.keys()) {
                allKeys.insert(key);
            }
            for (const QString& key : obj2.keys()) {
                allKeys.insert(key);
            }

            for (const QString& key : allKeys) {
                const QString newPath =
                    currentPath.isEmpty() ? key : currentPath + "." + key;
                compareValues(obj1.value(key), obj2.value(key), newPath);
            }

            if (type1 != type2 && !currentPath.isEmpty() && allKeys.isEmpty()) {
                paths.append(currentPath);
            }
            return;
        }

        const bool isArray1 = type1 == QJsonValue::Array;
        const bool isArray2 = type2 == QJsonValue::Array;
        if (isArray1 || isArray2) {
            QJsonArray arr1 = value1.toArray();
            QJsonArray arr2 = value2.toArray();

            if (arr1.size() != arr2.size() && !currentPath.isEmpty()) {
                paths.append(currentPath);
            }

            int maxSize = arr1.size();
            if (arr2.size() > maxSize) {
                maxSize = arr2.size();
            }

            auto jsonAt = [](const QJsonArray& array, int index) -> QJsonValue {
                if (index >= 0 && index < array.size()) {
                    return array.at(index);
                }
                return QJsonValue();
            };

            for (int i = 0; i < maxSize; ++i) {
                QString indexPath =
                    currentPath.isEmpty()
                        ? QStringLiteral("[%1]").arg(i)
                        : QStringLiteral("%1[%2]").arg(currentPath).arg(i);
                compareValues(jsonAt(arr1, i), jsonAt(arr2, i), indexPath);
            }
            return;
        }

        if ((type1 != type2 || value1 != value2) && !currentPath.isEmpty()) {
            paths.append(currentPath);
        }
    };

    // Start comparison from root
    compareValues(m_oldState.toJson(), m_newState.toJson(), "");

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
    : QObject(parent), m_logger("StateManager") {
    m_logger.debug("StateManager initialized");
}

StateManager::~StateManager() {
    // During static destruction, QCoreApplication might already be destroyed
    // In that case, we can't safely do anything, so just return
    if (!QCoreApplication::instance()) {
        return;
    }

    if (m_autoSaveTimer) {
        m_autoSaveTimer->stop();
        delete m_autoSaveTimer;
    }
}

StateManager& StateManager::instance() {
    static StateManager instance;
    return instance;
}

void StateManager::enableDebugMode(bool enabled) {
    m_debugMode = enabled;
    // Elevate logging verbosity when debug mode is on to surface diagnostics
    m_logger.setLevel(enabled ? SastLogging::Level::Debug
                              : SastLogging::Level::Info);
    if (enabled) {
        m_logger.debug("Debug mode enabled");
    }
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

void StateManager::set(const QString& path, const QVariant& value,
                       const QString& reason) {
    State newState;
    {
        QMutexLocker locker(&m_mutex);
        newState = m_currentState;
    }

    newState.set(path, value);
    setState(newState, reason.isEmpty() ? QString("Set %1").arg(path) : reason);
}

void StateManager::merge(const QJsonObject& data, const QString& reason) {
    State newState;
    {
        QMutexLocker locker(&m_mutex);
        newState = m_currentState;
    }

    newState.merge(data);
    setState(newState, reason.isEmpty() ? "Merge" : reason);
}

void StateManager::remove(const QString& path, const QString& reason) {
    State newState;
    {
        QMutexLocker locker(&m_mutex);
        newState = m_currentState;
    }

    newState.remove(path);
    setState(newState,
             reason.isEmpty() ? QString("Remove %1").arg(path) : reason);
}

void StateManager::reset(const State& newState, const QString& reason) {
    setState(newState, reason);
}

void StateManager::subscribe(const QString& path, QObject* subscriber,
                             StateObserver observer) {
    QMutexLocker locker(&m_mutex);

    Subscription sub;
    sub.path = path;
    sub.subscriber = subscriber;
    sub.observer = observer;

    m_subscriptions.append(sub);

    // Connect to destroyed signal to auto-cleanup
    connect(subscriber, &QObject::destroyed, this,
            &StateManager::onSubscriberDestroyed, Qt::UniqueConnection);

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
                           return sub.path == path &&
                                  sub.subscriber == subscriber;
                       }),
        m_subscriptions.end());
}

void StateManager::unsubscribeAll(QObject* subscriber) {
    QMutexLocker locker(&m_mutex);

    m_subscriptions.erase(
        std::remove_if(m_subscriptions.begin(), m_subscriptions.end(),
                       [subscriber](const Subscription& sub) {
                           return sub.subscriber == subscriber;
                       }),
        m_subscriptions.end());
}

void StateManager::addMiddleware(StateMiddleware middleware) {
    QMutexLocker locker(&m_mutex);
    m_middleware.append(middleware);
    m_logger.debug("Middleware added");
}

void StateManager::removeMiddleware(StateMiddleware middleware) {
    // Note: Function comparison is not straightforward in C++
    // This is a simplified implementation
    m_logger.warning(
        "removeMiddleware not fully implemented - function comparison "
        "limitation");
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
    return m_historyEnabled && m_historyIndex >= 0;
}

bool StateManager::canRedo() const {
    QMutexLocker locker(&m_mutex);
    return m_historyEnabled && m_historyIndex < m_history.size() - 1;
}

void StateManager::undo() {
    QMutexLocker locker(&m_mutex);

    if (!m_historyEnabled || m_historyIndex < 0) {
        m_logger.warning("Cannot undo - no history available");
        return;
    }

    const StateChange& change = m_history[m_historyIndex];
    m_historyIndex--;
    m_currentState = change.oldState();

    locker.unlock();

    emit stateChanged(change);
    emit historyChanged();

    m_logger.debug("Undo performed");
}

void StateManager::redo() {
    QMutexLocker locker(&m_mutex);

    if (!m_historyEnabled || m_historyIndex >= m_history.size() - 1) {
        m_logger.warning("Cannot redo - no future history available");
        return;
    }

    const StateChange& change = m_history[m_historyIndex + 1];
    m_historyIndex++;
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
        m_logger.error(
            QString("Failed to open file for writing: %1").arg(filePath));
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
        m_logger.error(
            QString("Failed to open file for reading: %1").arg(filePath));
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
            connect(m_autoSaveTimer, &QTimer::timeout, this,
                    &StateManager::onAutoSaveTimeout);
        }
        m_autoSaveTimer->setInterval(intervalMs);
        m_autoSaveTimer->start();
        m_logger.debug(
            QString("Auto-save enabled with interval: %1ms").arg(intervalMs));
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
    report += QString("Current State Size: %1 keys\n")
                  .arg(m_currentState.toJson().keys().size());
    report += QString("Subscriptions: %1\n").arg(m_subscriptions.size());
    report += QString("Middleware: %1\n").arg(m_middleware.size());
    report +=
        QString("History Enabled: %1\n").arg(m_historyEnabled ? "Yes" : "No");
    report += QString("History Size: %1\n").arg(m_history.size());
    report += QString("History Index: %1\n").arg(m_historyIndex);
    report += QString("Snapshots: %1\n").arg(m_snapshots.size());
    report += QString("Debug Mode: %1\n").arg(m_debugMode ? "Yes" : "No");

    return report;
}

void StateManager::onSubscriberDestroyed(QObject* obj) { unsubscribeAll(obj); }

void StateManager::onAutoSaveTimeout() {
    if (!m_autoSavePath.isEmpty()) {
        saveState(m_autoSavePath);
    }
}

void StateManager::setState(const State& newState, const QString& reason) {
    QMutexLocker locker(&m_mutex);

    State oldState = m_currentState;
    if (m_debugMode) {
        qDebug() << "StateManager::setState begin" << reason;
    }

    // Apply middleware
    State processedState = applyMiddleware(oldState, newState);

    // Check if state actually changed
    if (processedState == oldState) {
        return;
    }

    if (m_debugMode) {
        qDebug() << "StateManager::setState beforeStateChange";
    }
    emit beforeStateChange(oldState, processedState);

    m_currentState = processedState;

    // Prepare change for notifications
    StateChange change(oldState, processedState, reason);

    // Maintain history only when enabled
    if (m_historyEnabled) {
        if (m_historyIndex < m_history.size() - 1) {
            m_history = m_history.mid(0, m_historyIndex + 1);
        }

        m_history.append(change);
        m_historyIndex = static_cast<int>(m_history.size()) - 1;

        if (m_history.size() > m_maxHistorySize) {
            m_history.removeFirst();
            m_historyIndex = static_cast<int>(m_history.size()) - 1;
        }
    } else {
        // When history is disabled, keep indices consistent with an empty
        // history
        m_historyIndex = -1;
    }

    bool shouldEmitHistoryChanged = m_historyEnabled;

    locker.unlock();

    if (shouldEmitHistoryChanged) {
        emit historyChanged();
    }

    if (m_debugMode) {
        qDebug() << "StateManager::setState notifyObservers";
    }
    // Notify observers
    notifyObservers(change);

    // Emit fine-grained stateChanged signals. Prefer single-path fast path
    // based on reason to avoid expensive diff when possible.
    auto extractSinglePathFromReason = [](const QString& reason) -> QString {
        if (reason.startsWith("Set ")) {
            return reason.mid(4);
        }
        if (reason.startsWith("Remove ")) {
            return reason.mid(7);
        }
        return QString();
    };
    const QString singlePath = extractSinglePathFromReason(reason);
    if (!singlePath.isEmpty()) {
        emit stateChanged(singlePath, change.oldValue(singlePath),
                          change.newValue(singlePath));
    } else {
        for (const QString& changedPath : change.changedPaths()) {
            if (changedPath.isEmpty()) {
                continue;
            }
            emit stateChanged(changedPath, change.oldValue(changedPath),
                              change.newValue(changedPath));
        }
    }

    // Emit signals
    emit stateChanged(change);

    if (m_debugMode) {
        m_logger.debug(QString("State changed: %1").arg(reason));
        qDebug() << "StateManager::setState end";
    }
}

void StateManager::notifyObservers(const StateChange& change) {
    QMutexLocker locker(&m_mutex);
    QList<Subscription> subs = m_subscriptions;  // Copy to avoid issues
    locker.unlock();

    if (m_debugMode) {
        qDebug() << "StateManager::notifyObservers begin";
    }
    // Try to avoid heavy diff computation when possible by extracting
    // the single changed path from the reason (common Set/Remove cases).
    auto extractSinglePathFromReason = [](const QString& reason) -> QString {
        if (reason.startsWith("Set ")) {
            return reason.mid(4);
        }
        if (reason.startsWith("Remove ")) {
            return reason.mid(7);
        }
        return QString();
    };

    const QString singlePath = extractSinglePathFromReason(change.reason());

    // Only compute full changed paths if really necessary (e.g., for wildcard
    // subscriptions and when we don't have a singlePath optimization).
    QStringList changedPaths;
    bool needsFullPaths = false;
    for (const auto& sub : subs) {
        if (!sub.path.isEmpty() && sub.path.contains('*')) {
            needsFullPaths = true;
            break;
        }
    }
    if (needsFullPaths && singlePath.isEmpty()) {
        changedPaths = change.changedPaths();
    }
    // Diagnostic logging to help identify crashes in observer notification
    if (m_debugMode) {
        m_logger.debug(
            QString(
                "notifyObservers: subs=%1, hasSinglePath=%2, changedPaths=%3")
                .arg(subs.size())
                .arg(!singlePath.isEmpty())
                .arg(changedPaths.size()));
        qDebug() << "StateManager::notifyObservers subs" << subs.size()
                 << "hasSinglePath" << !singlePath.isEmpty()
                 << "changedPaths.size" << changedPaths.size();
    }

    auto patternMatches = [](const QString& pattern,
                             const QString& candidate) -> bool {
        if (pattern.isEmpty() || pattern == "*") {
            return true;
        }

        if (!pattern.contains('*')) {
            return candidate == pattern;
        }

        if (pattern.endsWith(".*")) {
            QString prefix = pattern.left(pattern.length() - 2);
            return candidate.startsWith(prefix);
        }

        qsizetype starIndex = pattern.indexOf('*');
        QString prefix = pattern.left(starIndex);
        QString suffix = pattern.mid(starIndex + 1);
        bool prefixMatches = candidate.startsWith(prefix);
        bool suffixMatches = suffix.isEmpty() || candidate.endsWith(suffix);
        return prefixMatches && suffixMatches;
    };

    int idx = 0;
    for (const Subscription& sub : subs) {
        if (sub.subscriber == nullptr || !sub.observer) {
            continue;
        }

        bool shouldNotify = false;

        if (sub.path.isEmpty() || sub.path == "*") {
            shouldNotify = true;
        } else if (sub.path.contains('*')) {
            if (!singlePath.isEmpty()) {
                shouldNotify = patternMatches(sub.path, singlePath);
            } else {
                for (const QString& path : changedPaths) {
                    if (patternMatches(sub.path, path)) {
                        shouldNotify = true;
                        break;
                    }
                }
            }
        } else if (change.hasChanged(sub.path)) {
            shouldNotify = true;
        }

        if (!shouldNotify) {
            continue;
        }
        if (m_debugMode) {
            m_logger.debug(
                QString("notifyObservers: calling observer #%1 for path '%2'")
                    .arg(idx)
                    .arg(sub.path));
            qDebug() << "StateManager::notifyObservers calling observer" << idx
                     << "path" << sub.path;
        }

        // Extra defensive guard: avoid invoking default-constructed or
        // moved-from functor
        if (!sub.observer) {
            continue;
        }

        try {
            sub.observer(change);
        } catch (const std::exception& e) {
            m_logger.error(
                QString("Exception in state observer: %1").arg(e.what()));
        } catch (...) {
            m_logger.error("Unknown exception in state observer");
        }

        if (m_debugMode) {
            m_logger.debug(
                QString("notifyObservers: observer #%1 completed").arg(idx));
            qDebug() << "StateManager::notifyObservers observer completed"
                     << idx;
        }
        ++idx;
    }
    if (m_debugMode) {
        qDebug() << "StateManager::notifyObservers end";
    }
}

State StateManager::applyMiddleware(const State& oldState,
                                    const State& newState) {
    State result = newState;

    for (const StateMiddleware& middleware : m_middleware) {
        try {
            result = middleware(oldState, result);
        } catch (const std::exception& e) {
            m_logger.error(
                QString("Exception in middleware: %1").arg(e.what()));
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
    : QObject(parent), m_state(initialState) {}

void StateStore::addReducer(const QString& key, Reducer reducer) {
    m_reducers[key] = reducer;
}

void StateStore::removeReducer(const QString& key) { m_reducers.remove(key); }

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
            qWarning() << "Exception in reducer:" << it.key() << "-"
                       << e.what();
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
