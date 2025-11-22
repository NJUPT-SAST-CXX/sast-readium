#include "PluginHookRegistry.h"
#include "../logging/LoggingMacros.h"

PluginHookRegistry* PluginHookRegistry::s_instance = nullptr;

PluginHookRegistry& PluginHookRegistry::instance() {
    if (!s_instance) {
        s_instance = new PluginHookRegistry(nullptr);
    }
    return *s_instance;
}

PluginHookRegistry::PluginHookRegistry(QObject* parent) : QObject(parent) {
    LOG_DEBUG("PluginHookRegistry: Initializing hook registry");
}

bool PluginHookRegistry::registerHook(const QString& hookName,
                                      const QString& description) {
    if (hookName.isEmpty()) {
        LOG_WARNING(
            "PluginHookRegistry: Attempt to register hook with empty name");
        return false;
    }

    if (m_hooks.contains(hookName)) {
        LOG_WARNING("PluginHookRegistry: Hook '{}' already registered",
                    hookName.toStdString());
        return false;
    }

    HookMetadata metadata;
    metadata.name = hookName;
    metadata.description = description.isEmpty()
                               ? QString("Hook point: %1").arg(hookName)
                               : description;
    metadata.enabled = true;
    metadata.hookPoint = std::make_unique<PluginHookPoint>(hookName);

    m_hooks[hookName] = std::move(metadata);

    LOG_INFO("PluginHookRegistry: Registered hook '{}'",
             hookName.toStdString());
    emit hookRegistered(hookName);

    return true;
}

void PluginHookRegistry::unregisterHook(const QString& hookName) {
    if (!m_hooks.contains(hookName)) {
        LOG_WARNING("PluginHookRegistry: Hook '{}' not found for unregister",
                    hookName.toStdString());
        return;
    }

    m_hooks.remove(hookName);
    LOG_INFO("PluginHookRegistry: Unregistered hook '{}'",
             hookName.toStdString());
    emit hookUnregistered(hookName);
}

bool PluginHookRegistry::hasHook(const QString& hookName) const {
    return m_hooks.contains(hookName);
}

QStringList PluginHookRegistry::getHookNames() const { return m_hooks.keys(); }

QString PluginHookRegistry::getHookDescription(const QString& hookName) const {
    if (!m_hooks.contains(hookName)) {
        return QString();
    }
    return m_hooks[hookName].description;
}

bool PluginHookRegistry::registerCallback(
    const QString& hookName, const QString& pluginName,
    PluginHookPoint::HookCallback callback) {
    if (!hasHook(hookName)) {
        LOG_WARNING(
            "PluginHookRegistry: Cannot register callback for non-existent "
            "hook '{}'",
            hookName.toStdString());
        return false;
    }

    if (pluginName.isEmpty()) {
        LOG_WARNING(
            "PluginHookRegistry: Cannot register callback with empty plugin "
            "name");
        return false;
    }

    if (!callback) {
        LOG_WARNING("PluginHookRegistry: Cannot register null callback");
        return false;
    }

    m_hooks[hookName].hookPoint->registerCallback(pluginName, callback);

    LOG_DEBUG(
        "PluginHookRegistry: Registered callback for hook '{}' from plugin "
        "'{}'",
        hookName.toStdString(), pluginName.toStdString());
    emit callbackRegistered(hookName, pluginName);

    return true;
}

void PluginHookRegistry::unregisterCallback(const QString& hookName,
                                            const QString& pluginName) {
    if (!hasHook(hookName)) {
        return;
    }

    m_hooks[hookName].hookPoint->unregisterCallback(pluginName);

    LOG_DEBUG(
        "PluginHookRegistry: Unregistered callback for hook '{}' from plugin "
        "'{}'",
        hookName.toStdString(), pluginName.toStdString());
    emit callbackUnregistered(hookName, pluginName);
}

void PluginHookRegistry::unregisterAllCallbacks(const QString& pluginName) {
    for (auto it = m_hooks.begin(); it != m_hooks.end(); ++it) {
        it.value().hookPoint->unregisterCallback(pluginName);
    }

    LOG_DEBUG("PluginHookRegistry: Unregistered all callbacks for plugin '{}'",
              pluginName.toStdString());
}

QVariant PluginHookRegistry::executeHook(const QString& hookName,
                                         const QVariantMap& context) {
    if (!hasHook(hookName)) {
        LOG_WARNING("PluginHookRegistry: Cannot execute non-existent hook '{}'",
                    hookName.toStdString());
        return QVariant();
    }

    const HookMetadata& metadata = m_hooks[hookName];

    if (!metadata.enabled) {
        LOG_DEBUG(
            "PluginHookRegistry: Hook '{}' is disabled, skipping execution",
            hookName.toStdString());
        return QVariant();
    }

    int callbackCount = metadata.hookPoint->callbackCount();
    if (callbackCount == 0) {
        LOG_DEBUG("PluginHookRegistry: No callbacks registered for hook '{}'",
                  hookName.toStdString());
        return QVariant();
    }

    LOG_DEBUG("PluginHookRegistry: Executing hook '{}' with {} callback(s)",
              hookName.toStdString(), callbackCount);

    QVariant result = metadata.hookPoint->execute(context);
    emit hookExecuted(hookName, callbackCount);

    return result;
}

int PluginHookRegistry::getCallbackCount(const QString& hookName) const {
    if (!hasHook(hookName)) {
        return 0;
    }
    return m_hooks[hookName].hookPoint->callbackCount();
}

void PluginHookRegistry::setHookEnabled(const QString& hookName, bool enabled) {
    if (!hasHook(hookName)) {
        LOG_WARNING(
            "PluginHookRegistry: Cannot enable/disable non-existent hook '{}'",
            hookName.toStdString());
        return;
    }

    m_hooks[hookName].enabled = enabled;
    LOG_INFO("PluginHookRegistry: Hook '{}' {}", hookName.toStdString(),
             enabled ? "enabled" : "disabled");
}

bool PluginHookRegistry::isHookEnabled(const QString& hookName) const {
    if (!hasHook(hookName)) {
        return false;
    }
    return m_hooks[hookName].enabled;
}
