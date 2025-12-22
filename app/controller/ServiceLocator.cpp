#include "ServiceLocator.h"
#include <QMetaObject>
#include <QMetaProperty>

// Core services
#include "../cache/CacheManager.h"
#include "../cache/PDFCacheManager.h"
#include "../command/CommandManager.h"
#include "../logging/LoggingManager.h"
#include "../plugin/PluginManager.h"
#include "AnnotationController.h"
#include "ConfigurationManager.h"
#include "EventBus.h"
#include "StateManager.h"

// UI services
#include "../managers/FileTypeIconManager.h"
#include "../managers/HighlightManager.h"
#include "../managers/I18nManager.h"
#include "../managers/KeyboardShortcutManager.h"
#include "../managers/OnboardingManager.h"
#include "../managers/StyleManager.h"
#include "../managers/SystemTrayManager.h"
#include "../ui/core/ContextMenuManager.h"
#include "../ui/core/UIConsistencyManager.h"
#include "../ui/core/UIRecoveryManager.h"
#include "../ui/core/UIResourceManager.h"
#include "../ui/core/UIStateManager.h"
#include "../ui/managers/AnnotationSelectionManager.h"
#include "../ui/managers/WelcomeScreenManager.h"
#include "../ui/theme/ReadingModeManager.h"
#include "../ui/viewer/SplitViewManager.h"
#include "../ui/widgets/EnhancedFocusIndicator.h"
#include "../ui/widgets/ToastNotification.h"

// Specialized services
#include "../accessibility/AccessibilityManager.h"
#include "../forms/FormFieldManager.h"
#include "../interaction/TextSelectionManager.h"
#include "../search/IncrementalSearchManager.h"
#include "../search/SearchEngine.h"

// ServiceLocator implementation
ServiceLocator::ServiceLocator(QObject* parent)
    : QObject(parent), m_logger("ServiceLocator") {
    // Initialize logging safely - don't log if system not ready
    try {
        m_logger.debug("ServiceLocator created");
    } catch (...) {
        // Logging system not initialized, continue silently
        // Intentionally not re-throwing - this is expected during static
        // initialization
    }
}

ServiceLocator::~ServiceLocator() {
    // Completely silent shutdown - no logging to avoid crashes
    // during static destruction when logging system is already destroyed
    clearServicesUnsafe();
}

ServiceLocator& ServiceLocator::instance() {
    static ServiceLocator instance;
    return instance;
}

void ServiceLocator::registerServiceFactory(const QString& typeName,
                                            ServiceFactory factory) {
    QMutexLocker locker(&m_mutex);

    // Validate factory
    if (!factory) {
        m_logger.error(QString("Cannot register null factory for service: %1")
                           .arg(typeName));
        return;
    }

    // Warn about duplicate registrations
    if (m_factories.contains(typeName)) {
        m_logger.warning(QString("Overwriting existing factory for service: %1")
                             .arg(typeName));
    }

    m_factories[typeName] = factory;
    m_logger.debug(QString("Registered factory for service: %1").arg(typeName));
    locker.unlock();
    emit serviceRegistered(typeName);
}

void ServiceLocator::registerServiceInstance(const QString& typeName,
                                             QObject* instance,
                                             ServiceOwnership ownership) {
    if (!instance) {
        m_logger.error(
            QString("Cannot register null service instance: %1").arg(typeName));
        return;
    }

    QMutexLocker locker(&m_mutex);

    // Remove old service if exists
    if (m_services.contains(typeName)) {
        m_logger.warning(
            QString("Replacing existing service: %1").arg(typeName));
        QObject* oldService = m_services[typeName].data();
        if (oldService && oldService->parent() == this) {
            oldService->deleteLater();
        }
    }

    m_services[typeName] = QPointer<QObject>(instance);
    m_logger.info(QString("Registered service instance: %1").arg(typeName));
    locker.unlock();
    emit serviceRegistered(typeName);
}

