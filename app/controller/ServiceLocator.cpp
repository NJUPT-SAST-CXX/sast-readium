#include "ServiceLocator.h"
#include <QMetaObject>
#include <QMetaProperty>

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
    m_factories[typeName] = factory;
    m_logger.debug(QString("Registered factory for service: %1").arg(typeName));
    emit serviceRegistered(typeName);
}

void ServiceLocator::registerServiceInstance(const QString& typeName,
                                             QObject* instance) {
    if (!instance) {
        m_logger.warning(
            QString("Attempted to register null service: %1").arg(typeName));
        return;
    }

    // Remove old service if exists
    if (m_services.contains(typeName)) {
        QObject* oldService = m_services[typeName];
        if (oldService && oldService->parent() == this) {
            oldService->deleteLater();
        }
    }

    m_services[typeName] = instance;
    m_logger.info(QString("Registered service instance: %1").arg(typeName));
    emit serviceRegistered(typeName);
}

QObject* ServiceLocator::getServiceInstance(const QString& typeName) {
    emit serviceRequested(typeName);

    // Check if service already exists
    if (m_services.contains(typeName)) {
        return m_services[typeName];
    }

    // Try to create service if lazy loading is enabled
    if (m_lazyLoading && m_factories.contains(typeName)) {
        QObject* service = createService(typeName);
        if (service) {
            m_services[typeName] = service;
            return service;
        }
    }

    m_logger.warning(QString("Service not found: %1").arg(typeName));
    return nullptr;
}

QObject* ServiceLocator::createService(const QString& typeName) {
    if (!m_factories.contains(typeName)) {
        m_logger.error(
            QString("No factory registered for service: %1").arg(typeName));
        return nullptr;
    }

    try {
        ServiceFactory factory = m_factories[typeName];
        QObject* service = factory();

        if (service) {
            service->setParent(this);  // Take ownership
            m_logger.info(QString("Created service: %1").arg(typeName));
            emit serviceCreated(typeName);
            return service;
        }
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
    return m_services.contains(typeName) || m_factories.contains(typeName);
}

void ServiceLocator::removeService(const QString& typeName) {
    if (m_services.contains(typeName)) {
        QObject* service = m_services.take(typeName);
        if (service && service->parent() == this) {
            service->deleteLater();
        }
        m_logger.info(QString("Removed service: %1").arg(typeName));
        emit serviceRemoved(typeName);
    }

    m_factories.remove(typeName);
    m_sharedServices.remove(typeName);
}

void ServiceLocator::clearServices() {
    // Don't log during shutdown to avoid crashes with destroyed logging system
    try {
        m_logger.info("Clearing all services...");
    } catch (...) {
        // Logging system may be destroyed during shutdown, continue silently
    }

    // Delete services we own
    for (auto it = m_services.begin(); it != m_services.end(); ++it) {
        if (it.value() && it.value()->parent() == this) {
            it.value()->deleteLater();
        }
    }

    m_services.clear();
    m_factories.clear();
    m_sharedServices.clear();
}

void ServiceLocator::clearServicesUnsafe() {
    // Completely silent shutdown - no logging to avoid crashes
    // during static destruction when logging system is already destroyed

    // Delete services we own
    for (auto it = m_services.begin(); it != m_services.end(); ++it) {
        if (it.value() && it.value()->parent() == this) {
            it.value()->deleteLater();
        }
    }

    m_services.clear();
    m_factories.clear();
    m_sharedServices.clear();
}

QStringList ServiceLocator::registeredServices() const {
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

// ServiceRegistry implementation
ServiceRegistry::ServiceRegistry(ServiceLocator* locator)
    : m_locator(locator ? locator : &ServiceLocator::instance()) {}

void ServiceRegistry::registerCoreServices() {
    // Register core application services
    // These would typically be implemented in the actual application
    // For now, we'll just log the intention

    // Example registrations (would need actual implementations):
    // m_locator->registerService<ConfigurationManager, ConfigurationManager>();
    // m_locator->registerService<CommandManager, CommandManager>();
    // m_locator->registerService<ErrorHandler, ErrorHandler>();
}

void ServiceRegistry::registerUIServices() {
    // Register UI-related services
    // m_locator->registerService<StyleManager, StyleManager>();
    // m_locator->registerService<ThemeManager, ThemeManager>();
    // m_locator->registerService<IconManager, IconManager>();
}

void ServiceRegistry::registerModelServices() {
    // Register model services
    // m_locator->registerService<DocumentModel, DocumentModel>();
    // m_locator->registerService<PageModel, PageModel>();
    // m_locator->registerService<RenderModel, RenderModel>();
}

void ServiceRegistry::registerControllerServices() {
    // Register controller services
    // m_locator->registerService<DocumentController, DocumentController>();
    // m_locator->registerService<PageController, PageController>();
    // m_locator->registerService<ApplicationController,
    // ApplicationController>();
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
    if (!object)
        return;

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

            if (service && property.isWritable()) {
                property.write(object, QVariant::fromValue(service));
            }
        }
    }
}

// ServiceScope implementation
ServiceScope::ServiceScope(ServiceLocator* locator)
    : m_locator(locator ? locator : &ServiceLocator::instance()) {}

ServiceScope::~ServiceScope() {
    // Remove all scoped services when scope ends
    for (const QString& typeName : m_scopedServices) {
        m_locator->removeService(typeName);
    }
}
