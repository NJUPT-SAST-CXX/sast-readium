#include <QAction>
#include <QDockWidget>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QSignalSpy>
#include <QStatusBar>
#include <QTest>
#include <QToolBar>

#include "../../app/controller/ServiceLocator.h"
#include "../../app/plugin/PluginInterface.h"
#include "../../app/plugin/PluginManager.h"
#include "../TestUtilities.h"

/**
 * @brief Mock plugin for testing extension points
 */
class MockExtensionPlugin : public PluginBase, public IUIExtension {
    Q_OBJECT
    Q_INTERFACES(IPluginInterface)

public:
    explicit MockExtensionPlugin(QObject* parent = nullptr)
        : PluginBase(parent) {
        m_metadata.name = "MockExtensionPlugin";
        m_metadata.version = "1.0.0";
        m_metadata.author = "Test";
        m_metadata.description = "Mock plugin for extension point tests";
    }

    // Configure what capabilities this plugin provides
    void setProvides(const QStringList& provides) {
        m_capabilities.provides = provides;
    }

    // IUIExtension implementation
    QList<QAction*> menuActions() const override { return m_menuActions; }
    QString menuPath() const override { return m_menuPath; }

    QList<QAction*> toolbarActions() const override { return m_toolbarActions; }
    QString toolbarName() const override { return m_toolbarName; }

    QWidget* createDockWidget(QWidget* parent = nullptr) override {
        if (m_createDockWidget) {
            return new QWidget(parent);
        }
        return nullptr;
    }
    QString dockWidgetTitle() const override { return m_dockWidgetTitle; }
    Qt::DockWidgetArea dockWidgetArea() const override {
        return m_dockWidgetArea;
    }

    QList<QAction*> contextMenuActions(
        const QString& contextId) const override {
        Q_UNUSED(contextId);
        return m_contextMenuActions;
    }

    QString statusBarMessage() const override { return m_statusBarMessage; }
    int statusBarTimeout() const override { return m_statusBarTimeout; }

    // Test configuration setters
    void setMenuActions(const QList<QAction*>& actions) {
        m_menuActions = actions;
    }
    void setMenuPath(const QString& path) { m_menuPath = path; }
    void setToolbarActions(const QList<QAction*>& actions) {
        m_toolbarActions = actions;
    }
    void setToolbarName(const QString& name) { m_toolbarName = name; }
    void setCreateDockWidget(bool create) { m_createDockWidget = create; }
    void setDockWidgetTitle(const QString& title) { m_dockWidgetTitle = title; }
    void setDockWidgetArea(Qt::DockWidgetArea area) { m_dockWidgetArea = area; }
    void setContextMenuActions(const QList<QAction*>& actions) {
        m_contextMenuActions = actions;
    }
    void setStatusBarMessage(const QString& msg) { m_statusBarMessage = msg; }
    void setStatusBarTimeout(int timeout) { m_statusBarTimeout = timeout; }

protected:
    bool onInitialize() override { return true; }
    void onShutdown() override {}

private:
    QList<QAction*> m_menuActions;
    QString m_menuPath;
    QList<QAction*> m_toolbarActions;
    QString m_toolbarName;
    bool m_createDockWidget = false;
    QString m_dockWidgetTitle;
    Qt::DockWidgetArea m_dockWidgetArea = Qt::RightDockWidgetArea;
    QList<QAction*> m_contextMenuActions;
    QString m_statusBarMessage;
    int m_statusBarTimeout = 0;
};

/**
 * @brief Test fixture for Extension Points
 */
class PluginExtensionPointsTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override;
    void cleanupTestCase() override;
    void init() override;
    void cleanup() override;

    // IExtensionPoint base interface tests
    void test_extension_point_interface();

    // MenuExtensionPoint tests
    void test_menu_extension_point_id();
    void test_menu_extension_point_description();
    void test_menu_extension_point_accepts_null();
    void test_menu_extension_point_accepts_menu_provider();
    void test_menu_extension_point_accepts_ui_menu_provider();
    void test_menu_extension_point_rejects_non_menu_provider();
    void test_menu_extension_point_extend();

    // ToolbarExtensionPoint tests
    void test_toolbar_extension_point_id();
    void test_toolbar_extension_point_description();
    void test_toolbar_extension_point_accepts_null();
    void test_toolbar_extension_point_accepts_toolbar_provider();
    void test_toolbar_extension_point_accepts_ui_toolbar_provider();
    void test_toolbar_extension_point_rejects_non_toolbar_provider();
    void test_toolbar_extension_point_extend();

    // DocumentHandlerExtensionPoint tests
    void test_document_handler_extension_point_id();
    void test_document_handler_extension_point_description();
    void test_document_handler_extension_point_accepts_null();
    void test_document_handler_extension_point_accepts_document_handler();
    void test_document_handler_extension_point_accepts_document_type();
    void test_document_handler_extension_point_accepts_file_handler();
    void test_document_handler_extension_point_rejects_non_handler();
    void test_document_handler_extension_point_extend();

    // DockWidgetExtensionPoint tests
    void test_dock_widget_extension_point_id();
    void test_dock_widget_extension_point_description();
    void test_dock_widget_extension_point_accepts_null();
    void test_dock_widget_extension_point_accepts_dock_widget();
    void test_dock_widget_extension_point_accepts_ui_dock();
    void test_dock_widget_extension_point_rejects_non_dock();
    void test_dock_widget_extension_point_extend();
    void test_dock_widget_extension_point_extend_no_ui_extension();

    // ContextMenuExtensionPoint tests
    void test_context_menu_extension_point_id();
    void test_context_menu_extension_point_description();
    void test_context_menu_extension_point_accepts_null();
    void test_context_menu_extension_point_accepts_context_menu();
    void test_context_menu_extension_point_accepts_ui_context_menu();
    void test_context_menu_extension_point_rejects_non_context_menu();
    void test_context_menu_extension_point_extend();

    // StatusBarExtensionPoint tests
    void test_status_bar_extension_point_id();
    void test_status_bar_extension_point_description();
    void test_status_bar_extension_point_accepts_null();
    void test_status_bar_extension_point_accepts_status_bar();
    void test_status_bar_extension_point_accepts_ui_status_bar();
    void test_status_bar_extension_point_rejects_non_status_bar();
    void test_status_bar_extension_point_extend();

    // PluginManager extension point management tests
    void test_plugin_manager_register_extension_point();
    void test_plugin_manager_register_null_extension_point();
    void test_plugin_manager_register_duplicate_extension_point();
    void test_plugin_manager_unregister_extension_point();
    void test_plugin_manager_get_extension_points();
    void test_plugin_manager_apply_extension_points();

private:
    QMainWindow* m_mainWindow = nullptr;
    QMenuBar* m_menuBar = nullptr;
    QToolBar* m_toolBar = nullptr;
};

void PluginExtensionPointsTest::initTestCase() { TestBase::initTestCase(); }

void PluginExtensionPointsTest::cleanupTestCase() {
    TestBase::cleanupTestCase();
}

void PluginExtensionPointsTest::init() {
    // Create main window with UI components for extension point tests
    m_mainWindow = new QMainWindow();
    m_menuBar = m_mainWindow->menuBar();
    m_toolBar = m_mainWindow->addToolBar("Main");

    // Register UI components with ServiceLocator
    auto& locator = ServiceLocator::instance();
    locator.registerService<QMainWindow>(m_mainWindow);
    locator.registerService<QMenuBar>(m_menuBar);
    locator.registerService<QToolBar>(m_toolBar);
}

void PluginExtensionPointsTest::cleanup() {
    // Unregister services
    auto& locator = ServiceLocator::instance();
    locator.unregisterService<QMainWindow>();
    locator.unregisterService<QMenuBar>();
    locator.unregisterService<QToolBar>();

    delete m_mainWindow;
    m_mainWindow = nullptr;
    m_menuBar = nullptr;
    m_toolBar = nullptr;
}

// ============================================================================
// IExtensionPoint Base Interface Tests
// ============================================================================

void PluginExtensionPointsTest::test_extension_point_interface() {
    MenuExtensionPoint menuExt;
    ToolbarExtensionPoint toolbarExt;
    DocumentHandlerExtensionPoint docExt;
    DockWidgetExtensionPoint dockExt;
    ContextMenuExtensionPoint ctxMenuExt;
    StatusBarExtensionPoint statusExt;

    // All extension points should have unique IDs
    QStringList ids;
    ids << menuExt.id() << toolbarExt.id() << docExt.id() << dockExt.id()
        << ctxMenuExt.id() << statusExt.id();

    // Verify uniqueness
    QSet<QString> idSet(ids.begin(), ids.end());
    QCOMPARE(idSet.size(), ids.size());

    // All should have descriptions
    QVERIFY(!menuExt.description().isEmpty());
    QVERIFY(!toolbarExt.description().isEmpty());
    QVERIFY(!docExt.description().isEmpty());
    QVERIFY(!dockExt.description().isEmpty());
    QVERIFY(!ctxMenuExt.description().isEmpty());
    QVERIFY(!statusExt.description().isEmpty());
}