QObject* ServiceLocator::getServiceInstance(const QString& typeName) {
    emit serviceRequested(typeName);

    QMutexLocker locker(&m_mutex);

    // Check if service already exists
    if (m_services.contains(typeName)) {
        if (!m_services[typeName].isNull()) {
            return m_services[typeName].data();
        } else {
            // Clean up dangling pointer entry
            m_services.remove(typeName);
        }
    }

    // Try to create service if lazy loading is enabled
    if (m_lazyLoading && m_factories.contains(typeName)) {
        locker.unlock();  // Unlock during service creation to avoid deadlock
        QObject* service = createService(typeName);
        locker.relock();

        if (service != nullptr) {
            m_services[typeName] = QPointer<QObject>(service);
            return service;
        }
    }

    m_logger.warning(QString("Service not found: %1").arg(typeName));
    return nullptr;
}

QObject* ServiceLocator::createService(const QString& typeName) {
    // Check factory exists (without lock to avoid nested locks)
    ServiceFactory factory;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_factories.contains(typeName)) {
            m_logger.error(
                QString("No factory registered for service: %1").arg(typeName));
            return nullptr;
        }
        factory = m_factories[typeName];
    }

    try {
        // Create service without holding lock
        QObject* service = factory();

        if (service == nullptr) {
            m_logger.error(
                QString("Factory returned null for service: %1").arg(typeName));
            return nullptr;
        }

        service->setParent(this);  // Take ownership
        m_logger.info(QString("Created service: %1").arg(typeName));
        emit serviceCreated(typeName);
        return service;
    } catch (const std::exception& e) {
        m_logger.error(QString("Failed to create service %1: %2")
                           .arg(typeName)
                           .arg(QString::fromStdString(e.what())));
    } catch (...) {
        m_logger.error(
            QString("Unknown error creating service: %1").arg(typeName));
    }

    return nullptr;
}

bool ServiceLocator::hasService(const QString& typeName) const {
    QMutexLocker locker(&m_mutex);
    return m_services.contains(typeName) || m_factories.contains(typeName);
}

void ServiceLocator::removeService(const QString& typeName) {
    QMutexLocker locker(&m_mutex);

    // Remove from plugin tracking
    if (m_serviceToPlugin.contains(typeName)) {
        QString pluginName = m_serviceToPlugin.take(typeName);
        if (m_pluginServices.contains(pluginName)) {
            m_pluginServices[pluginName].removeAll(typeName);
            if (m_pluginServices[pluginName].isEmpty()) {
                m_pluginServices.remove(pluginName);
            }
        }
    }

    bool wasRemoved = false;
    if (m_services.contains(typeName)) {
        QPointer<QObject> ptr = m_services.take(typeName);
        QObject* service = ptr.data();
        if (service != nullptr && service->parent() == this) {
            service->deleteLater();
        }
        wasRemoved = true;
    }

    m_factories.remove(typeName);
    m_sharedServices.remove(typeName);

    if (wasRemoved) {
        m_logger.info(QString("Removed service: %1").arg(typeName));
        locker.unlock();
        emit serviceRemoved(typeName);
    }
}

void ServiceLocator::clearServices() {
    // Don't log during shutdown to avoid crashes with destroyed logging system
    try {
        m_logger.info("Clearing all services...");
    } catch (...) {
        // Logging system may be destroyed during shutdown, continue silently
    }

    QMutexLocker locker(&m_mutex);

    // Delete services we own
    for (auto it = m_services.begin(); it != m_services.end(); ++it) {
        QObject* svc = it.value().data();
        if (svc != nullptr && svc->parent() == this) {
            svc->deleteLater();
        }
    }

    m_services.clear();
    m_factories.clear();
    m_sharedServices.clear();
    m_pluginServices.clear();
    m_serviceToPlugin.clear();
}

void ServiceLocator::clearServicesUnsafe() {
    // Completely silent shutdown - no logging to avoid crashes
    // during static destruction when logging system is already destroyed
    // Don't use mutex here as it might already be destroyed

    // Delete services we own
    for (auto it = m_services.begin(); it != m_services.end(); ++it) {
        QObject* svc = it.value().data();
        if (svc != nullptr && svc->parent() == this) {
            svc->deleteLater();
        }
    }

    m_services.clear();
    m_factories.clear();
    m_sharedServices.clear();
    m_pluginServices.clear();
    m_serviceToPlugin.clear();
}

