#pragma once

#include <QHash>
#include <QJsonObject>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariant>
#include <functional>
#include <memory>
#include "../logging/SimpleLogging.h"

/**
 * @brief State - Represents application state
 *
 * Immutable state object that holds application data.
 */
class State {
public:
    State() = default;
    explicit State(QJsonObject data);

    // State access
    [[nodiscard]] QVariant get(const QString& path) const;
    [[nodiscard]] QJsonObject getObject(const QString& path) const;
    [[nodiscard]] QJsonValue getValue(const QString& path) const;
    [[nodiscard]] bool has(const QString& path) const;

    // State manipulation (returns new state)
    State set(const QString& path, const QVariant& value);
    State merge(const QJsonObject& data);
    State remove(const QString& path);

    // Serialization
    [[nodiscard]] QJsonObject toJson() const { return m_data; }
    [[nodiscard]] QString toString() const;

    // Comparison
    bool operator==(const State& other) const;
    bool operator!=(const State& other) const;

private:
    QJsonObject m_data;

    // Helper methods
    [[nodiscard]] QJsonValue getValueByPath(const QJsonObject& obj,
                                            const QStringList& path) const;
    [[nodiscard]] QJsonObject setValueByPath(const QJsonObject& obj,
                                             const QStringList& path,
                                             const QJsonValue& value) const;
    [[nodiscard]] QJsonObject removeValueByPath(const QJsonObject& obj,
                                                const QStringList& path) const;
};

/**
 * @brief StateChange - Represents a change in state
 */
class StateChange {
public:
    StateChange(const State& oldState, const State& newState,
                const QString& reason = QString());

    [[nodiscard]] State oldState() const { return m_oldState; }
    [[nodiscard]] State newState() const { return m_newState; }
    [[nodiscard]] QString reason() const { return m_reason; }
    [[nodiscard]] qint64 timestamp() const { return m_timestamp; }

    // Change analysis
    [[nodiscard]] QStringList changedPaths() const;
    [[nodiscard]] bool hasChanged(const QString& path) const;
    [[nodiscard]] QVariant oldValue(const QString& path) const;
    [[nodiscard]] QVariant newValue(const QString& path) const;

private:
    State m_oldState;
    State m_newState;
    QString m_reason;
    qint64 m_timestamp;
};

/**
 * @brief StateManager - Centralized state management
 *
 * Manages application state with support for observers, middleware,
 * and time-travel debugging.
 */
class StateManager : public QObject {
    Q_OBJECT

public:
    ~StateManager() override;

    // Singleton access
    static StateManager& instance();

    // State access
    State currentState() const;
    QVariant get(const QString& path) const;
    bool has(const QString& path) const;

    // State mutations
    void set(const QString& path, const QVariant& value,
             const QString& reason = QString());
    void merge(const QJsonObject& data, const QString& reason = QString());
    void remove(const QString& path, const QString& reason = QString());
    void reset(const State& newState = State(),
               const QString& reason = "Reset");

    // Observers
    using StateObserver = std::function<void(const StateChange&)>;
    void subscribe(const QString& path, QObject* subscriber,
                   StateObserver observer);
    void subscribe(QObject* subscriber, StateObserver observer);
    void unsubscribe(const QString& path, QObject* subscriber);
    void unsubscribeAll(QObject* subscriber);

    // Middleware
    using StateMiddleware = std::function<State(const State&, const State&)>;
    void addMiddleware(StateMiddleware middleware);
    void removeMiddleware(StateMiddleware middleware);

    // History and time-travel
    void enableHistory(int maxSize = 100);
    void disableHistory();
    bool canUndo() const;
    bool canRedo() const;
    void undo();
    void redo();
    QList<StateChange> history() const { return m_history; }
    void clearHistory();

    // Persistence
    bool saveState(const QString& filePath);
    bool loadState(const QString& filePath);
    void setAutoSave(bool enabled, int intervalMs = 5000);

    // State snapshots
    void createSnapshot(const QString& name);
    bool restoreSnapshot(const QString& name);
    QStringList snapshots() const;
    void deleteSnapshot(const QString& name);