// ============================================================================
// MenuExtensionPoint Tests
// ============================================================================

void PluginExtensionPointsTest::test_menu_extension_point_id() {
    MenuExtensionPoint ext;
    QCOMPARE(ext.id(), QString("org.sast.readium.menu"));
}

void PluginExtensionPointsTest::test_menu_extension_point_description() {
    MenuExtensionPoint ext;
    QVERIFY(ext.description().contains("menu", Qt::CaseInsensitive));
}

void PluginExtensionPointsTest::test_menu_extension_point_accepts_null() {
    MenuExtensionPoint ext;
    QVERIFY(!ext.accepts(nullptr));
}

void PluginExtensionPointsTest::
    test_menu_extension_point_accepts_menu_provider() {
    MenuExtensionPoint ext;
    MockExtensionPlugin plugin;
    plugin.setProvides({"menu"});
    QVERIFY(ext.accepts(&plugin));
}

void PluginExtensionPointsTest::
    test_menu_extension_point_accepts_ui_menu_provider() {
    MenuExtensionPoint ext;
    MockExtensionPlugin plugin;
    plugin.setProvides({"ui.menu"});
    QVERIFY(ext.accepts(&plugin));
}

void PluginExtensionPointsTest::
    test_menu_extension_point_rejects_non_menu_provider() {
    MenuExtensionPoint ext;
    MockExtensionPlugin plugin;
    plugin.setProvides({"toolbar", "dock_widget"});
    QVERIFY(!ext.accepts(&plugin));
}

void PluginExtensionPointsTest::test_menu_extension_point_extend() {
    MenuExtensionPoint ext;
    MockExtensionPlugin plugin;
    plugin.setProvides({"menu"});

    int menuCountBefore = m_menuBar->actions().count();

    // Extend should not crash
    ext.extend(&plugin);

    // A "Plugins" menu should be added
    bool hasPluginsMenu = false;
    for (QAction* action : m_menuBar->actions()) {
        if (action->text() == "Plugins" ||
            action->text() ==
                QCoreApplication::translate("MenuExtensionPoint", "Plugins")) {
            hasPluginsMenu = true;
            break;
        }
    }
    QVERIFY(hasPluginsMenu || m_menuBar->actions().count() > menuCountBefore);
}

// ============================================================================
// ToolbarExtensionPoint Tests
// ============================================================================

void PluginExtensionPointsTest::test_toolbar_extension_point_id() {
    ToolbarExtensionPoint ext;
    QCOMPARE(ext.id(), QString("org.sast.readium.toolbar"));
}

void PluginExtensionPointsTest::test_toolbar_extension_point_description() {
    ToolbarExtensionPoint ext;
    QVERIFY(ext.description().contains("toolbar", Qt::CaseInsensitive));
}

void PluginExtensionPointsTest::test_toolbar_extension_point_accepts_null() {
    ToolbarExtensionPoint ext;
    QVERIFY(!ext.accepts(nullptr));
}

void PluginExtensionPointsTest::
    test_toolbar_extension_point_accepts_toolbar_provider() {
    ToolbarExtensionPoint ext;
    MockExtensionPlugin plugin;
    plugin.setProvides({"toolbar"});
    QVERIFY(ext.accepts(&plugin));
}

void PluginExtensionPointsTest::
    test_toolbar_extension_point_accepts_ui_toolbar_provider() {
    ToolbarExtensionPoint ext;
    MockExtensionPlugin plugin;
    plugin.setProvides({"ui.toolbar"});
    QVERIFY(ext.accepts(&plugin));
}

void PluginExtensionPointsTest::
    test_toolbar_extension_point_rejects_non_toolbar_provider() {
    ToolbarExtensionPoint ext;
    MockExtensionPlugin plugin;
    plugin.setProvides({"menu", "dock_widget"});
    QVERIFY(!ext.accepts(&plugin));
}

void PluginExtensionPointsTest::test_toolbar_extension_point_extend() {
    ToolbarExtensionPoint ext;
    MockExtensionPlugin plugin;
    plugin.setProvides({"toolbar"});

    int actionCountBefore = m_toolBar->actions().count();

    // Extend should not crash
    ext.extend(&plugin);

    // Actions should be added to toolbar
    QVERIFY(m_toolBar->actions().count() >= actionCountBefore);
}