QStringList ServiceLocator::registeredServices() const {
    QMutexLocker locker(&m_mutex);

    QStringList services;
    services.append(m_services.keys());

    // Add factories that haven't been instantiated yet
    for (const QString& factoryType : m_factories.keys()) {
        if (!services.contains(factoryType)) {
            services.append(factoryType);
        }
    }

    return services;
}

// Plugin service management
void ServiceLocator::registerPluginService(const QString& pluginName,
                                           const QString& typeName,
                                           QObject* instance) {
    if (!instance) {
        m_logger.error(
            QString("Cannot register null plugin service: %1 from plugin: %2")
                .arg(typeName)
                .arg(pluginName));
        return;
    }

    QMutexLocker locker(&m_mutex);

    // Track plugin ownership
    if (!m_pluginServices.contains(pluginName)) {
        m_pluginServices[pluginName] = QStringList();
    }

    if (!m_pluginServices[pluginName].contains(typeName)) {
        m_pluginServices[pluginName].append(typeName);
    }

    m_serviceToPlugin[typeName] = pluginName;

    // Register the service
    if (m_services.contains(typeName)) {
        m_logger.warning(QString("Plugin %1 is replacing existing service: %2")
                             .arg(pluginName)
                             .arg(typeName));
        QObject* oldService = m_services[typeName].data();
        if (oldService && oldService->parent() == this) {
            oldService->deleteLater();
        }
    }

    m_services[typeName] = QPointer<QObject>(instance);
    m_logger.info(QString("Registered plugin service: %1 from plugin: %2")
                      .arg(typeName)
                      .arg(pluginName));

    locker.unlock();
    emit serviceRegistered(typeName);
}

void ServiceLocator::unregisterPluginServices(const QString& pluginName) {
    QMutexLocker locker(&m_mutex);

    if (!m_pluginServices.contains(pluginName)) {
        m_logger.debug(
            QString("No services registered for plugin: %1").arg(pluginName));
        return;
    }

    QStringList serviceNames = m_pluginServices.take(pluginName);

    for (const QString& typeName : serviceNames) {
        m_serviceToPlugin.remove(typeName);

        if (m_services.contains(typeName)) {
            QPointer<QObject> ptr = m_services.take(typeName);
            QObject* service = ptr.data();
            if (service && service->parent() == this) {
                service->deleteLater();
            }
        }

        m_factories.remove(typeName);
        m_sharedServices.remove(typeName);
    }

    m_logger.info(QString("Unregistered %1 services from plugin: %2")
                      .arg(serviceNames.size())
                      .arg(pluginName));
}

QStringList ServiceLocator::getPluginServices(const QString& pluginName) const {
    QMutexLocker locker(&m_mutex);
    return m_pluginServices.value(pluginName, QStringList());
}

bool ServiceLocator::isPluginService(const QString& typeName) const {
    QMutexLocker locker(&m_mutex);
    return m_serviceToPlugin.contains(typeName);
}

// ServiceRegistry implementation
ServiceRegistry::ServiceRegistry(ServiceLocator* locator)
    : m_locator(locator ? locator : &ServiceLocator::instance()) {}

