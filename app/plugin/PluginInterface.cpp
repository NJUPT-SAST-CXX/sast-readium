#include "PluginInterface.h"
#include <QAction>
#include <QCoreApplication>
#include <QDir>
#include <QDockWidget>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QStandardPaths>
#include <QStatusBar>
#include <QToolBar>
#include "../command/CommandManager.h"
#include "../controller/ConfigurationManager.h"
#include "../controller/EventBus.h"
#include "../controller/ServiceLocator.h"

// ============================================================================
// PluginBase Implementation
// ============================================================================

PluginBase::PluginBase(QObject* parent) : QObject(parent), m_logger("Plugin") {
    m_logger.debug("PluginBase created");
}

PluginBase::~PluginBase() {
    // Avoid calling virtual functions from destructor. The host should call
    // shutdown() explicitly before destruction. If still initialized, log only.
    if (m_initialized) {
        m_logger.warning(
            "Plugin destroyed while still initialized; skipping shutdown in "
            "destructor");
    }
}

bool PluginBase::initialize() {
    if (m_initialized) {
        m_logger.warning("Plugin already initialized");
        return true;
    }

    m_logger.info(QString("Initializing plugin: %1").arg(m_metadata.name));

    try {
        if (!onInitialize()) {
            m_logger.error("Plugin initialization failed");
            emit error("Initialization failed");
            return false;
        }

        m_initialized = true;
        emit initialized();
        emit statusChanged("Initialized");

        m_logger.info(QString("Plugin initialized successfully: %1")
                          .arg(m_metadata.name));
        return true;

    } catch (const std::exception& e) {
        m_logger.error(QString("Exception during plugin initialization: %1")
                           .arg(e.what()));
        emit error(QString("Initialization exception: %1").arg(e.what()));
        return false;
    }
}

void PluginBase::shutdown() {
    if (!m_initialized) {
        return;
    }

    m_logger.info(QString("Shutting down plugin: %1").arg(m_metadata.name));

    try {
        onShutdown();
        m_initialized = false;
        emit shutdownCompleted();
        emit statusChanged("Shutdown");

        m_logger.info(
            QString("Plugin shutdown successfully: %1").arg(m_metadata.name));

    } catch (const std::exception& e) {
        m_logger.error(
            QString("Exception during plugin shutdown: %1").arg(e.what()));
        emit error(QString("Shutdown exception: %1").arg(e.what()));
    }
}

void PluginBase::configure(const QJsonObject& config) {
    m_configuration = config;
    m_logger.debug(QString("Plugin configured: %1").arg(m_metadata.name));
}

ServiceLocator* PluginBase::serviceLocator() {
    return &ServiceLocator::instance();
}

EventBus* PluginBase::eventBus() { return &EventBus::instance(); }

CommandManager* PluginBase::commandManager() {
    return &GlobalCommandManager::instance();
}

ConfigurationManager* PluginBase::configurationManager() {
    return &ConfigurationManager::instance();
}

// ============================================================================
// PluginContext Implementation
// ============================================================================

PluginContext::PluginContext(QObject* parent) : QObject(parent) {}

bool PluginContext::sendMessage(const QString& targetPlugin,
                                const QVariant& message) {
    // This would require integration with PluginManager
    // For now, emit a signal that PluginManager can connect to
    emit messageReceived(targetPlugin, message);
    return true;
}

void PluginContext::broadcastMessage(const QVariant& message) {
    // Broadcast to all plugins
    emit messageReceived("*", message);
}

QString PluginContext::pluginDataPath(const QString& pluginName) const {
    QString dataPath =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString pluginPath = dataPath + "/plugins/" + pluginName + "/data";

    QDir dir;
    dir.mkpath(pluginPath);

    return pluginPath;
}

QString PluginContext::pluginConfigPath(const QString& pluginName) const {
    QString configPath =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QString pluginPath = configPath + "/plugins/" + pluginName;

    QDir dir;
    dir.mkpath(pluginPath);

    return pluginPath;
}

// ============================================================================
// MenuExtensionPoint Implementation
// ============================================================================