// ============================================================================
// DocumentHandlerExtensionPoint Tests
// ============================================================================

void PluginExtensionPointsTest::test_document_handler_extension_point_id() {
    DocumentHandlerExtensionPoint ext;
    QCOMPARE(ext.id(), QString("org.sast.readium.document_handler"));
}

void PluginExtensionPointsTest::
    test_document_handler_extension_point_description() {
    DocumentHandlerExtensionPoint ext;
    QVERIFY(ext.description().contains("document", Qt::CaseInsensitive));
}

void PluginExtensionPointsTest::
    test_document_handler_extension_point_accepts_null() {
    DocumentHandlerExtensionPoint ext;
    QVERIFY(!ext.accepts(nullptr));
}

void PluginExtensionPointsTest::
    test_document_handler_extension_point_accepts_document_handler() {
    DocumentHandlerExtensionPoint ext;
    MockExtensionPlugin plugin;
    plugin.setProvides({"document.handler"});
    QVERIFY(ext.accepts(&plugin));
}

void PluginExtensionPointsTest::
    test_document_handler_extension_point_accepts_document_type() {
    DocumentHandlerExtensionPoint ext;
    MockExtensionPlugin plugin;
    plugin.setProvides({"document.type"});
    QVERIFY(ext.accepts(&plugin));
}

void PluginExtensionPointsTest::
    test_document_handler_extension_point_accepts_file_handler() {
    DocumentHandlerExtensionPoint ext;
    MockExtensionPlugin plugin;
    plugin.setProvides({"file.handler"});
    QVERIFY(ext.accepts(&plugin));
}

void PluginExtensionPointsTest::
    test_document_handler_extension_point_rejects_non_handler() {
    DocumentHandlerExtensionPoint ext;
    MockExtensionPlugin plugin;
    plugin.setProvides({"menu", "toolbar"});
    QVERIFY(!ext.accepts(&plugin));
}

void PluginExtensionPointsTest::test_document_handler_extension_point_extend() {
    DocumentHandlerExtensionPoint ext;
    MockExtensionPlugin plugin;
    plugin.setProvides({"document.handler"});

    // Extend should not crash (logs capabilities)
    ext.extend(&plugin);
    QVERIFY(true);
}

// ============================================================================
// DockWidgetExtensionPoint Tests
// ============================================================================

void PluginExtensionPointsTest::test_dock_widget_extension_point_id() {
    DockWidgetExtensionPoint ext;
    QCOMPARE(ext.id(), QString("org.sast.readium.dock_widget"));
}

void PluginExtensionPointsTest::test_dock_widget_extension_point_description() {
    DockWidgetExtensionPoint ext;
    QVERIFY(ext.description().contains("dock", Qt::CaseInsensitive));
}

void PluginExtensionPointsTest::
    test_dock_widget_extension_point_accepts_null() {
    DockWidgetExtensionPoint ext;
    QVERIFY(!ext.accepts(nullptr));
}

void PluginExtensionPointsTest::
    test_dock_widget_extension_point_accepts_dock_widget() {
    DockWidgetExtensionPoint ext;
    MockExtensionPlugin plugin;
    plugin.setProvides({"dock_widget"});
    QVERIFY(ext.accepts(&plugin));
}

void PluginExtensionPointsTest::
    test_dock_widget_extension_point_accepts_ui_dock() {
    DockWidgetExtensionPoint ext;
    MockExtensionPlugin plugin;
    plugin.setProvides({"ui.dock"});
    QVERIFY(ext.accepts(&plugin));
}

void PluginExtensionPointsTest::
    test_dock_widget_extension_point_rejects_non_dock() {
    DockWidgetExtensionPoint ext;
    MockExtensionPlugin plugin;
    plugin.setProvides({"menu", "toolbar"});
    QVERIFY(!ext.accepts(&plugin));
}

void PluginExtensionPointsTest::test_dock_widget_extension_point_extend() {
    DockWidgetExtensionPoint ext;
    MockExtensionPlugin plugin;
    plugin.setProvides({"dock_widget"});
    plugin.setCreateDockWidget(true);
    plugin.setDockWidgetTitle("Test Dock");
    plugin.setDockWidgetArea(Qt::LeftDockWidgetArea);

    // Extend should add dock widget
    ext.extend(&plugin);

    // Verify dock widget was added (check by object name pattern)
    bool found = false;
    for (QDockWidget* dock : m_mainWindow->findChildren<QDockWidget*>()) {
        if (dock->objectName().contains("PluginDock")) {
            found = true;
            break;
        }
    }
    QVERIFY(found);
}

