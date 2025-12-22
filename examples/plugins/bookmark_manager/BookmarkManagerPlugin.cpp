#include "BookmarkManagerPlugin.h"
#include <QAction>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QStandardPaths>
#include <QUuid>
#include <QVBoxLayout>
#include "controller/EventBus.h"
#include "plugin/PluginHookRegistry.h"

BookmarkManagerPlugin::BookmarkManagerPlugin(QObject* parent)
    : PluginBase(parent),
      m_addBookmarkAction(nullptr),
      m_autoSync(false),
      m_bookmarksCreated(0) {
    m_metadata.name = "Bookmark Manager";
    m_metadata.version = "1.0.0";
    m_metadata.description =
        "Bookmark management with categories and cloud sync";
    m_metadata.author = "SAST Readium Team";
    m_metadata.dependencies = QStringList();
    m_capabilities.provides = QStringList()
                              << "bookmark.manager" << "bookmark.sync"
                              << "ui.dock" << "navigation";
}

BookmarkManagerPlugin::~BookmarkManagerPlugin() {
    qDeleteAll(m_menuActions);
    qDeleteAll(m_toolbarActions);
    qDeleteAll(m_contextActions);
}

bool BookmarkManagerPlugin::onInitialize() {
    m_logger.info("BookmarkManagerPlugin: Initializing...");

    m_storageFile =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
        "/bookmarks.json";
    m_autoSync = m_configuration.value("autoSync").toBool(false);

    // Load saved bookmarks
    loadBookmarks();

    // Create actions
    m_addBookmarkAction = new QAction("Add Bookmark", this);
    m_addBookmarkAction->setShortcut(QKeySequence("Ctrl+D"));
    connect(m_addBookmarkAction, &QAction::triggered, this,
            &BookmarkManagerPlugin::onAddBookmark);
    m_menuActions.append(m_addBookmarkAction);
    m_toolbarActions.append(m_addBookmarkAction);

    QAction* showAction = new QAction("Show Bookmarks", this);
    showAction->setShortcut(QKeySequence("Ctrl+Shift+B"));
    connect(showAction, &QAction::triggered, this,
            &BookmarkManagerPlugin::onShowBookmarks);
    m_menuActions.append(showAction);

    QAction* syncAction = new QAction("Sync Bookmarks", this);
    connect(syncAction, &QAction::triggered, this,
            &BookmarkManagerPlugin::onSyncBookmarks);
    m_menuActions.append(syncAction);

    // Context menu action
    QAction* contextAdd = new QAction("Bookmark This Page", this);
    connect(contextAdd, &QAction::triggered, this,
            &BookmarkManagerPlugin::onAddBookmark);
    m_contextActions.append(contextAdd);

    registerHooks();
    setupEventSubscriptions();

    m_logger.info(QString("BookmarkManagerPlugin: Loaded %1 bookmarks")
                      .arg(m_bookmarks.size()));
    return true;
}

void BookmarkManagerPlugin::onShutdown() {
    m_logger.info("BookmarkManagerPlugin: Shutting down...");
    saveBookmarks();
    PluginHookRegistry::instance().unregisterAllCallbacks(name());
    eventBus()->unsubscribeAll(this);
    m_logger.info(QString("BookmarkManagerPlugin: Created %1 bookmarks")
                      .arg(m_bookmarksCreated));
}

void BookmarkManagerPlugin::handleMessage(const QString& from,
                                          const QVariant& message) {
    QVariantMap msgMap = message.toMap();
    QString action = msgMap.value("action").toString();

    if (action == "add") {
        Bookmark bm;
        bm.documentPath = msgMap.value("documentPath").toString();
        bm.pageNumber = msgMap.value("pageNumber").toInt();
        bm.title = msgMap.value("title").toString();
        bm.category = msgMap.value("category").toString("Default");

        QString bmId = addBookmark(bm);

        Event* resp = new Event("plugin.response");
        QVariantMap data;
        data["from"] = name();
        data["to"] = from;
        data["success"] = !bmId.isEmpty();
        data["bookmarkId"] = bmId;
        resp->setData(QVariant::fromValue(data));
        eventBus()->publish(resp);

    } else if (action == "delete") {
        QString bmId = msgMap.value("bookmarkId").toString();
        bool success = deleteBookmark(bmId);

        Event* resp = new Event("plugin.response");
        QVariantMap data;
        data["from"] = name();
        data["to"] = from;
        data["success"] = success;
        resp->setData(QVariant::fromValue(data));
        eventBus()->publish(resp);

    } else if (action == "get_bookmarks") {
        QString docPath = msgMap.value("documentPath").toString();
        QList<Bookmark> bookmarks = docPath.isEmpty()
                                        ? m_bookmarks.values()
                                        : getBookmarksForDocument(docPath);

        Event* resp = new Event("plugin.response");
        QVariantMap data;
        data["from"] = name();
        data["to"] = from;

        QJsonArray arr;
        for (const auto& bm : bookmarks) {
            QJsonObject obj;
            obj["id"] = bm.id;
            obj["title"] = bm.title;
            obj["pageNumber"] = bm.pageNumber;
            obj["category"] = bm.category;
            arr.append(obj);
        }
        data["bookmarks"] = arr.toVariantList();
        resp->setData(QVariant::fromValue(data));
        eventBus()->publish(resp);

    } else if (action == "navigate") {
        QString bmId = msgMap.value("bookmarkId").toString();
        if (m_bookmarks.contains(bmId)) {
            const Bookmark& bm = m_bookmarks[bmId];
            Event* navEvent = new Event("navigation.goToPage");
            QVariantMap navData;
            navData["pageNumber"] = bm.pageNumber;
            navData["documentPath"] = bm.documentPath;
            navEvent->setData(QVariant::fromValue(navData));
            eventBus()->publish(navEvent);
        }
    }
}