    // Debugging
    void enableDebugMode(bool enabled) { m_debugMode = enabled; }
    bool isDebugMode() const { return m_debugMode; }
    QString stateReport() const;

signals:
    void stateChanged(const StateChange& change);
    void stateChanged(const QString& path, const QVariant& oldValue,
                      const QVariant& newValue);
    void beforeStateChange(const State& currentState, const State& newState);
    void historyChanged();
    void snapshotCreated(const QString& name);
    void snapshotRestored(const QString& name);

private slots:
    void onSubscriberDestroyed(QObject* obj);
    void onAutoSaveTimeout();

private:
    explicit StateManager(QObject* parent = nullptr);

public:
    StateManager(const StateManager&) = delete;
    StateManager& operator=(const StateManager&) = delete;
    StateManager(StateManager&&) = delete;
    StateManager& operator=(StateManager&&) = delete;

private:
    // State mutation helper
    void setState(const State& newState, const QString& reason);
    void notifyObservers(const StateChange& change);
    State applyMiddleware(const State& oldState, const State& newState);

    // Current state
    State m_currentState;

    // Observers
    struct Subscription {
        QString path;
        QObject* subscriber;
        StateObserver observer;
    };
    QList<Subscription> m_subscriptions;

    // Middleware
    QList<StateMiddleware> m_middleware;

    // History
    QList<StateChange> m_history;
    int m_historyIndex = -1;
    int m_maxHistorySize = 100;
    bool m_historyEnabled = false;

    // Snapshots
    QHash<QString, State> m_snapshots;

    // Auto-save
    QTimer* m_autoSaveTimer = nullptr;
    QString m_autoSavePath;

    // Thread safety
    mutable QMutex m_mutex;

    // Debug mode
    bool m_debugMode = false;

    // Logging
    SastLogging::CategoryLogger m_logger{"StateManager"};
};

/**
 * @brief StateStore - Store with reducers for state management
 *
 * Redux-like store pattern for more complex state management.
 */
class StateStore : public QObject {
    Q_OBJECT

public:
    explicit StateStore(const State& initialState = State(),
                        QObject* parent = nullptr);

    // Actions
    struct Action {
        QString type;
        QVariant payload;
        QVariantMap metadata;
    };

    // Reducers
    using Reducer = std::function<State(const State&, const Action&)>;
    void addReducer(const QString& key, Reducer reducer);
    void removeReducer(const QString& key);

    // Dispatch actions
    void dispatch(const Action& action);
    void dispatch(const QString& type, const QVariant& payload = QVariant());

    // State access
    [[nodiscard]] State state() const { return m_state; }
    [[nodiscard]] QVariant get(const QString& path) const;

    // Subscriptions
    using StoreObserver = std::function<void(const State&, const Action&)>;
    void subscribe(StoreObserver observer);
    void unsubscribe(StoreObserver observer);

signals:
    void stateChanged(const State& state, const Action& action);
    void actionDispatched(const Action& action);

private:
    State m_state;
    QHash<QString, Reducer> m_reducers;
    QList<StoreObserver> m_observers;

    State applyReducers(const State& state, const Action& action);
    void notifyObservers(const Action& action);
};

/**
 * @brief StateSelector - Efficient state selection with memoization
 */
template <typename T>
class StateSelector {
public:
    using SelectorFunc = std::function<T(const State&)>;

    explicit StateSelector(SelectorFunc selector) : m_selector(selector) {}

    T select(const State& state) {
        if (!m_cached || m_lastState != state) {
            m_cachedValue = m_selector(state);
            m_lastState = state;
            m_cached = true;
        }
        return m_cachedValue;
    }

    void invalidate() { m_cached = false; }

private:
    SelectorFunc m_selector;
    State m_lastState;
    T m_cachedValue;
    bool m_cached = false;
};

// Convenience macros
#define STATE StateManager::instance().currentState()
#define STATE_GET(path) StateManager::instance().get(path)
#define STATE_SET(path, value) StateManager::instance().set(path, value)
#define STATE_SUBSCRIBE(path, handler) \
    StateManager::instance().subscribe(path, this, handler)