bool MenuExtensionPoint::accepts(IPluginInterface* plugin) const {
    if (!plugin) {
        return false;
    }

    // Check if plugin provides menu extension capability
    QStringList provides = plugin->provides();
    return provides.contains("menu") || provides.contains("ui.menu");
}

void MenuExtensionPoint::extend(IPluginInterface* plugin) {
    if (!plugin) {
        return;
    }

    qDebug() << "MenuExtensionPoint::extend called for plugin:"
             << plugin->name();

    // Try to get MenuBar from ServiceLocator
    auto& serviceLocator = ServiceLocator::instance();
    QObject* menuBarObj = serviceLocator.getService<QMenuBar>();

    if (!menuBarObj) {
        qWarning() << "MenuBar not registered in ServiceLocator. Plugin menu "
                      "extensions require MenuBar to be registered.";
        qWarning()
            << "To enable plugin menu extensions, register MenuBar with "
               "ServiceLocator::instance().registerService<QMenuBar>(menuBar)";
        return;
    }

    QMenuBar* menuBar = qobject_cast<QMenuBar*>(menuBarObj);
    if (!menuBar) {
        qWarning() << "Failed to cast MenuBar from ServiceLocator";
        return;
    }

    // Create a "Plugins" menu if it doesn't exist
    QMenu* pluginsMenu = nullptr;
    for (QAction* action : menuBar->actions()) {
        if (action->text() ==
                QCoreApplication::translate("MenuExtensionPoint", "Plugins") ||
            action->text() == "Plugins") {
            pluginsMenu = action->menu();
            break;
        }
    }

    if (!pluginsMenu) {
        pluginsMenu = menuBar->addMenu(
            QCoreApplication::translate("MenuExtensionPoint", "Plugins"));
        qDebug() << "Created 'Plugins' menu in MenuBar";
    }

    // Add plugin submenu
    QMenu* pluginSubMenu = pluginsMenu->addMenu(plugin->name());
    pluginSubMenu->setToolTip(plugin->description());

    // Add a sample action (plugins should provide their own actions via a
    // future API)
    QAction* aboutAction = pluginSubMenu->addAction(
        QCoreApplication::translate("MenuExtensionPoint", "About %1")
            .arg(plugin->name()));
    QObject::connect(aboutAction, &QAction::triggered, [plugin]() {
        QString info = QString("Plugin: %1\nVersion: %2\nAuthor: %3\n\n%4")
                           .arg(plugin->name())
                           .arg(plugin->version())
                           .arg(plugin->author())
                           .arg(plugin->description());
        // Note: QMessageBox requires QWidget parent, using nullptr for now
        // In production, should get main window from ServiceLocator
        qDebug() << "Plugin info:" << info;
    });

    qDebug() << "Successfully added menu for plugin:" << plugin->name();
}

// ============================================================================
// ToolbarExtensionPoint Implementation
// ============================================================================

bool ToolbarExtensionPoint::accepts(IPluginInterface* plugin) const {
    if (!plugin) {
        return false;
    }

    // Check if plugin provides toolbar extension capability
    QStringList provides = plugin->provides();
    return provides.contains("toolbar") || provides.contains("ui.toolbar");
}

