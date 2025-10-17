#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <functional>
#include <memory>
#include <typeinfo>
#include "../logging/SimpleLogging.h"

/**
 * @brief ServiceLocator - Provides dependency injection through service
 * location
 *
 * This class implements the Service Locator pattern to provide a centralized
 * registry for services and dependencies. It allows loose coupling between
 * components by providing a way to obtain dependencies without direct creation.
 */
class ServiceLocator : public QObject {
    Q_OBJECT

public:
    ~ServiceLocator();

    // Singleton access
    static ServiceLocator& instance();

    // Service registration
    template <typename Interface, typename Implementation>
    void registerService() {
        QString typeName = QString::fromStdString(typeid(Interface).name());
        auto factory = []() -> QObject* { return new Implementation(); };
        registerServiceFactory(typeName, factory);
    }

    template <typename Interface>
    void registerService(Interface* instance) {
        QString typeName = QString::fromStdString(typeid(Interface).name());
        registerServiceInstance(typeName, instance);
    }

    template <typename Interface>
    void registerService(std::shared_ptr<Interface> instance) {
        QString typeName = QString::fromStdString(typeid(Interface).name());
        m_sharedServices[typeName] = instance;
        registerServiceInstance(typeName, instance.get());
    }

    // Service retrieval
    template <typename Interface>
    Interface* getService() {
        QString typeName = QString::fromStdString(typeid(Interface).name());
        QObject* service = getServiceInstance(typeName);
        return qobject_cast<Interface*>(service);
    }

    template <typename Interface>
    std::shared_ptr<Interface> getSharedService() {
        QString typeName = QString::fromStdString(typeid(Interface).name());
        if (m_sharedServices.contains(typeName)) {
            return std::static_pointer_cast<Interface>(
                m_sharedServices[typeName]);
        }
        return nullptr;
    }

    // Service management
    bool hasService(const QString& typeName) const;
    void removeService(const QString& typeName);
    void clearServices();
    void clearServicesUnsafe();  // Safe shutdown without logging
    QStringList registeredServices() const;

    // Direct access for special cases (use with caution)
    QObject* getServiceInstance(const QString& typeName);

    // Lazy initialization
    void setLazyLoading(bool lazy) { m_lazyLoading = lazy; }
    bool isLazyLoading() const { return m_lazyLoading; }

signals:
    void serviceRegistered(const QString& typeName);
    void serviceRemoved(const QString& typeName);
    void serviceRequested(const QString& typeName);
    void serviceCreated(const QString& typeName);

private:
    ServiceLocator(QObject* parent = nullptr);
    ServiceLocator(const ServiceLocator&) = delete;
    ServiceLocator& operator=(const ServiceLocator&) = delete;

    using ServiceFactory = std::function<QObject*()>;

    void registerServiceFactory(const QString& typeName,
                                ServiceFactory factory);
    void registerServiceInstance(const QString& typeName, QObject* instance);
    QObject* createService(const QString& typeName);

    // Service storage
    QHash<QString, QObject*> m_services;
    QHash<QString, ServiceFactory> m_factories;
    QHash<QString, std::shared_ptr<void>> m_sharedServices;

    // Configuration
    bool m_lazyLoading = true;

    // Logging
    SastLogging::CategoryLogger m_logger{"ServiceLocator"};
};

/**
 * @brief ServiceRegistry - Helper class for registering multiple services
 */
class ServiceRegistry {
public:
    explicit ServiceRegistry(ServiceLocator* locator = nullptr);

    // Register common application services
    void registerCoreServices();
    void registerUIServices();
    void registerModelServices();
    void registerControllerServices();

    // Register custom service
    template <typename Interface, typename Implementation>
    ServiceRegistry& registerService() {
        m_locator->registerService<Interface, Implementation>();
        return *this;
    }

    template <typename Interface>
    ServiceRegistry& registerService(Interface* instance) {
        m_locator->registerService<Interface>(instance);
        return *this;
    }

private:
    ServiceLocator* m_locator;
};

/**
 * @brief ServiceProvider - Base class for classes that provide services
 */
class ServiceProvider : public QObject {
    Q_OBJECT

public:
    explicit ServiceProvider(QObject* parent = nullptr);
    virtual ~ServiceProvider();

    // Service lifecycle
    virtual void initializeServices() = 0;
    virtual void shutdownServices() = 0;

    // Service registration
    template <typename Interface>
    void provideService(Interface* service) {
        ServiceLocator::instance().registerService<Interface>(service);
        m_providedServices.append(
            QString::fromStdString(typeid(Interface).name()));
    }

signals:
    void servicesInitialized();
    void servicesShutdown();

protected:
    QStringList providedServices() const { return m_providedServices; }

private:
    QStringList m_providedServices;
};

/**
 * @brief DependencyInjector - Helper for injecting dependencies into objects
 */
class DependencyInjector {
public:
    // Inject dependencies into an object
    template <typename T>
    static void inject(T* object) {
        // This would typically use reflection or metadata to determine
        // what dependencies to inject. For C++/Qt, we'll use a simpler approach
        injectServices(object);
    }

    // Manual dependency injection
    template <typename Object, typename Service>
    static void injectService(Object* object,
                              void (Object::*setter)(Service*)) {
        Service* service = ServiceLocator::instance().getService<Service>();
        if (service && object && setter) {
            (object->*setter)(service);
        }
    }

private:
    static void injectServices(QObject* object);
};

/**
 * @brief ServiceScope - RAII wrapper for temporary service registration
 */
class ServiceScope {
public:
    explicit ServiceScope(ServiceLocator* locator = nullptr);
    ~ServiceScope();

    template <typename Interface, typename Implementation>
    void registerScoped() {
        QString typeName = QString::fromStdString(typeid(Interface).name());
        m_scopedServices.append(typeName);
        m_locator->registerService<Interface, Implementation>();
    }

    template <typename Interface>
    void registerScoped(Interface* instance) {
        QString typeName = QString::fromStdString(typeid(Interface).name());
        m_scopedServices.append(typeName);
        m_locator->registerService<Interface>(instance);
    }

private:
    ServiceLocator* m_locator;
    QStringList m_scopedServices;
};

// Convenience macros for service access
#define GET_SERVICE(Type) ServiceLocator::instance().getService<Type>()
#define REGISTER_SERVICE(Interface, Implementation) \
    ServiceLocator::instance().registerService<Interface, Implementation>()
#define PROVIDE_SERVICE(Interface, instance) \
    ServiceLocator::instance().registerService<Interface>(instance)