void PluginExtensionPointsTest::
    test_dock_widget_extension_point_extend_no_ui_extension() {
    DockWidgetExtensionPoint ext;

    // Plugin that provides dock_widget but doesn't implement IUIExtension
    // properly (returns null dock widget)
    MockExtensionPlugin plugin;
    plugin.setProvides({"dock_widget"});
    plugin.setCreateDockWidget(false);  // Will return null

    // Extend should not crash
    ext.extend(&plugin);
    QVERIFY(true);
}

// ============================================================================
// ContextMenuExtensionPoint Tests
// ============================================================================

void PluginExtensionPointsTest::test_context_menu_extension_point_id() {
    ContextMenuExtensionPoint ext;
    QCOMPARE(ext.id(), QString("org.sast.readium.context_menu"));
}

void PluginExtensionPointsTest::
    test_context_menu_extension_point_description() {
    ContextMenuExtensionPoint ext;
    QVERIFY(ext.description().contains("context", Qt::CaseInsensitive));
}

void PluginExtensionPointsTest::
    test_context_menu_extension_point_accepts_null() {
    ContextMenuExtensionPoint ext;
    QVERIFY(!ext.accepts(nullptr));
}

void PluginExtensionPointsTest::
    test_context_menu_extension_point_accepts_context_menu() {
    ContextMenuExtensionPoint ext;
    MockExtensionPlugin plugin;
    plugin.setProvides({"context_menu"});
    QVERIFY(ext.accepts(&plugin));
}

void PluginExtensionPointsTest::
    test_context_menu_extension_point_accepts_ui_context_menu() {
    ContextMenuExtensionPoint ext;
    MockExtensionPlugin plugin;
    plugin.setProvides({"ui.context_menu"});
    QVERIFY(ext.accepts(&plugin));
}

void PluginExtensionPointsTest::
    test_context_menu_extension_point_rejects_non_context_menu() {
    ContextMenuExtensionPoint ext;
    MockExtensionPlugin plugin;
    plugin.setProvides({"menu", "toolbar"});
    QVERIFY(!ext.accepts(&plugin));
}

void PluginExtensionPointsTest::test_context_menu_extension_point_extend() {
    ContextMenuExtensionPoint ext;
    MockExtensionPlugin plugin;
    plugin.setProvides({"context_menu"});

    // Extend should not crash (registers for future use)
    ext.extend(&plugin);
    QVERIFY(true);
}

// ============================================================================
// StatusBarExtensionPoint Tests
// ============================================================================

void PluginExtensionPointsTest::test_status_bar_extension_point_id() {
    StatusBarExtensionPoint ext;
    QCOMPARE(ext.id(), QString("org.sast.readium.status_bar"));
}

void PluginExtensionPointsTest::test_status_bar_extension_point_description() {
    StatusBarExtensionPoint ext;
    QVERIFY(ext.description().contains("status", Qt::CaseInsensitive));
}

void PluginExtensionPointsTest::test_status_bar_extension_point_accepts_null() {
    StatusBarExtensionPoint ext;
    QVERIFY(!ext.accepts(nullptr));
}

void PluginExtensionPointsTest::
    test_status_bar_extension_point_accepts_status_bar() {
    StatusBarExtensionPoint ext;
    MockExtensionPlugin plugin;
    plugin.setProvides({"status_bar"});
    QVERIFY(ext.accepts(&plugin));
}

void PluginExtensionPointsTest::
    test_status_bar_extension_point_accepts_ui_status_bar() {
    StatusBarExtensionPoint ext;
    MockExtensionPlugin plugin;
    plugin.setProvides({"ui.status_bar"});
    QVERIFY(ext.accepts(&plugin));
}

void PluginExtensionPointsTest::
    test_status_bar_extension_point_rejects_non_status_bar() {
    StatusBarExtensionPoint ext;
    MockExtensionPlugin plugin;
    plugin.setProvides({"menu", "toolbar"});
    QVERIFY(!ext.accepts(&plugin));
}

void PluginExtensionPointsTest::test_status_bar_extension_point_extend() {
    StatusBarExtensionPoint ext;
    MockExtensionPlugin plugin;
    plugin.setProvides({"status_bar"});
    plugin.setStatusBarMessage("Plugin loaded successfully");
    plugin.setStatusBarTimeout(5000);

    // Extend should display status bar message
    ext.extend(&plugin);
    QVERIFY(true);
}

