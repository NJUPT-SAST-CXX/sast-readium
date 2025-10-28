#pragma once

#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMainWindow>
#include <QMutex>
#include <QObject>
#include <QSettings>
#include <QSplitter>
#include <QString>
#include <QTimer>
#include <QVariant>
#include <QWidget>
#include <functional>
#include "../../logging/SimpleLogging.h"

/**
 * @brief Comprehensive UI state management system
 *
 * Manages application-wide UI state including window geometry, splitter
 * positions, widget visibility, user preferences, and component states.
 * Provides automatic state persistence and restoration for a consistent user
 * experience.
 *
 * Features:
 * - Automatic state saving and restoration
 * - Component state synchronization
 * - Memory-efficient state storage
 * - Error recovery for corrupted state
 * - Thread-safe state operations
 */
class UIStateManager : public QObject {
    Q_OBJECT
    friend class ComponentStateGuard;

public:
    enum class StateScope {
        Session,   // Temporary state for current session
        User,      // Persistent user preferences
        Global,    // Application-wide settings
        Component  // Component-specific state
    };

    enum class StatePriority {
        Low,      // Optional state, can be lost
        Normal,   // Standard state persistence
        High,     // Critical state, must be preserved
        Critical  // Essential state, backup on failure
    };

    struct StateInfo {
        QString key;
        QVariant value;
        StateScope scope;
        StatePriority priority;
        QDateTime lastModified;
        QString component;

        StateInfo() = default;
        StateInfo(const QString& k, const QVariant& v,
                  StateScope s = StateScope::User,
                  StatePriority p = StatePriority::Normal,
                  const QString& comp = QString())
            : key(k),
              value(v),
              scope(s),
              priority(p),
              lastModified(QDateTime::currentDateTime()),
              component(comp) {}
    };

    static UIStateManager& instance();

    // State management
    void setState(const QString& key, const QVariant& value,
                  StateScope scope = StateScope::User,
                  StatePriority priority = StatePriority::Normal,
                  const QString& component = QString());
    QVariant getState(const QString& key,
                      const QVariant& defaultValue = QVariant(),
                      StateScope scope = StateScope::User) const;
    bool hasState(const QString& key,
                  StateScope scope = StateScope::User) const;
    void removeState(const QString& key, StateScope scope = StateScope::User);
    void clearScope(StateScope scope);

    // Component state management
    void registerComponent(QWidget* widget, const QString& componentId);
    void unregisterComponent(QWidget* widget);
    void saveComponentState(QWidget* widget);
    void restoreComponentState(QWidget* widget);
    void saveAllComponentStates();
    void restoreAllComponentStates();

    // Window state management
    void saveWindowState(QMainWindow* window);
    void restoreWindowState(QMainWindow* window);
    void saveGeometry(QWidget* widget, const QString& key = QString());
    void restoreGeometry(QWidget* widget, const QString& key = QString());

    // Splitter state management
    void saveSplitterState(QSplitter* splitter, const QString& key = QString());
    void restoreSplitterState(QSplitter* splitter,
                              const QString& key = QString());

    // Batch operations
    void beginBatchUpdate();
    void endBatchUpdate();
    void saveAllStates();
    void restoreAllStates();

    // State synchronization
    void synchronizeState(const QString& key, StateScope fromScope,
                          StateScope toScope);
    void synchronizeComponent(const QString& componentId);

    // Persistence control
    void enableAutosave(bool enabled, int intervalMs = 30000);
    void forceSave();
    void forceRestore();

    // State validation and recovery
    bool validateState(const QString& key, StateScope scope = StateScope::User);
    void repairCorruptedState();
    void createStateBackup();
    void restoreFromBackup();

    // Configuration
    void setStateFile(const QString& filePath);
    void setCompressionEnabled(bool enabled) { m_compressionEnabled = enabled; }
    void setEncryptionEnabled(bool enabled) { m_encryptionEnabled = enabled; }
    void setMaxStateAge(int days) { m_maxStateAgeDays = days; }

    // State monitoring
    QStringList getStateKeys(StateScope scope = StateScope::User) const;
    QJsonObject exportState(StateScope scope = StateScope::User) const;
    void importState(const QJsonObject& stateData,
                     StateScope scope = StateScope::User);

