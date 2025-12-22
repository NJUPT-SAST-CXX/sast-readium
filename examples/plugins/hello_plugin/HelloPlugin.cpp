#include "HelloPlugin.h"
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QFileInfo>
#include <QMessageBox>
#include "../../../app/controller/EventBus.h"
#include "../../../app/controller/ServiceLocator.h"
#include "../../../app/plugin/PluginHookRegistry.h"

HelloPlugin::HelloPlugin(QObject* parent)
    : PluginBase(parent),
      m_documentOpenCount(0),
      m_totalPagesViewed(0),
      m_showStatsAction(nullptr),
      m_resetCounterAction(nullptr),
      m_aboutAction(nullptr),
      m_toolbarAction(nullptr),
      m_copyPathAction(nullptr) {
    // Set plugin metadata
    m_metadata.name = "Hello Plugin";
    m_metadata.version = "2.0.0";
    m_metadata.description =
        "Example plugin demonstrating SAST Readium plugin system with UI "
        "extensions, hooks, and inter-plugin communication";
    m_metadata.author = "SAST Readium Team";
    m_metadata.dependencies = QStringList();  // No dependencies

    // Set capabilities - declare UI extension support
    m_capabilities.provides = QStringList()
                              << "example" << "demo" << "menu" << "toolbar"
                              << "context_menu" << "status_bar";
    m_capabilities.requiredPlugins = QStringList();  // No required plugins

    m_statusMessage = tr("Hello Plugin ready");
    m_logger.info("HelloPlugin instance created");
}

HelloPlugin::~HelloPlugin() {
    destroyUIActions();
    m_logger.info("HelloPlugin instance destroyed");
}

bool HelloPlugin::onInitialize() {
    m_logger.info("HelloPlugin: Initializing...");

    try {
        // Create UI actions first
        createUIActions();

        // Setup event subscriptions
        setupEventSubscriptions();

        // Setup hook callbacks
        setupHooks();

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

        // Check if UI extensions are enabled via config
        bool enableMenu = m_configuration.value("enableMenu").toBool(true);
        bool enableToolbar =
            m_configuration.value("enableToolbar").toBool(true);
        if (!enableMenu && m_showStatsAction) {
            m_showStatsAction->setVisible(false);
            m_resetCounterAction->setVisible(false);
            m_aboutAction->setVisible(false);
        }
        if (!enableToolbar && m_toolbarAction) {
            m_toolbarAction->setVisible(false);
        }

        updateStatusBar();
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
        // Remove hooks
        removeHooks();

        // Remove event subscriptions
        removeEventSubscriptions();

        // Destroy UI actions
        destroyUIActions();

        // Log statistics
        logMessage(QString("Plugin shutting down. Processed %1 document(s), "
                           "%2 pages viewed")
                       .arg(m_documentOpenCount)
                       .arg(m_totalPagesViewed));

        m_logger.info("HelloPlugin: Shutdown complete");

    } catch (const std::exception& e) {
        m_logger.error(
            QString("HelloPlugin: Shutdown error: %1").arg(e.what()));
    }
}

// ============================================================================
// IUIExtension Implementation
// ============================================================================

QList<QAction*> HelloPlugin::menuActions() const {
    QList<QAction*> actions;
    if (m_showStatsAction)
        actions.append(m_showStatsAction);
    if (m_resetCounterAction)
        actions.append(m_resetCounterAction);
    if (m_aboutAction)
        actions.append(m_aboutAction);
    return actions;
}

QString HelloPlugin::menuPath() const { return tr("Tools/Hello Plugin"); }

QList<QAction*> HelloPlugin::toolbarActions() const {
    QList<QAction*> actions;
    if (m_toolbarAction)
        actions.append(m_toolbarAction);
    return actions;
}

QString HelloPlugin::toolbarName() const { return tr("Hello Plugin"); }

QList<QAction*> HelloPlugin::contextMenuActions(
    const QString& contextId) const {
    QList<QAction*> actions;

    // Only add context menu action for document-related contexts
    if (contextId == "document" || contextId == "page") {
        if (m_copyPathAction)
            actions.append(m_copyPathAction);
    }

    return actions;
}

QString HelloPlugin::statusBarMessage() const { return m_statusMessage; }

int HelloPlugin::statusBarTimeout() const {
    // 0 = permanent message
    return 0;
}

// ============================================================================
// Inter-plugin Communication
// ============================================================================