void ServiceRegistry::registerCoreServices() {
    SastLogging::CategoryLogger logger("ServiceRegistry");
    logger.info("Registering core services...");

    // Register singleton core services
    try {
        m_locator->registerService<CacheManager>(&CacheManager::instance());
        logger.debug("Registered CacheManager");
    } catch (const std::exception& e) {
        logger.error(
            QString("Failed to register CacheManager: %1").arg(e.what()));
    }

    try {
        m_locator->registerService<CommandManager>(
            &GlobalCommandManager::instance());
        logger.debug("Registered CommandManager");
    } catch (const std::exception& e) {
        logger.error(
            QString("Failed to register CommandManager: %1").arg(e.what()));
    }

    try {
        m_locator->registerService<ConfigurationManager>(
            &ConfigurationManager::instance());
        logger.debug("Registered ConfigurationManager");
    } catch (const std::exception& e) {
        logger.error(QString("Failed to register ConfigurationManager: %1")
                         .arg(e.what()));
    }

    try {
        m_locator->registerService<StateManager>(&StateManager::instance());
        logger.debug("Registered StateManager");
    } catch (const std::exception& e) {
        logger.error(
            QString("Failed to register StateManager: %1").arg(e.what()));
    }

    try {
        m_locator->registerService<EventBus>(&EventBus::instance());
        logger.debug("Registered EventBus");
    } catch (const std::exception& e) {
        logger.error(QString("Failed to register EventBus: %1").arg(e.what()));
    }

    try {
        m_locator->registerService<LoggingManager>(&LoggingManager::instance());
        logger.debug("Registered LoggingManager");
    } catch (const std::exception& e) {
        logger.error(
            QString("Failed to register LoggingManager: %1").arg(e.what()));
    }

    try {
        m_locator->registerService<PluginManager>(&PluginManager::instance());
        logger.debug("Registered PluginManager");
    } catch (const std::exception& e) {
        logger.error(
            QString("Failed to register PluginManager: %1").arg(e.what()));
    }

    // Register factory-based services that may be needed
    // PDFCacheManager is typically created per-document
    m_locator->registerService<PDFCacheManager, PDFCacheManager>();
    logger.debug("Registered PDFCacheManager factory");

    // TODO: AccessibilityManager factory causes linker issues - needs proper
    // initialization m_locator->registerService<AccessibilityManager,
    // AccessibilityManager>(); logger.debug("Registered AccessibilityManager
    // factory");

    logger.info(QString(
        "Core services registered successfully (7 singletons, 2 factories)"));
}