void ToolbarExtensionPoint::extend(IPluginInterface* plugin) {
    if (!plugin) {
        return;
    }

    qDebug() << "ToolbarExtensionPoint::extend called for plugin:"
             << plugin->name();

    // Try to get ToolBar from ServiceLocator
    auto& serviceLocator = ServiceLocator::instance();
    QObject* toolBarObj = serviceLocator.getService<QToolBar>();

    if (!toolBarObj) {
        qWarning() << "ToolBar not registered in ServiceLocator. Plugin "
                      "toolbar extensions require ToolBar to be registered.";
        qWarning()
            << "To enable plugin toolbar extensions, register ToolBar with "
               "ServiceLocator::instance().registerService<QToolBar>(toolBar)";
        return;
    }

    QToolBar* toolBar = qobject_cast<QToolBar*>(toolBarObj);
    if (!toolBar) {
        qWarning() << "Failed to cast ToolBar from ServiceLocator";
        return;
    }

    // Add a separator before plugin actions
    toolBar->addSeparator();

    // Add a sample action (plugins should provide their own actions via a
    // future API) Create a simple action that shows plugin info
    QAction* pluginAction = toolBar->addAction(plugin->name());
    pluginAction->setToolTip(
        QString("%1 - %2").arg(plugin->name(), plugin->description()));
    pluginAction->setStatusTip(
        QString("Plugin: %1 v%2").arg(plugin->name(), plugin->version()));

    QObject::connect(pluginAction, &QAction::triggered, [plugin]() {
        QString info = QString("Plugin: %1\nVersion: %2\nAuthor: %3\n\n%4")
                           .arg(plugin->name())
                           .arg(plugin->version())
                           .arg(plugin->author())
                           .arg(plugin->description());
        qDebug() << "Plugin toolbar action triggered:" << info;
    });

    qDebug() << "Successfully added toolbar action for plugin:"
             << plugin->name();
}

// ============================================================================
// DocumentHandlerExtensionPoint Implementation
// ============================================================================

bool DocumentHandlerExtensionPoint::accepts(IPluginInterface* plugin) const {
    if (!plugin) {
        return false;
    }

    // Check if plugin provides document handler capability
    QStringList provides = plugin->provides();
    return provides.contains("document.handler") ||
           provides.contains("document.type") ||
           provides.contains("file.handler");
}

void DocumentHandlerExtensionPoint::extend(IPluginInterface* plugin) {
    if (!plugin) {
        return;
    }

    qDebug() << "DocumentHandlerExtensionPoint::extend called for plugin:"
             << plugin->name();

    // Get plugin capabilities
    QStringList provides = plugin->provides();

    // Log what the plugin provides for document handling
    qDebug() << "Plugin" << plugin->name() << "provides:" << provides;

    // In a full implementation, this would integrate with:
    // 1. FileTypeIconManager - Register file type associations
    // 2. DocumentModel - Register custom document loaders
    // 3. RenderModel - Register custom document renderers
    // 4. ExportDocumentCommand - Register custom document exporters

    // For now, we log the plugin's capabilities for future integration
    // A production implementation would use a dedicated DocumentHandlerRegistry
    // service to store and manage these capabilities
    qDebug() << "Registered document handler for plugin:" << plugin->name();
    qDebug() << "Handler capabilities:" << provides;

    // Future enhancement: Emit signal to notify DocumentController
    // that a new document handler is available
    // emit documentHandlerRegistered(plugin->name(), provides);
}

// ============================================================================
// DockWidgetExtensionPoint Implementation
// ============================================================================

bool DockWidgetExtensionPoint::accepts(IPluginInterface* plugin) const {
    if (!plugin) {
        return false;
    }

    // Check if plugin provides dock widget capability
    QStringList provides = plugin->provides();
    return provides.contains("dock_widget") || provides.contains("ui.dock") ||
           provides.contains("ui.dockwidget");
}

void DockWidgetExtensionPoint::extend(IPluginInterface* plugin) {
    if (!plugin) {
        return;
    }

    qDebug() << "DockWidgetExtensionPoint::extend called for plugin:"
             << plugin->name();

    // Try to cast to IUIExtension
    auto* uiExtension = dynamic_cast<IUIExtension*>(plugin);
    if (!uiExtension) {
        qWarning() << "Plugin" << plugin->name()
                   << "does not implement IUIExtension interface";
        return;
    }

    // Create the dock widget
    QWidget* dockContent = uiExtension->createDockWidget();
    if (!dockContent) {
        qWarning() << "Plugin" << plugin->name() << "returned null dock widget";
        return;
    }

    // Get MainWindow from ServiceLocator
    auto& serviceLocator = ServiceLocator::instance();
    QObject* mainWindowObj = serviceLocator.getService<QMainWindow>();

    if (!mainWindowObj) {
        qWarning() << "MainWindow not registered in ServiceLocator. Plugin "
                      "dock widget extensions require MainWindow registration.";
        delete dockContent;
        return;
    }

    QMainWindow* mainWindow = qobject_cast<QMainWindow*>(mainWindowObj);
    if (!mainWindow) {
        qWarning() << "Failed to cast MainWindow from ServiceLocator";
        delete dockContent;
        return;
    }

    // Create and add the dock widget
    QDockWidget* dockWidget =
        new QDockWidget(uiExtension->dockWidgetTitle(), mainWindow);
    dockWidget->setWidget(dockContent);
    dockWidget->setObjectName(QString("PluginDock_%1").arg(plugin->name()));

    mainWindow->addDockWidget(uiExtension->dockWidgetArea(), dockWidget);

    qDebug() << "Successfully added dock widget for plugin:" << plugin->name();
}