    // Memory management
    void cleanupExpiredStates();
    void optimizeStateStorage();
    qint64 getStateStorageSize() const;

signals:
    void stateChanged(const QString& key, const QVariant& value,
                      StateScope scope);
    void componentStateChanged(const QString& componentId);
    void stateSaved(StateScope scope, int itemCount);
    void stateRestored(StateScope scope, int itemCount);
    void stateError(const QString& operation, const QString& error);

private slots:
    void onAutosaveTimer();
    void onComponentDestroyed(QObject* object);

private:
    UIStateManager();
    ~UIStateManager() override;
    UIStateManager(const UIStateManager&) = delete;
    UIStateManager& operator=(const UIStateManager&) = delete;

    // Internal state management
    QString generateStateKey(const QString& key, StateScope scope) const;
    QString getScopePrefix(StateScope scope) const;
    void saveStateToFile();
    void loadStateFromFile();

    // Component state helpers
    QVariantMap captureWidgetState(QWidget* widget);
    void applyWidgetState(QWidget* widget, const QVariantMap& state);
    QString getWidgetStateKey(QWidget* widget) const;

    // Persistence helpers
    QByteArray serializeState(const QVariantMap& state);
    QVariantMap deserializeState(const QByteArray& data);
    void ensureStateDirectory();

    // Validation helpers
    bool isValidStateValue(const QVariant& value);
    void sanitizeStateValue(QVariant& value);

    // Data members
    QHash<QString, StateInfo> m_states;
    QHash<QWidget*, QString> m_registeredComponents;
    QHash<QString, QVariantMap> m_componentStates;

    QString m_stateFilePath;
    QTimer* m_autosaveTimer;
    bool m_autosaveEnabled;
    bool m_batchUpdateMode;
    bool m_compressionEnabled;
    bool m_encryptionEnabled;
    int m_maxStateAgeDays;

    mutable QMutex m_stateMutex;
    SastLogging::CategoryLogger m_logger;
};

/**
 * @brief RAII helper for component state management
 */
class ComponentStateGuard {
public:
    explicit ComponentStateGuard(QWidget* widget,
                                 const QString& componentId = QString());
    ~ComponentStateGuard();

    void commit();    // Save current state
    void rollback();  // Restore original state

private:
    QWidget* m_widget;
    QString m_componentId;
    QVariantMap m_originalState;
    bool m_committed;
};

/**
 * @brief State binding helper for automatic state synchronization
 */
class StateBinding : public QObject {
    Q_OBJECT

public:
    StateBinding(QWidget* widget, const QString& stateKey,
                 const QString& property = QString(),
                 QObject* parent = nullptr);

    void setTwoWay(bool enabled) { m_twoWay = enabled; }
    void setTransform(std::function<QVariant(const QVariant&)> toWidget,
                      std::function<QVariant(const QVariant&)> fromWidget);

private slots:
    void onStateChanged(const QString& key, const QVariant& value,
                        UIStateManager::StateScope scope);
    void onWidgetPropertyChanged();

private:
    QWidget* m_widget;
    QString m_stateKey;
    QString m_property;
    bool m_twoWay;
    std::function<QVariant(const QVariant&)> m_toWidgetTransform;
    std::function<QVariant(const QVariant&)> m_fromWidgetTransform;
};

// Convenience macros
#define UI_STATE_MANAGER UIStateManager::instance()

#define SAVE_COMPONENT_STATE(widget) UI_STATE_MANAGER.saveComponentState(widget)

#define RESTORE_COMPONENT_STATE(widget) \
    UI_STATE_MANAGER.restoreComponentState(widget)

#define SET_UI_STATE(key, value) UI_STATE_MANAGER.setState(key, value)

#define GET_UI_STATE(key, defaultValue) \
    UI_STATE_MANAGER.getState(key, defaultValue)

#define REGISTER_UI_COMPONENT(widget, id) \
    UI_STATE_MANAGER.registerComponent(widget, id)

#define STATE_GUARD(widget, id) ComponentStateGuard guard(widget, id)

#include "UIStateManager.h"