void ServiceRegistry::registerUIServices() {
    SastLogging::CategoryLogger logger("ServiceRegistry");
    logger.info("Registering UI services...");

    int successCount = 0;

    // Register UI Manager singleton services
    try {
        m_locator->registerService<StyleManager>(&StyleManager::instance());
        logger.debug("Registered StyleManager");
        successCount++;
    } catch (const std::exception& e) {
        logger.error(
            QString("Failed to register StyleManager: %1").arg(e.what()));
    }

    try {
        m_locator->registerService<FileTypeIconManager>(
            &FileTypeIconManager::instance());
        logger.debug("Registered FileTypeIconManager");
        successCount++;
    } catch (const std::exception& e) {
        logger.error(QString("Failed to register FileTypeIconManager: %1")
                         .arg(e.what()));
    }

    try {
        m_locator->registerService<I18nManager>(&I18nManager::instance());
        logger.debug("Registered I18nManager");
        successCount++;
    } catch (const std::exception& e) {
        logger.error(
            QString("Failed to register I18nManager: %1").arg(e.what()));
    }

    try {
        m_locator->registerService<KeyboardShortcutManager>(
            &KeyboardShortcutManager::instance());
        logger.debug("Registered KeyboardShortcutManager");
        successCount++;
    } catch (const std::exception& e) {
        logger.error(QString("Failed to register KeyboardShortcutManager: %1")
                         .arg(e.what()));
    }

    try {
        m_locator->registerService<OnboardingManager>(
            &OnboardingManager::instance());
        logger.debug("Registered OnboardingManager");
        successCount++;
    } catch (const std::exception& e) {
        logger.error(
            QString("Failed to register OnboardingManager: %1").arg(e.what()));
    }

    try {
        m_locator->registerService<SystemTrayManager>(
            &SystemTrayManager::instance());
        logger.debug("Registered SystemTrayManager");
        successCount++;
    } catch (const std::exception& e) {
        logger.error(
            QString("Failed to register SystemTrayManager: %1").arg(e.what()));
    }

    // Register UI Core singleton services
    try {
        m_locator->registerService<UIStateManager>(&UIStateManager::instance());
        logger.debug("Registered UIStateManager");
        successCount++;
    } catch (const std::exception& e) {
        logger.error(
            QString("Failed to register UIStateManager: %1").arg(e.what()));
    }

    try {
        m_locator->registerService<UIResourceManager>(
            &UIResourceManager::instance());
        logger.debug("Registered UIResourceManager");
        successCount++;
    } catch (const std::exception& e) {
        logger.error(
            QString("Failed to register UIResourceManager: %1").arg(e.what()));
    }

    try {
        m_locator->registerService<UIConsistencyManager>(
            &UIConsistencyManager::instance());
        logger.debug("Registered UIConsistencyManager");
        successCount++;
    } catch (const std::exception& e) {
        logger.error(QString("Failed to register UIConsistencyManager: %1")
                         .arg(e.what()));
    }

    try {
        m_locator->registerService<UIRecoveryManager>(
            &UIRecoveryManager::instance());
        logger.debug("Registered UIRecoveryManager");
        successCount++;
    } catch (const std::exception& e) {
        logger.error(
            QString("Failed to register UIRecoveryManager: %1").arg(e.what()));
    }

    try {
        m_locator->registerService<ContextMenuManager>(
            &ContextMenuManager::instance());
        logger.debug("Registered ContextMenuManager");
        successCount++;
    } catch (const std::exception& e) {
        logger.error(
            QString("Failed to register ContextMenuManager: %1").arg(e.what()));
    }

    // Register UI Widget singleton services
    try {
        m_locator->registerService<EnhancedFocusIndicator>(
            &EnhancedFocusIndicator::instance());
        logger.debug("Registered EnhancedFocusIndicator");
        successCount++;
    } catch (const std::exception& e) {
        logger.error(QString("Failed to register EnhancedFocusIndicator: %1")
                         .arg(e.what()));
    }

    try {
        m_locator->registerService<ToastNotification>(
            &ToastNotification::instance());
        logger.debug("Registered ToastNotification");
        successCount++;
    } catch (const std::exception& e) {
        logger.error(
            QString("Failed to register ToastNotification: %1").arg(e.what()));
    }

    // Register UI-specific singleton managers
    try {
        m_locator->registerService<HighlightManager>(
            &HighlightManager::instance());
        logger.debug("Registered HighlightManager");
        successCount++;
    } catch (const std::exception& e) {
        logger.error(
            QString("Failed to register HighlightManager: %1").arg(e.what()));
    }

    // TODO: ReadingModeManager singleton causes linker issues - needs proper
    // initialization try {
    //     m_locator->registerService<ReadingModeManager>(
    //         &ReadingModeManager::instance());
    //     logger.debug("Registered ReadingModeManager");
    //     successCount++;
    // } catch (const std::exception& e) {
    //     logger.error(
    //         QString("Failed to register ReadingModeManager:
    //         %1").arg(e.what()));
    // }

    try {
        m_locator->registerService<SplitViewManager>(
            &SplitViewManager::instance());
        logger.debug("Registered SplitViewManager");
        successCount++;
    } catch (const std::exception& e) {
        logger.error(
            QString("Failed to register SplitViewManager: %1").arg(e.what()));
    }

    try {
        m_locator->registerService<WelcomeScreenManager>(
            &WelcomeScreenManager::instance());
        logger.debug("Registered WelcomeScreenManager");
        successCount++;
    } catch (const std::exception& e) {
        logger.error(QString("Failed to register WelcomeScreenManager: %1")
                         .arg(e.what()));
    }

    try {
        m_locator->registerService<AnnotationSelectionManager>(
            &AnnotationSelectionManager::instance());
        logger.debug("Registered AnnotationSelectionManager");
        successCount++;
    } catch (const std::exception& e) {
        logger.error(
            QString("Failed to register AnnotationSelectionManager: %1")
                .arg(e.what()));
    }

    logger.info(QString("UI services registered successfully (%1/18 services)")
                    .arg(successCount));
}

void ServiceRegistry::registerModelServices() {
    SastLogging::CategoryLogger logger("ServiceRegistry");
    logger.info("Registering model services...");

    // Models are typically created on-demand and tied to specific
    // documents/views They are registered as factories, not singleton instances
    // The ServiceLocator will create new instances when requested

    // Note: Most models in this application are created directly by their
    // controllers or views and not managed through ServiceLocator. The factory
    // pattern here is available for future use cases where centralized model
    // creation is beneficial.

    // Example registrations (commented out as models are currently created
    // directly): m_locator->registerService<DocumentModel, DocumentModel>();
    // m_locator->registerService<PageModel, PageModel>();
    // m_locator->registerService<RenderModel, RenderModel>();
    // m_locator->registerService<AnnotationModel, AnnotationModel>();
    // m_locator->registerService<BookmarkModel, BookmarkModel>();
    // m_locator->registerService<SearchModel, SearchModel>();
    // m_locator->registerService<ThumbnailModel, ThumbnailModel>();
    // m_locator->registerService<HighlightModel, HighlightModel>();
    // m_locator->registerService<PDFOutlineModel, PDFOutlineModel>();
    // m_locator->registerService<AccessibilityModel, AccessibilityModel>();

    logger.info(
        "Model services registry ready (factory registration available "
        "on-demand)");
}