// ============================================================================
// ContextMenuExtensionPoint Implementation
// ============================================================================

bool ContextMenuExtensionPoint::accepts(IPluginInterface* plugin) const {
    if (!plugin) {
        return false;
    }

    // Check if plugin provides context menu capability
    QStringList provides = plugin->provides();
    return provides.contains("context_menu") ||
           provides.contains("ui.context_menu") ||
           provides.contains("contextmenu");
}

void ContextMenuExtensionPoint::extend(IPluginInterface* plugin) {
    if (!plugin) {
        return;
    }

    qDebug() << "ContextMenuExtensionPoint::extend called for plugin:"
             << plugin->name();

    // Try to cast to IUIExtension
    auto* uiExtension = dynamic_cast<IUIExtension*>(plugin);
    if (!uiExtension) {
        qWarning() << "Plugin" << plugin->name()
                   << "does not implement IUIExtension interface";
        return;
    }

    // Log that context menu extension is registered
    // In a full implementation, this would register with a ContextMenuRegistry
    // service that would provide these actions when context menus are created
    qDebug() << "Context menu extension registered for plugin:"
             << plugin->name();

    // Future: Register with ContextMenuRegistry service
    // auto* registry = serviceLocator.getService<ContextMenuRegistry>();
    // if (registry) {
    //     registry->registerContextMenuProvider(plugin->name(), uiExtension);
    // }
}

// ============================================================================
// StatusBarExtensionPoint Implementation
// ============================================================================

bool StatusBarExtensionPoint::accepts(IPluginInterface* plugin) const {
    if (!plugin) {
        return false;
    }

    // Check if plugin provides status bar capability
    QStringList provides = plugin->provides();
    return provides.contains("status_bar") ||
           provides.contains("ui.status_bar") || provides.contains("statusbar");
}

void StatusBarExtensionPoint::extend(IPluginInterface* plugin) {
    if (!plugin) {
        return;
    }

    qDebug() << "StatusBarExtensionPoint::extend called for plugin:"
             << plugin->name();

    // Try to cast to IUIExtension
    auto* uiExtension = dynamic_cast<IUIExtension*>(plugin);
    if (!uiExtension) {
        qWarning() << "Plugin" << plugin->name()
                   << "does not implement IUIExtension interface";
        return;
    }

    // Get MainWindow from ServiceLocator
    auto& serviceLocator = ServiceLocator::instance();
    QObject* mainWindowObj = serviceLocator.getService<QMainWindow>();

    if (!mainWindowObj) {
        qWarning() << "MainWindow not registered in ServiceLocator. Plugin "
                      "status bar extensions require MainWindow registration.";
        return;
    }

    QMainWindow* mainWindow = qobject_cast<QMainWindow*>(mainWindowObj);
    if (!mainWindow) {
        qWarning() << "Failed to cast MainWindow from ServiceLocator";
        return;
    }

    QStatusBar* statusBar = mainWindow->statusBar();
    if (!statusBar) {
        qWarning() << "MainWindow has no status bar";
        return;
    }

    // Display the plugin's status bar message
    QString message = uiExtension->statusBarMessage();
    if (!message.isEmpty()) {
        int timeout = uiExtension->statusBarTimeout();
        statusBar->showMessage(message, timeout);
        qDebug() << "Displayed status bar message for plugin:"
                 << plugin->name();
    }
}