void HelloPlugin::handleMessage(const QString& from, const QVariant& message) {
    m_logger.info(QString("HelloPlugin: Received message from '%1': %2")
                      .arg(from, message.toString()));

    // Handle specific message types
    QVariantMap msgMap = message.toMap();
    QString action = msgMap.value("action").toString();

    if (action == "get_stats") {
        // Another plugin requested our statistics
        QVariantMap response;
        response["documentsOpened"] = m_documentOpenCount;
        response["pagesViewed"] = m_totalPagesViewed;
        response["lastDocument"] = m_lastOpenedDocument;

        // Send response back via EventBus
        Event* responseEvent = new Event("plugin.response");
        QVariantMap data;
        data["from"] = name();
        data["to"] = from;
        data["data"] = response;
        responseEvent->setData(QVariant::fromValue(data));
        eventBus()->publish(responseEvent);
    } else if (action == "reset") {
        // Another plugin requested to reset our counters
        onResetCounter();
    }
}

// ============================================================================
// Event Handlers
// ============================================================================

void HelloPlugin::setupEventSubscriptions() {
    m_logger.debug("HelloPlugin: Setting up event subscriptions");

    EventBus& bus = EventBus::instance();

    bus.subscribe("document.opened", this,
                  [this](Event* event) { onDocumentOpened(event->data()); });

    bus.subscribe("document.closed", this,
                  [this](Event* event) { onDocumentClosed(event->data()); });

    // Subscribe to page view events for statistics
    bus.subscribe("page.viewed", this, [this](Event* event) {
        m_totalPagesViewed++;
        updateStatusBar();
    });

    m_logger.debug("HelloPlugin: Event subscriptions set up");
}

void HelloPlugin::removeEventSubscriptions() {
    m_logger.debug("HelloPlugin: Removing event subscriptions");
    EventBus::instance().unsubscribeAll(this);
    m_logger.debug("HelloPlugin: Event subscriptions removed");
}

void HelloPlugin::onDocumentOpened(const QVariant& data) {
    m_documentOpenCount++;

    QString filePath = data.toString();
    m_lastOpenedDocument = filePath;

    QFileInfo fileInfo(filePath);
    logMessage(QString("Document opened: %1 (Total: %2)")
                   .arg(fileInfo.fileName())
                   .arg(m_documentOpenCount));

    m_logger.info(QString("HelloPlugin: Document opened event received: %1")
                      .arg(filePath));

    updateStatusBar();
    emit statusChanged(
        QString("Processed %1 documents").arg(m_documentOpenCount));
}

void HelloPlugin::onDocumentClosed(const QVariant& data) {
    QString filePath = data.toString();
    QFileInfo fileInfo(filePath);
    logMessage(QString("Document closed: %1").arg(fileInfo.fileName()));

    m_logger.info(QString("HelloPlugin: Document closed event received: %1")
                      .arg(filePath));

    updateStatusBar();
}

// ============================================================================
// Hook Registration
// ============================================================================

void HelloPlugin::setupHooks() {
    m_logger.debug("HelloPlugin: Setting up hooks");

    auto& registry = PluginHookRegistry::instance();

    // Register pre-load hook to log document loading
    registry.registerCallback(StandardHooks::DOCUMENT_PRE_LOAD, name(),
                              [this](const QVariantMap& context) {
                                  return onDocumentPreLoad(context);
                              });

    // Register post-load hook
    registry.registerCallback(StandardHooks::DOCUMENT_POST_LOAD, name(),
                              [this](const QVariantMap& context) {
                                  return onDocumentPostLoad(context);
                              });

    m_logger.debug("HelloPlugin: Hooks set up");
}

void HelloPlugin::removeHooks() {
    m_logger.debug("HelloPlugin: Removing hooks");
    PluginHookRegistry::instance().unregisterAllCallbacks(name());
    m_logger.debug("HelloPlugin: Hooks removed");
}

QVariant HelloPlugin::onDocumentPreLoad(const QVariantMap& context) {
    QString filePath = context.value("filePath").toString();
    m_logger.info(
        QString("HelloPlugin: [HOOK] Pre-load for: %1").arg(filePath));

    // Return validation result - we could block loading here if needed
    QVariantMap result;
    result["allow"] = true;
    result["message"] = "HelloPlugin approved loading";
    return result;
}

QVariant HelloPlugin::onDocumentPostLoad(const QVariantMap& context) {
    QString filePath = context.value("filePath").toString();
    int pageCount = context.value("pageCount").toInt();

    m_logger.info(QString("HelloPlugin: [HOOK] Post-load for: %1 (%2 pages)")
                      .arg(filePath)
                      .arg(pageCount));

    QVariantMap result;
    result["processed"] = true;
    result["pluginName"] = name();
    return result;
}

// ============================================================================
// UI Actions
// ============================================================================