void ServiceRegistry::registerControllerServices() {
    SastLogging::CategoryLogger logger("ServiceRegistry");
    logger.info("Registering controller services...");

    int successCount = 0;

    // Register singleton controllers
    try {
        m_locator->registerService<AnnotationController>(
            AnnotationController::instance());
        logger.debug("Registered AnnotationController");
        successCount++;
    } catch (const std::exception& e) {
        logger.error(QString("Failed to register AnnotationController: %1")
                         .arg(e.what()));
    }

    // Register specialized manager and engine singletons
    try {
        m_locator->registerService<SearchEngine>(&SearchEngine::instance());
        logger.debug("Registered SearchEngine");
        successCount++;
    } catch (const std::exception& e) {
        logger.error(
            QString("Failed to register SearchEngine: %1").arg(e.what()));
    }

    try {
        m_locator->registerService<IncrementalSearchManager>(
            &IncrementalSearchManager::instance());
        logger.debug("Registered IncrementalSearchManager");
        successCount++;
    } catch (const std::exception& e) {
        logger.error(QString("Failed to register IncrementalSearchManager: %1")
                         .arg(e.what()));
    }

    try {
        m_locator->registerService<TextSelectionManager>(
            &TextSelectionManager::instance());
        logger.debug("Registered TextSelectionManager");
        successCount++;
    } catch (const std::exception& e) {
        logger.error(QString("Failed to register TextSelectionManager: %1")
                         .arg(e.what()));
    }

    // TODO: FormFieldManager singleton causes linker issues - needs proper
    // initialization try {
    //     m_locator->registerService<FormFieldManager>(
    //         &FormFieldManager::instance());
    //     logger.debug("Registered FormFieldManager");
    //     successCount++;
    // } catch (const std::exception& e) {
    //     logger.error(
    //         QString("Failed to register FormFieldManager:
    //         %1").arg(e.what()));
    // }

    // Note: DocumentController, PageController, ApplicationController, and
    // AccessibilityController are typically created and managed by the
    // application, not through ServiceLocator. They can be registered when
    // needed using the registerService template method.

    logger.info(
        QString("Controller services registered successfully (%1/5 services)")
            .arg(successCount));
}

// ServiceProvider implementation
ServiceProvider::ServiceProvider(QObject* parent) : QObject(parent) {}

ServiceProvider::~ServiceProvider() {
    // Remove provided services from the locator
    for (const QString& typeName : m_providedServices) {
        ServiceLocator::instance().removeService(typeName);
    }
}

// DependencyInjector implementation
void DependencyInjector::injectServices(QObject* object) {
    if (object == nullptr) {
        return;
    }

    // Use Qt's meta-object system to find properties that need injection
    const QMetaObject* metaObject = object->metaObject();

    for (int i = 0; i < metaObject->propertyCount(); ++i) {
        QMetaProperty property = metaObject->property(i);

        // Check if property has an "inject" annotation (custom attribute)
        // This is a simplified approach - in practice, you might use a naming
        // convention or custom attributes
        QString propertyName = property.name();
        if (propertyName.endsWith("Service") ||
            propertyName.endsWith("Controller") ||
            propertyName.endsWith("Manager")) {
            // Try to get the service type from the property type
            QString typeName = property.typeName();
            QObject* service =
                ServiceLocator::instance().getServiceInstance(typeName);

            if (service != nullptr && property.isWritable()) {
                property.write(object, QVariant::fromValue(service));
            }
        }
    }
}

// ServiceScope implementation
ServiceScope::ServiceScope(ServiceLocator* locator)
    : m_locator(locator != nullptr ? locator : &ServiceLocator::instance()) {}

ServiceScope::~ServiceScope() {
    // Remove all scoped services when scope ends
    for (const QString& typeName : m_scopedServices) {
        m_locator->removeService(typeName);
    }
}