// ============================================================================
// IUIExtension
// ============================================================================

QList<QAction*> BookmarkManagerPlugin::menuActions() const {
    return m_menuActions;
}

QList<QAction*> BookmarkManagerPlugin::toolbarActions() const {
    return m_toolbarActions;
}

QList<QAction*> BookmarkManagerPlugin::contextMenuActions() const {
    return m_contextActions;
}

QString BookmarkManagerPlugin::statusBarMessage() const {
    if (m_currentDocument.isEmpty()) {
        return QString();
    }
    int count = getBookmarksForDocument(m_currentDocument).size();
    return count > 0 ? QString("Bookmarks: %1").arg(count) : QString();
}

QWidget* BookmarkManagerPlugin::createDockWidget() {
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);

    QLabel* titleLabel = new QLabel("<b>Bookmarks</b>");
    layout->addWidget(titleLabel);

    QListWidget* listWidget = new QListWidget();
    listWidget->setObjectName("bookmarkList");
    layout->addWidget(listWidget);

    // Populate with current bookmarks
    for (const auto& bm : m_bookmarks) {
        listWidget->addItem(
            QString("%1 - Page %2").arg(bm.title).arg(bm.pageNumber));
    }

    return widget;
}

// ============================================================================
// Bookmark Management
// ============================================================================

QString BookmarkManagerPlugin::addBookmark(const Bookmark& bookmark) {
    Bookmark newBm = bookmark;
    newBm.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    newBm.createdAt = QDateTime::currentDateTime();
    newBm.modifiedAt = newBm.createdAt;

    if (newBm.title.isEmpty()) {
        newBm.title = QString("Page %1").arg(newBm.pageNumber);
    }
    if (newBm.category.isEmpty()) {
        newBm.category = "Default";
    }

    m_bookmarks[newBm.id] = newBm;
    m_bookmarksCreated++;

    // Publish event
    Event* event = new Event("bookmark.created");
    QVariantMap data;
    data["bookmarkId"] = newBm.id;
    data["pageNumber"] = newBm.pageNumber;
    data["title"] = newBm.title;
    event->setData(QVariant::fromValue(data));
    eventBus()->publish(event);

    m_logger.info(
        QString("BookmarkManagerPlugin: Added bookmark '%1'").arg(newBm.title));

    if (m_autoSync) {
        syncToCloud();
    }

    return newBm.id;
}

bool BookmarkManagerPlugin::updateBookmark(const QString& id,
                                           const Bookmark& bookmark) {
    if (!m_bookmarks.contains(id)) {
        return false;
    }

    Bookmark updated = bookmark;
    updated.id = id;
    updated.modifiedAt = QDateTime::currentDateTime();
    m_bookmarks[id] = updated;

    return true;
}

bool BookmarkManagerPlugin::deleteBookmark(const QString& id) {
    if (!m_bookmarks.contains(id)) {
        return false;
    }

    m_bookmarks.remove(id);

    Event* event = new Event("bookmark.deleted");
    QVariantMap data;
    data["bookmarkId"] = id;
    event->setData(QVariant::fromValue(data));
    eventBus()->publish(event);

    return true;
}

Bookmark BookmarkManagerPlugin::getBookmark(const QString& id) const {
    return m_bookmarks.value(id);
}

QList<Bookmark> BookmarkManagerPlugin::getBookmarksForDocument(
    const QString& documentPath) const {
    QList<Bookmark> result;
    for (const auto& bm : m_bookmarks) {
        if (bm.documentPath == documentPath) {
            result.append(bm);
        }
    }
    return result;
}

