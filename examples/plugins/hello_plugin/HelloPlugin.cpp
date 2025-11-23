#include "HelloPlugin.h"
#include <QDebug>
#include "../../../app/controller/EventBus.h"
#include "../../../app/controller/ServiceLocator.h"

HelloPlugin::HelloPlugin(QObject* parent)
    : PluginBase(parent), m_documentOpenCount(0) {
    // Set plugin metadata
    m_metadata.name = "Hello Plugin";
    m_metadata.version = "1.0.0";
    m_metadata.description =
        "Example plugin demonstrating SAST Readium plugin system";
    m_metadata.author = "SAST Readium Team";
    m_metadata.dependencies = QStringList();  // No dependencies

    // Set capabilities
    m_capabilities.provides = QStringList() << "example" << "demo";
    m_capabilities.requiredPlugins = QStringList();  // No required plugins

    m_logger.info("HelloPlugin instance created");
}

HelloPlugin::~HelloPlugin() { m_logger.info("HelloPlugin instance destroyed"); }

bool HelloPlugin::onInitialize() {
    m_logger.info("HelloPlugin: Initializing...");

    try {
        // Setup event subscriptions
        setupEventSubscriptions();

        // Log initialization success
        logMessage("Hello Plugin initialized successfully!");
        logMessage(QString("Plugin name: %1, Version: %2")
                       .arg(m_metadata.name, m_metadata.version));

        // Access configuration
        if (m_configuration.contains("greeting")) {
            QString greeting = m_configuration["greeting"].toString();
            logMessage(QString("Custom greeting: %1").arg(greeting));
        } else {
            logMessage("Using default greeting");
        }

        m_logger.info("HelloPlugin: Initialization complete");
        return true;

    } catch (const std::exception& e) {
        m_logger.error(
            QString("HelloPlugin: Initialization failed: %1").arg(e.what()));
        return false;
    }
}

void HelloPlugin::onShutdown() {
    m_logger.info("HelloPlugin: Shutting down...");

    try {
        // Remove event subscriptions
        removeEventSubscriptions();

        // Log statistics
        logMessage(QString("Plugin shutting down. Processed %1 document(s)")
                       .arg(m_documentOpenCount));

        m_logger.info("HelloPlugin: Shutdown complete");

    } catch (const std::exception& e) {
        m_logger.error(
            QString("HelloPlugin: Shutdown error: %1").arg(e.what()));
    }
}

void HelloPlugin::setupEventSubscriptions() {
    m_logger.debug("HelloPlugin: Setting up event subscriptions");

    // Subscribe to document events using EventBus
    EventBus& eventBus = EventBus::instance();

    eventBus.subscribe("document.opened", this, [this](Event* event) {
        onDocumentOpened(event->data());
    });

    eventBus.subscribe("document.closed", this, [this](Event* event) {
        onDocumentClosed(event->data());
    });

    m_logger.debug("HelloPlugin: Event subscriptions set up");
}

void HelloPlugin::removeEventSubscriptions() {
    m_logger.debug("HelloPlugin: Removing event subscriptions");

    // Unsubscribe from all events
    EventBus& eventBus = EventBus::instance();
    eventBus.unsubscribeAll(this);

    m_logger.debug("HelloPlugin: Event subscriptions removed");
}

void HelloPlugin::onDocumentOpened(const QVariant& data) {
    m_documentOpenCount++;

    QString filePath = data.toString();
    logMessage(QString("Document opened: %1 (Total: %2)")
                   .arg(filePath)
                   .arg(m_documentOpenCount));

    m_logger.info(QString("HelloPlugin: Document opened event received: %1")
                      .arg(filePath));

    // Emit status change
    emit statusChanged(
        QString("Processed %1 documents").arg(m_documentOpenCount));
}

void HelloPlugin::onDocumentClosed(const QVariant& data) {
    QString filePath = data.toString();
    logMessage(QString("Document closed: %1").arg(filePath));

    m_logger.info(QString("HelloPlugin: Document closed event received: %1")
                      .arg(filePath));
}

void HelloPlugin::logMessage(const QString& message) {
    // Log to plugin logger
    m_logger.info(QString("[Hello Plugin] %1").arg(message));

    // Also log via qDebug for demonstration
    qDebug() << "[Hello Plugin]" << message;
}
