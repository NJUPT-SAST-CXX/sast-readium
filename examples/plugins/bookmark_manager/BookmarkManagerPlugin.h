#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include "plugin/PluginInterface.h"

/**
 * @brief Bookmark data structure
 */
struct Bookmark {
    QString id;
    QString documentPath;
    int pageNumber;
    QString title;
    QString description;
    QString category;
    QColor color;
    QDateTime createdAt;
    QDateTime modifiedAt;
};

/**
 * @brief BookmarkManagerPlugin - Bookmark management plugin
 *
 * This plugin demonstrates:
 * - **Bookmark CRUD**: Create, read, update, delete bookmarks
 * - **Categories**: Organize bookmarks by category
 * - **Navigation**: Quick jump to bookmarked pages
 * - **Persistence**: Save/load bookmarks to JSON
 * - **Sync Simulation**: Mock cloud synchronization
 * - **Dock Widget**: Bookmark panel UI
 */
class BookmarkManagerPlugin : public PluginBase, public IUIExtension {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.sast.readium.IPlugin/1.0" FILE
                          "bookmark_manager.json")
    Q_INTERFACES(IPluginInterface IUIExtension)

public:
    explicit BookmarkManagerPlugin(QObject* parent = nullptr);
    ~BookmarkManagerPlugin() override;

    void handleMessage(const QString& from, const QVariant& message) override;

    // IUIExtension
    QList<QAction*> menuActions() const override;
    QList<QAction*> toolbarActions() const override;
    QList<QAction*> contextMenuActions() const override;
    QString statusBarMessage() const override;
    QWidget* createDockWidget() override;
    QString menuPath() const override { return "Bookmarks"; }
    QString toolbarId() const override { return "navigation_toolbar"; }

    // Bookmark management
    QString addBookmark(const Bookmark& bookmark);
    bool updateBookmark(const QString& id, const Bookmark& bookmark);
    bool deleteBookmark(const QString& id);
    Bookmark getBookmark(const QString& id) const;
    QList<Bookmark> getBookmarksForDocument(const QString& documentPath) const;
    QList<Bookmark> getBookmarksByCategory(const QString& category) const;
    QStringList getCategories() const;

protected:
    bool onInitialize() override;
    void onShutdown() override;

private slots:
    void onAddBookmark();
    void onShowBookmarks();
    void onSyncBookmarks();

private:
    void registerHooks();
    void setupEventSubscriptions();
    void loadBookmarks();
    void saveBookmarks();
    bool syncToCloud();
    bool syncFromCloud();

    QVariant onBookmarkCreated(const QVariantMap& context);
    QVariant onPageViewed(const QVariantMap& context);

    // Storage
    QHash<QString, Bookmark> m_bookmarks;
    QString m_currentDocument;

    // UI
    QList<QAction*> m_menuActions;
    QList<QAction*> m_toolbarActions;
    QList<QAction*> m_contextActions;
    QAction* m_addBookmarkAction;

    // Config
    QString m_storageFile;
    bool m_autoSync;

    // Stats
    int m_bookmarksCreated;
};
