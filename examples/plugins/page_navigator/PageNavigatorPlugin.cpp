#include "PageNavigatorPlugin.h"
#include <QAction>
#include <QInputDialog>
#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>
#include "controller/EventBus.h"
#include "plugin/PluginHookRegistry.h"

PageNavigatorPlugin::PageNavigatorPlugin(QObject* parent)
    : PluginBase(parent),
      m_maxHistorySize(50),
      m_currentPage(1),
      m_totalPages(1),
      m_backAction(nullptr),
      m_forwardAction(nullptr),
      m_navigations(0) {
    m_metadata.name = "Page Navigator";
    m_metadata.version = "1.0.0";
    m_metadata.description =
        "Advanced page navigation with history, quick jump, and visual tools";
    m_metadata.author = "SAST Readium Team";
    m_metadata.dependencies = QStringList();
    m_capabilities.provides = QStringList()
                              << "navigation.history" << "navigation.quickjump"
                              << "ui.dock" << "ui.toolbar";
}

PageNavigatorPlugin::~PageNavigatorPlugin() {
    qDeleteAll(m_menuActions);
    qDeleteAll(m_toolbarActions);
}

bool PageNavigatorPlugin::onInitialize() {
    m_logger.info("PageNavigatorPlugin: Initializing...");

    m_maxHistorySize = m_configuration.value("maxHistorySize").toInt(50);

    // Back action
    m_backAction = new QAction("Back", this);
    m_backAction->setShortcut(QKeySequence("Alt+Left"));
    m_backAction->setEnabled(false);
    connect(m_backAction, &QAction::triggered, this,
            &PageNavigatorPlugin::onGoBack);
    m_menuActions.append(m_backAction);
    m_toolbarActions.append(m_backAction);

    // Forward action
    m_forwardAction = new QAction("Forward", this);
    m_forwardAction->setShortcut(QKeySequence("Alt+Right"));
    m_forwardAction->setEnabled(false);
    connect(m_forwardAction, &QAction::triggered, this,
            &PageNavigatorPlugin::onGoForward);
    m_menuActions.append(m_forwardAction);
    m_toolbarActions.append(m_forwardAction);

    // Separator
    QAction* sep1 = new QAction(this);
    sep1->setSeparator(true);
    m_menuActions.append(sep1);

    // Go to page
    QAction* goToAction = new QAction("Go to Page...", this);
    goToAction->setShortcut(QKeySequence("Ctrl+G"));
    connect(goToAction, &QAction::triggered, this,
            &PageNavigatorPlugin::onGoToPage);
    m_menuActions.append(goToAction);

    // First/Last page
    QAction* firstAction = new QAction("First Page", this);
    firstAction->setShortcut(QKeySequence("Home"));
    connect(firstAction, &QAction::triggered, this,
            &PageNavigatorPlugin::onGoToFirstPage);
    m_menuActions.append(firstAction);

    QAction* lastAction = new QAction("Last Page", this);
    lastAction->setShortcut(QKeySequence("End"));
    connect(lastAction, &QAction::triggered, this,
            &PageNavigatorPlugin::onGoToLastPage);
    m_menuActions.append(lastAction);

    registerHooks();
    setupEventSubscriptions();

    m_logger.info("PageNavigatorPlugin: Initialized successfully");
    return true;
}

void PageNavigatorPlugin::onShutdown() {
    m_logger.info("PageNavigatorPlugin: Shutting down...");
    PluginHookRegistry::instance().unregisterAllCallbacks(name());
    eventBus()->unsubscribeAll(this);
    m_logger.info(QString("PageNavigatorPlugin: Total navigations: %1")
                      .arg(m_navigations));
}