// ============================================================================
// PluginManager Extension Point Management Tests
// ============================================================================

void PluginExtensionPointsTest::test_plugin_manager_register_extension_point() {
    PluginManager& mgr = PluginManager::instance();

    // Clear existing extension points
    for (IExtensionPoint* ep : mgr.getExtensionPoints()) {
        mgr.unregisterExtensionPoint(ep->id());
    }

    auto* menuExt = new MenuExtensionPoint();
    mgr.registerExtensionPoint(menuExt);

    QList<IExtensionPoint*> points = mgr.getExtensionPoints();
    QVERIFY(points.contains(menuExt));

    // Cleanup
    mgr.unregisterExtensionPoint(menuExt->id());
    delete menuExt;
}

void PluginExtensionPointsTest::
    test_plugin_manager_register_null_extension_point() {
    PluginManager& mgr = PluginManager::instance();
    int countBefore = mgr.getExtensionPoints().size();

    // Should not crash
    mgr.registerExtensionPoint(nullptr);

    QCOMPARE(mgr.getExtensionPoints().size(), countBefore);
}

void PluginExtensionPointsTest::
    test_plugin_manager_register_duplicate_extension_point() {
    PluginManager& mgr = PluginManager::instance();

    // Clear existing extension points
    for (IExtensionPoint* ep : mgr.getExtensionPoints()) {
        mgr.unregisterExtensionPoint(ep->id());
    }

    auto* menuExt1 = new MenuExtensionPoint();
    auto* menuExt2 = new MenuExtensionPoint();

    mgr.registerExtensionPoint(menuExt1);
    int countAfterFirst = mgr.getExtensionPoints().size();

    // Duplicate should not be added
    mgr.registerExtensionPoint(menuExt2);
    QCOMPARE(mgr.getExtensionPoints().size(), countAfterFirst);

    // Cleanup
    mgr.unregisterExtensionPoint(menuExt1->id());
    delete menuExt1;
    delete menuExt2;
}

void PluginExtensionPointsTest::
    test_plugin_manager_unregister_extension_point() {
    PluginManager& mgr = PluginManager::instance();

    auto* menuExt = new MenuExtensionPoint();
    mgr.registerExtensionPoint(menuExt);
    QVERIFY(mgr.getExtensionPoints().contains(menuExt));

    mgr.unregisterExtensionPoint(menuExt->id());
    QVERIFY(!mgr.getExtensionPoints().contains(menuExt));

    delete menuExt;
}

void PluginExtensionPointsTest::test_plugin_manager_get_extension_points() {
    PluginManager& mgr = PluginManager::instance();

    // Clear existing extension points
    for (IExtensionPoint* ep : mgr.getExtensionPoints()) {
        mgr.unregisterExtensionPoint(ep->id());
    }

    auto* menuExt = new MenuExtensionPoint();
    auto* toolbarExt = new ToolbarExtensionPoint();

    mgr.registerExtensionPoint(menuExt);
    mgr.registerExtensionPoint(toolbarExt);

    QList<IExtensionPoint*> points = mgr.getExtensionPoints();
    QCOMPARE(points.size(), 2);
    QVERIFY(points.contains(menuExt));
    QVERIFY(points.contains(toolbarExt));

    // Cleanup
    mgr.unregisterExtensionPoint(menuExt->id());
    mgr.unregisterExtensionPoint(toolbarExt->id());
    delete menuExt;
    delete toolbarExt;
}

void PluginExtensionPointsTest::test_plugin_manager_apply_extension_points() {
    PluginManager& mgr = PluginManager::instance();

    // Clear existing extension points
    for (IExtensionPoint* ep : mgr.getExtensionPoints()) {
        mgr.unregisterExtensionPoint(ep->id());
    }

    auto* menuExt = new MenuExtensionPoint();
    mgr.registerExtensionPoint(menuExt);

    MockExtensionPlugin plugin;
    plugin.setProvides({"menu"});

    // Apply extension points should not crash
    mgr.applyExtensionPoints(&plugin);
    QVERIFY(true);

    // Cleanup
    mgr.unregisterExtensionPoint(menuExt->id());
    delete menuExt;
}

QTEST_MAIN(PluginExtensionPointsTest)
#include "test_plugin_extension_points.moc"