void HelloPlugin::createUIActions() {
    // Menu actions
    m_showStatsAction = new QAction(tr("Show Statistics"), this);
    m_showStatsAction->setToolTip(tr("Show plugin statistics"));
    connect(m_showStatsAction, &QAction::triggered, this,
            &HelloPlugin::onShowStatistics);

    m_resetCounterAction = new QAction(tr("Reset Counters"), this);
    m_resetCounterAction->setToolTip(tr("Reset all plugin counters"));
    connect(m_resetCounterAction, &QAction::triggered, this,
            &HelloPlugin::onResetCounter);

    m_aboutAction = new QAction(tr("About Hello Plugin"), this);
    m_aboutAction->setToolTip(tr("Show plugin information"));
    connect(m_aboutAction, &QAction::triggered, this,
            &HelloPlugin::onShowAbout);

    // Toolbar action
    m_toolbarAction = new QAction(tr("📊"), this);
    m_toolbarAction->setToolTip(tr("Hello Plugin - Click for statistics"));
    connect(m_toolbarAction, &QAction::triggered, this,
            &HelloPlugin::onShowStatistics);

    // Context menu action
    m_copyPathAction = new QAction(tr("Copy Document Path"), this);
    m_copyPathAction->setToolTip(tr("Copy current document path to clipboard"));
    connect(m_copyPathAction, &QAction::triggered, this,
            &HelloPlugin::onCopyDocumentPath);
}

void HelloPlugin::destroyUIActions() {
    delete m_showStatsAction;
    m_showStatsAction = nullptr;
    delete m_resetCounterAction;
    m_resetCounterAction = nullptr;
    delete m_aboutAction;
    m_aboutAction = nullptr;
    delete m_toolbarAction;
    m_toolbarAction = nullptr;
    delete m_copyPathAction;
    m_copyPathAction = nullptr;
}

void HelloPlugin::updateStatusBar() {
    m_statusMessage = tr("Hello Plugin | Docs: %1 | Pages: %2")
                          .arg(m_documentOpenCount)
                          .arg(m_totalPagesViewed);
    emit statusChanged(m_statusMessage);
}

// ============================================================================
// UI Action Slots
// ============================================================================

void HelloPlugin::onShowStatistics() {
    QString stats = tr("<h3>Hello Plugin Statistics</h3>"
                       "<p><b>Documents Opened:</b> %1</p>"
                       "<p><b>Total Pages Viewed:</b> %2</p>"
                       "<p><b>Last Document:</b> %3</p>"
                       "<p><b>Plugin Version:</b> %4</p>")
                        .arg(m_documentOpenCount)
                        .arg(m_totalPagesViewed)
                        .arg(m_lastOpenedDocument.isEmpty()
                                 ? tr("None")
                                 : QFileInfo(m_lastOpenedDocument).fileName())
                        .arg(m_metadata.version);

    QMessageBox::information(nullptr, tr("Hello Plugin Statistics"), stats);
}

void HelloPlugin::onResetCounter() {
    m_documentOpenCount = 0;
    m_totalPagesViewed = 0;
    m_lastOpenedDocument.clear();
    updateStatusBar();
    logMessage("Counters reset");
}

void HelloPlugin::onShowAbout() {
    QString about =
        tr("<h3>Hello Plugin</h3>"
           "<p><b>Version:</b> %1</p>"
           "<p><b>Author:</b> %2</p>"
           "<p><b>Description:</b><br/>%3</p>"
           "<hr/>"
           "<p><b>Demonstrates:</b></p>"
           "<ul>"
           "<li>Plugin lifecycle management</li>"
           "<li>Event subscription</li>"
           "<li>Hook registration</li>"
           "<li>UI extensions (menu, toolbar, context menu)</li>"
           "<li>Inter-plugin communication</li>"
           "<li>Configuration-driven behavior</li>"
           "</ul>")
            .arg(m_metadata.version, m_metadata.author, m_metadata.description);

    QMessageBox::about(nullptr, tr("About Hello Plugin"), about);
}

void HelloPlugin::onCopyDocumentPath() {
    if (m_lastOpenedDocument.isEmpty()) {
        QMessageBox::warning(nullptr, tr("Hello Plugin"),
                             tr("No document has been opened yet."));
        return;
    }

    QApplication::clipboard()->setText(m_lastOpenedDocument);
    logMessage(
        QString("Copied path to clipboard: %1").arg(m_lastOpenedDocument));
}

void HelloPlugin::logMessage(const QString& message) {
    m_logger.info(QString("[Hello Plugin] %1").arg(message));
    qDebug() << "[Hello Plugin]" << message;
}