void PageNavigatorPlugin::handleMessage(const QString& from,
                                        const QVariant& message) {
    QVariantMap msgMap = message.toMap();
    QString action = msgMap.value("action").toString();

    if (action == "go_to_page") {
        int page = msgMap.value("pageNumber").toInt();
        goToPage(page);
    } else if (action == "go_to_percentage") {
        double pct = msgMap.value("percentage").toDouble();
        goToPercentage(pct);
    } else if (action == "go_back") {
        goBack();
    } else if (action == "go_forward") {
        goForward();
    } else if (action == "get_history") {
        Event* resp = new Event("plugin.response");
        QVariantMap data;
        data["from"] = name();
        data["to"] = from;
        data["canGoBack"] = canGoBack();
        data["canGoForward"] = canGoForward();
        data["historySize"] = m_historyBack.size();
        data["currentPage"] = m_currentPage;
        data["totalPages"] = m_totalPages;
        resp->setData(QVariant::fromValue(data));
        eventBus()->publish(resp);
    }
}

// ============================================================================
// IUIExtension
// ============================================================================

QList<QAction*> PageNavigatorPlugin::menuActions() const {
    return m_menuActions;
}

QList<QAction*> PageNavigatorPlugin::toolbarActions() const {
    return m_toolbarActions;
}

QString PageNavigatorPlugin::statusBarMessage() const {
    if (m_totalPages > 0) {
        double pct =
            (static_cast<double>(m_currentPage) / m_totalPages) * 100.0;
        return QString("Page %1/%2 (%3%)")
            .arg(m_currentPage)
            .arg(m_totalPages)
            .arg(pct, 0, 'f', 1);
    }
    return QString();
}

QWidget* PageNavigatorPlugin::createDockWidget() {
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);

    QLabel* titleLabel = new QLabel("<b>Page Navigator</b>");
    layout->addWidget(titleLabel);

    QLabel* pageLabel = new QLabel("Page: 1/1");
    pageLabel->setObjectName("pageLabel");
    layout->addWidget(pageLabel);

    QSlider* pageSlider = new QSlider(Qt::Horizontal);
    pageSlider->setObjectName("pageSlider");
    pageSlider->setMinimum(1);
    pageSlider->setMaximum(1);
    pageSlider->setValue(1);
    layout->addWidget(pageSlider);

    QLabel* historyLabel = new QLabel("History: 0 items");
    historyLabel->setObjectName("historyLabel");
    layout->addWidget(historyLabel);

    layout->addStretch();
    return widget;
}

// ============================================================================
// Navigation API
// ============================================================================

void PageNavigatorPlugin::goToPage(int pageNumber) {
    if (pageNumber < 1 || pageNumber > m_totalPages ||
        pageNumber == m_currentPage) {
        return;
    }

    recordNavigation(m_currentPage);
    m_currentPage = pageNumber;
    m_navigations++;

    Event* event = new Event("navigation.goToPage");
    QVariantMap data;
    data["pageNumber"] = pageNumber;
    event->setData(QVariant::fromValue(data));
    eventBus()->publish(event);

    m_backAction->setEnabled(canGoBack());
    m_forwardAction->setEnabled(canGoForward());
}

void PageNavigatorPlugin::goToPercentage(double percentage) {
    int page = qBound(1, static_cast<int>(m_totalPages * percentage / 100.0),
                      m_totalPages);
    goToPage(page);
}

void PageNavigatorPlugin::goBack() {
    if (!canGoBack())
        return;

    NavigationEntry entry = m_historyBack.takeLast();

    NavigationEntry current;
    current.pageNumber = m_currentPage;
    current.documentPath = m_currentDocument;
    current.timestamp = QDateTime::currentDateTime();
    m_historyForward.append(current);

    m_currentPage = entry.pageNumber;
    m_navigations++;

    Event* event = new Event("navigation.goToPage");
    QVariantMap data;
    data["pageNumber"] = entry.pageNumber;
    data["fromHistory"] = true;
    event->setData(QVariant::fromValue(data));
    eventBus()->publish(event);

    m_backAction->setEnabled(canGoBack());
    m_forwardAction->setEnabled(canGoForward());
}