QList<Bookmark> BookmarkManagerPlugin::getBookmarksByCategory(
    const QString& category) const {
    QList<Bookmark> result;
    for (const auto& bm : m_bookmarks) {
        if (bm.category == category) {
            result.append(bm);
        }
    }
    return result;
}

QStringList BookmarkManagerPlugin::getCategories() const {
    QSet<QString> categories;
    for (const auto& bm : m_bookmarks) {
        categories.insert(bm.category);
    }
    return categories.values();
}

// ============================================================================
// Persistence
// ============================================================================

void BookmarkManagerPlugin::loadBookmarks() {
    QFile file(m_storageFile);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    QJsonArray arr = doc.object()["bookmarks"].toArray();
    for (const auto& val : arr) {
        QJsonObject obj = val.toObject();
        Bookmark bm;
        bm.id = obj["id"].toString();
        bm.documentPath = obj["documentPath"].toString();
        bm.pageNumber = obj["pageNumber"].toInt();
        bm.title = obj["title"].toString();
        bm.description = obj["description"].toString();
        bm.category = obj["category"].toString();
        bm.color = QColor(obj["color"].toString());
        bm.createdAt =
            QDateTime::fromString(obj["createdAt"].toString(), Qt::ISODate);
        bm.modifiedAt =
            QDateTime::fromString(obj["modifiedAt"].toString(), Qt::ISODate);

        if (!bm.id.isEmpty()) {
            m_bookmarks[bm.id] = bm;
        }
    }
}

void BookmarkManagerPlugin::saveBookmarks() {
    QJsonArray arr;
    for (const auto& bm : m_bookmarks) {
        QJsonObject obj;
        obj["id"] = bm.id;
        obj["documentPath"] = bm.documentPath;
        obj["pageNumber"] = bm.pageNumber;
        obj["title"] = bm.title;
        obj["description"] = bm.description;
        obj["category"] = bm.category;
        obj["color"] = bm.color.name();
        obj["createdAt"] = bm.createdAt.toString(Qt::ISODate);
        obj["modifiedAt"] = bm.modifiedAt.toString(Qt::ISODate);
        arr.append(obj);
    }

    QJsonObject root;
    root["bookmarks"] = arr;
    root["savedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QFile file(m_storageFile);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        file.close();
    }
}

bool BookmarkManagerPlugin::syncToCloud() {
    m_logger.info("BookmarkManagerPlugin: Simulating cloud upload...");
    // Simulated sync
    return true;
}

bool BookmarkManagerPlugin::syncFromCloud() {
    m_logger.info("BookmarkManagerPlugin: Simulating cloud download...");
    // Simulated sync
    return true;
}

// ============================================================================
// Slots
// ============================================================================

void BookmarkManagerPlugin::onAddBookmark() {
    Event* event = new Event("ui.showAddBookmarkDialog");
    QVariantMap data;
    data["documentPath"] = m_currentDocument;
    event->setData(QVariant::fromValue(data));
    eventBus()->publish(event);
}

void BookmarkManagerPlugin::onShowBookmarks() {
    eventBus()->publish(new Event("ui.showBookmarkPanel"));
}

void BookmarkManagerPlugin::onSyncBookmarks() {
    syncToCloud();
    syncFromCloud();
}

// ============================================================================
// Hooks & Events
// ============================================================================

void BookmarkManagerPlugin::registerHooks() {
    auto& registry = PluginHookRegistry::instance();
    registry.registerCallback(
        "bookmark.created", name(),
        [this](const QVariantMap& ctx) { return onBookmarkCreated(ctx); });
}

void BookmarkManagerPlugin::setupEventSubscriptions() {
    eventBus()->subscribe("document.opened", this, [this](Event* event) {
        m_currentDocument = event->data().toString();
    });

    eventBus()->subscribe("document.closed", this, [this](Event* event) {
        if (m_currentDocument == event->data().toString()) {
            m_currentDocument.clear();
        }
    });

    eventBus()->subscribe("page.viewed", this, [this](Event* event) {
        // Could track recently viewed pages for bookmark suggestions
    });
}

QVariant BookmarkManagerPlugin::onBookmarkCreated(const QVariantMap& context) {
    Q_UNUSED(context)
    QVariantMap result;
    result["acknowledged"] = true;
    result["totalBookmarks"] = m_bookmarks.size();
    return result;
}

QVariant BookmarkManagerPlugin::onPageViewed(const QVariantMap& context) {
    Q_UNUSED(context)
    return QVariant();
}
