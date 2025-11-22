#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <functional>

/**
 * @brief Plugin Hook Point
 *
 * Represents a hook point in the application where plugins can register
 * callbacks.
 */
class PluginHookPoint : public QObject {
    Q_OBJECT

public:
    explicit PluginHookPoint(const QString& name, QObject* parent = nullptr)
        : QObject(parent), m_name(name) {}

    QString name() const { return m_name; }

    using HookCallback = std::function<QVariant(const QVariantMap&)>;

    void registerCallback(const QString& pluginName, HookCallback callback) {
        m_callbacks[pluginName] = callback;
    }

    void unregisterCallback(const QString& pluginName) {
        m_callbacks.remove(pluginName);
    }

    QVariant execute(const QVariantMap& context) const {
        QVariantList results;
        for (const auto& callback : m_callbacks) {
            QVariant result = callback(context);
            if (result.isValid()) {
                results.append(result);
            }
        }
        return results;
    }

    int callbackCount() const { return m_callbacks.size(); }

private:
    QString m_name;
    QHash<QString, HookCallback> m_callbacks;
};