void PageNavigatorPlugin::goForward() {
    if (!canGoForward())
        return;

    NavigationEntry entry = m_historyForward.takeLast();

    NavigationEntry current;
    current.pageNumber = m_currentPage;
    current.documentPath = m_currentDocument;
    current.timestamp = QDateTime::currentDateTime();
    m_historyBack.append(current);

    m_currentPage = entry.pageNumber;
    m_navigations++;

    Event* event = new Event("navigation.goToPage");
    QVariantMap data;
    data["pageNumber"] = entry.pageNumber;
    data["fromHistory"] = true;
    event->setData(QVariant::fromValue(data));
    eventBus()->publish(event);

    m_backAction->setEnabled(canGoBack());
    m_forwardAction->setEnabled(canGoForward());
}

void PageNavigatorPlugin::goToFirstPage() { goToPage(1); }

void PageNavigatorPlugin::goToLastPage() { goToPage(m_totalPages); }

bool PageNavigatorPlugin::canGoBack() const { return !m_historyBack.isEmpty(); }

bool PageNavigatorPlugin::canGoForward() const {
    return !m_historyForward.isEmpty();
}

void PageNavigatorPlugin::recordNavigation(int pageNumber) {
    NavigationEntry entry;
    entry.pageNumber = pageNumber;
    entry.documentPath = m_currentDocument;
    entry.timestamp = QDateTime::currentDateTime();

    m_historyBack.append(entry);
    m_historyForward.clear();

    while (m_historyBack.size() > m_maxHistorySize) {
        m_historyBack.removeFirst();
    }
}

// ============================================================================
// Slots
// ============================================================================

void PageNavigatorPlugin::onGoBack() { goBack(); }

void PageNavigatorPlugin::onGoForward() { goForward(); }

void PageNavigatorPlugin::onGoToPage() {
    Event* event = new Event("ui.showGoToPageDialog");
    QVariantMap data;
    data["currentPage"] = m_currentPage;
    data["totalPages"] = m_totalPages;
    event->setData(QVariant::fromValue(data));
    eventBus()->publish(event);
}

void PageNavigatorPlugin::onGoToFirstPage() { goToFirstPage(); }

void PageNavigatorPlugin::onGoToLastPage() { goToLastPage(); }

// ============================================================================
// Hooks & Events
// ============================================================================

void PageNavigatorPlugin::registerHooks() {
    auto& registry = PluginHookRegistry::instance();
    registry.registerCallback(
        "page.changed", name(),
        [this](const QVariantMap& ctx) { return onPageChanged(ctx); });
}

void PageNavigatorPlugin::setupEventSubscriptions() {
    eventBus()->subscribe("document.opened", this, [this](Event* event) {
        QVariantMap data = event->data().toMap();
        m_currentDocument = data.value("filePath").toString();
        m_totalPages = data.value("pageCount").toInt(1);
        m_currentPage = 1;
        m_historyBack.clear();
        m_historyForward.clear();
        m_backAction->setEnabled(false);
        m_forwardAction->setEnabled(false);
    });

    eventBus()->subscribe("document.closed", this, [this](Event*) {
        m_currentDocument.clear();
        m_totalPages = 1;
        m_currentPage = 1;
    });

    eventBus()->subscribe("page.changed", this, [this](Event* event) {
        int newPage = event->data().toInt();
        if (newPage != m_currentPage && newPage >= 1 &&
            newPage <= m_totalPages) {
            recordNavigation(m_currentPage);
            m_currentPage = newPage;
            m_backAction->setEnabled(canGoBack());
            m_forwardAction->setEnabled(canGoForward());
        }
    });
}

QVariant PageNavigatorPlugin::onPageChanged(const QVariantMap& context) {
    int page = context.value("pageNumber").toInt();
    QVariantMap result;
    result["previousPage"] = m_currentPage;
    result["newPage"] = page;
    result["historySize"] = m_historyBack.size();
    return result;
}
