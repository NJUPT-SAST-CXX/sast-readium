#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <memory>
#include "PluginHookPoint.h"

/**
 * @brief PluginHookRegistry - Central registry for plugin hook points
 *
 * This class manages all hook points in the application where plugins can
 * register callbacks. It provides a centralized way to register, unregister,
 * and execute hooks.
 *
 * The registry is a singleton and integrates with the ServiceLocator pattern.
 */
class PluginHookRegistry : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Get the singleton instance
     */
    static PluginHookRegistry& instance();

    /**
     * @brief Register a new hook point
     * @param hookName Unique hook name (e.g., "document.pre_load")
     * @param description Human-readable description
     * @return True if hook was registered successfully
     */
    bool registerHook(const QString& hookName,
                      const QString& description = QString());

    /**
     * @brief Unregister a hook point
     * @param hookName Hook name to unregister
     */
    void unregisterHook(const QString& hookName);

    /**
     * @brief Check if hook exists
     * @param hookName Hook name to check
     * @return True if hook is registered
     */
    bool hasHook(const QString& hookName) const;

    /**
     * @brief Get all registered hook names
     * @return List of hook names
     */
    QStringList getHookNames() const;

    /**
     * @brief Get hook description
     * @param hookName Hook name
     * @return Description, or empty string if not found
     */
    QString getHookDescription(const QString& hookName) const;

    /**
     * @brief Register a callback for a hook
     * @param hookName Hook name
     * @param pluginName Plugin name registering the callback
     * @param callback Callback function
     * @return True if callback was registered successfully
     */
    bool registerCallback(const QString& hookName, const QString& pluginName,
                          PluginHookPoint::HookCallback callback);

    /**
     * @brief Unregister a callback for a hook
     * @param hookName Hook name
     * @param pluginName Plugin name to unregister
     */
    void unregisterCallback(const QString& hookName, const QString& pluginName);

    /**
     * @brief Unregister all callbacks for a plugin
     * @param pluginName Plugin name
     */
    void unregisterAllCallbacks(const QString& pluginName);

    /**
     * @brief Execute hook callbacks
     * @param hookName Hook name
     * @param context Context data to pass to callbacks
     * @return List of results from all callbacks
     */
    QVariant executeHook(const QString& hookName,
                         const QVariantMap& context = QVariantMap());

    /**
     * @brief Get number of callbacks registered for a hook
     * @param hookName Hook name
     * @return Callback count
     */
    int getCallbackCount(const QString& hookName) const;

    /**
     * @brief Enable/disable a hook
     * @param hookName Hook name
     * @param enabled Enable state
     */
    void setHookEnabled(const QString& hookName, bool enabled);

    /**
     * @brief Check if hook is enabled
     * @param hookName Hook name
     * @return True if hook is enabled
     */
    bool isHookEnabled(const QString& hookName) const;

signals:
    /**
     * @brief Emitted when a hook is registered
     */
    void hookRegistered(const QString& hookName);

    /**
     * @brief Emitted when a hook is unregistered
     */
    void hookUnregistered(const QString& hookName);

    /**
     * @brief Emitted when a callback is registered
     */
    void callbackRegistered(const QString& hookName, const QString& pluginName);

    /**
     * @brief Emitted when a callback is unregistered
     */
    void callbackUnregistered(const QString& hookName,
                              const QString& pluginName);

    /**
     * @brief Emitted when a hook is executed
     */
    void hookExecuted(const QString& hookName, int callbackCount);

private:
    explicit PluginHookRegistry(QObject* parent = nullptr);
    ~PluginHookRegistry() override = default;
    Q_DISABLE_COPY(PluginHookRegistry)

    struct HookMetadata {
        QString name;
        QString description;
        bool enabled;
        std::unique_ptr<PluginHookPoint> hookPoint;

        HookMetadata() : enabled(true) {}
    };

    QHash<QString, HookMetadata> m_hooks;
    static PluginHookRegistry* s_instance;
};

/**
 * @brief Standard Hook Names
 *
 * Predefined hook names for common workflow stages.
 */
namespace StandardHooks {
// Document workflow hooks
constexpr const char* DOCUMENT_PRE_LOAD = "document.pre_load";
constexpr const char* DOCUMENT_POST_LOAD = "document.post_load";
constexpr const char* DOCUMENT_PRE_CLOSE = "document.pre_close";
constexpr const char* DOCUMENT_POST_CLOSE = "document.post_close";
constexpr const char* DOCUMENT_METADATA_EXTRACTED =
    "document.metadata_extracted";

// Rendering workflow hooks
constexpr const char* RENDER_PRE_PAGE = "render.pre_page";
constexpr const char* RENDER_POST_PAGE = "render.post_page";
constexpr const char* RENDER_APPLY_FILTER = "render.apply_filter";
constexpr const char* RENDER_OVERLAY = "render.overlay";

// Search workflow hooks
constexpr const char* SEARCH_PRE_EXECUTE = "search.pre_execute";
constexpr const char* SEARCH_POST_EXECUTE = "search.post_execute";
constexpr const char* SEARCH_INDEX_BUILD = "search.index_build";
constexpr const char* SEARCH_RESULTS_RANK = "search.results_rank";

// Cache workflow hooks
constexpr const char* CACHE_PRE_ADD = "cache.pre_add";
constexpr const char* CACHE_POST_ADD = "cache.post_add";
constexpr const char* CACHE_PRE_EVICT = "cache.pre_evict";
constexpr const char* CACHE_POST_EVICT = "cache.post_evict";
constexpr const char* CACHE_OPTIMIZE = "cache.optimize";

// Annotation workflow hooks
constexpr const char* ANNOTATION_CREATED = "annotation.created";
constexpr const char* ANNOTATION_UPDATED = "annotation.updated";
constexpr const char* ANNOTATION_DELETED = "annotation.deleted";
constexpr const char* ANNOTATION_RENDER = "annotation.render";

// Export workflow hooks
constexpr const char* EXPORT_PRE_EXECUTE = "export.pre_execute";
constexpr const char* EXPORT_POST_EXECUTE = "export.post_execute";
}  // namespace StandardHooks
