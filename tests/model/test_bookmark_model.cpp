#include <QSignalSpy>
#include <QTest>
#include "../../app/model/BookmarkModel.h"
#include "../TestUtilities.h"

class TestBookmarkModel : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void init() {
        m_model = new BookmarkModel();
        m_model->clearAllBookmarks();
    }

    void cleanup() {
        m_model->clearAllBookmarks();
        delete m_model;
        m_model = nullptr;
    }

    void testConstruction() {
        QVERIFY(m_model != nullptr);
        QCOMPARE(m_model->rowCount(), 0);
        QCOMPARE(m_model->getBookmarkCount(), 0);
    }

    void testAddBookmark() {
        QSignalSpy spy(m_model, &BookmarkModel::bookmarkAdded);

        Bookmark bookmark("/path/to/doc.pdf", 5, "Test Bookmark");
        bool result = m_model->addBookmark(bookmark);

        QVERIFY(result);
        QCOMPARE(m_model->rowCount(), 1);
        QCOMPARE(spy.count(), 1);
    }

    void testRemoveBookmark() {
        Bookmark bookmark("/path/to/doc.pdf", 5, "Test Bookmark");
        m_model->addBookmark(bookmark);

        QString id = m_model->getAllBookmarks().first().id;

        QSignalSpy spy(m_model, &BookmarkModel::bookmarkRemoved);

        bool result = m_model->removeBookmark(id);
        QVERIFY(result);
        QCOMPARE(m_model->rowCount(), 0);
        QCOMPARE(spy.count(), 1);
    }

    void testRemoveNonExistentBookmark() {
        bool result = m_model->removeBookmark("non-existent-id");
        QVERIFY(!result);
    }

    void testUpdateBookmark() {
        Bookmark bookmark("/path/to/doc.pdf", 5, "Original Title");
        m_model->addBookmark(bookmark);

        QString id = m_model->getAllBookmarks().first().id;

        QSignalSpy spy(m_model, &BookmarkModel::bookmarkUpdated);

        Bookmark updated = m_model->getBookmark(id);
        updated.title = "Updated Title";

        bool result = m_model->updateBookmark(id, updated);
        QVERIFY(result);
        QCOMPARE(spy.count(), 1);

        Bookmark retrieved = m_model->getBookmark(id);
        QCOMPARE(retrieved.title, QString("Updated Title"));
    }

    void testGetBookmarksForDocument() {
        m_model->addBookmark(Bookmark("/path/to/doc1.pdf", 1));
        m_model->addBookmark(Bookmark("/path/to/doc1.pdf", 5));
        m_model->addBookmark(Bookmark("/path/to/doc2.pdf", 3));

        QList<Bookmark> doc1Bookmarks =
            m_model->getBookmarksForDocument("/path/to/doc1.pdf");
        QList<Bookmark> doc2Bookmarks =
            m_model->getBookmarksForDocument("/path/to/doc2.pdf");

        QCOMPARE(doc1Bookmarks.size(), 2);
        QCOMPARE(doc2Bookmarks.size(), 1);
    }

    void testHasBookmarkForPage() {
        m_model->addBookmark(Bookmark("/path/to/doc.pdf", 5));

        QVERIFY(m_model->hasBookmarkForPage("/path/to/doc.pdf", 5));
        QVERIFY(!m_model->hasBookmarkForPage("/path/to/doc.pdf", 6));
        QVERIFY(!m_model->hasBookmarkForPage("/other/path.pdf", 5));
    }

    void testGetBookmarkForPage() {
        Bookmark bookmark("/path/to/doc.pdf", 5, "Page 5 Bookmark");
        m_model->addBookmark(bookmark);

        Bookmark retrieved = m_model->getBookmarkForPage("/path/to/doc.pdf", 5);
        QCOMPARE(retrieved.title, QString("Page 5 Bookmark"));
        QCOMPARE(retrieved.pageNumber, 5);
    }

    void testCategories() {
        Bookmark bookmark1("/path/to/doc.pdf", 1, "Bookmark 1");
        bookmark1.category = "Work";
        m_model->addBookmark(bookmark1);

        Bookmark bookmark2("/path/to/doc.pdf", 2, "Bookmark 2");
        bookmark2.category = "Personal";
        m_model->addBookmark(bookmark2);

        Bookmark bookmark3("/path/to/doc.pdf", 3, "Bookmark 3");
        bookmark3.category = "Work";
        m_model->addBookmark(bookmark3);

        QStringList categories = m_model->getCategories();
        QVERIFY(categories.contains("Work"));
        QVERIFY(categories.contains("Personal"));

        QList<Bookmark> workBookmarks = m_model->getBookmarksInCategory("Work");
        QCOMPARE(workBookmarks.size(), 2);
    }

    void testMoveBookmarkToCategory() {
        Bookmark bookmark("/path/to/doc.pdf", 1, "Test");
        bookmark.category = "Old";
        m_model->addBookmark(bookmark);

        QString id = m_model->getAllBookmarks().first().id;

        bool result = m_model->moveBookmarkToCategory(id, "New");
        QVERIFY(result);

        Bookmark retrieved = m_model->getBookmark(id);
        QCOMPARE(retrieved.category, QString("New"));
    }

    void testSearchBookmarks() {
        m_model->addBookmark(
            Bookmark("/path/doc.pdf", 1, "Important Meeting Notes"));
        m_model->addBookmark(Bookmark("/path/doc.pdf", 2, "Project Summary"));
        m_model->addBookmark(Bookmark("/path/doc.pdf", 3, "Meeting Agenda"));

        QList<Bookmark> results = m_model->searchBookmarks("Meeting");
        QCOMPARE(results.size(), 2);
    }

    void testGetRecentBookmarks() {
        for (int i = 0; i < 15; ++i) {
            m_model->addBookmark(
                Bookmark("/path/doc.pdf", i, QString("Bookmark %1").arg(i)));
        }

        QList<Bookmark> recent = m_model->getRecentBookmarks(10);
        QCOMPARE(recent.size(), 10);
    }

    void testClearAllBookmarks() {
        m_model->addBookmark(Bookmark("/path/doc1.pdf", 1));
        m_model->addBookmark(Bookmark("/path/doc2.pdf", 2));
        m_model->addBookmark(Bookmark("/path/doc3.pdf", 3));

        QSignalSpy spy(m_model, &BookmarkModel::bookmarksCleared);

        m_model->clearAllBookmarks();

        QCOMPARE(m_model->rowCount(), 0);
        QCOMPARE(spy.count(), 1);
    }

    void testGetDocumentPaths() {
        m_model->addBookmark(Bookmark("/path/doc1.pdf", 1));
        m_model->addBookmark(Bookmark("/path/doc2.pdf", 2));
        m_model->addBookmark(Bookmark("/path/doc1.pdf", 3));

        QStringList paths = m_model->getDocumentPaths();
        QCOMPARE(paths.size(), 2);
        QVERIFY(paths.contains("/path/doc1.pdf"));
        QVERIFY(paths.contains("/path/doc2.pdf"));
    }

    void testGetBookmarkCountForDocument() {
        m_model->addBookmark(Bookmark("/path/doc1.pdf", 1));
        m_model->addBookmark(Bookmark("/path/doc1.pdf", 2));
        m_model->addBookmark(Bookmark("/path/doc2.pdf", 1));

        QCOMPARE(m_model->getBookmarkCountForDocument("/path/doc1.pdf"), 2);
        QCOMPARE(m_model->getBookmarkCountForDocument("/path/doc2.pdf"), 1);
        QCOMPARE(m_model->getBookmarkCountForDocument("/path/doc3.pdf"), 0);
    }

    void testAutoSave() {
        m_model->setAutoSave(true);
        QVERIFY(m_model->isAutoSaveEnabled());

        m_model->setAutoSave(false);
        QVERIFY(!m_model->isAutoSaveEnabled());
    }

    void testBookmarkStruct() {
        Bookmark bookmark("/path/to/doc.pdf", 10, "Test Title");
        QVERIFY(!bookmark.id.isEmpty());
        QCOMPARE(bookmark.documentPath, QString("/path/to/doc.pdf"));
        QCOMPARE(bookmark.pageNumber, 10);
        QCOMPARE(bookmark.title, QString("Test Title"));
        QVERIFY(bookmark.createdTime.isValid());

        QJsonObject json = bookmark.toJson();
        QVERIFY(!json.isEmpty());

        Bookmark loaded = Bookmark::fromJson(json);
        QCOMPARE(loaded.id, bookmark.id);
        QCOMPARE(loaded.documentPath, bookmark.documentPath);
        QCOMPARE(loaded.pageNumber, bookmark.pageNumber);
    }

    void testBookmarkComparison() {
        Bookmark bookmark1("/path/doc.pdf", 1, "Test");
        Bookmark bookmark2 = bookmark1;

        QVERIFY(bookmark1 == bookmark2);

        Bookmark bookmark3("/path/doc.pdf", 2, "Other");
        QVERIFY(bookmark1 != bookmark3);
    }

    void testModelRoles() {
        Bookmark bookmark("/path/to/doc.pdf", 5, "Test Bookmark");
        bookmark.notes = "Some notes";
        bookmark.category = "Work";
        m_model->addBookmark(bookmark);

        QModelIndex index = m_model->index(0, 0);

        QVERIFY(
            m_model->data(index, BookmarkModel::IdRole).toString().length() >
            0);
        QCOMPARE(m_model->data(index, BookmarkModel::TitleRole).toString(),
                 QString("Test Bookmark"));
        QCOMPARE(
            m_model->data(index, BookmarkModel::DocumentPathRole).toString(),
            QString("/path/to/doc.pdf"));
        QCOMPARE(m_model->data(index, BookmarkModel::PageNumberRole).toInt(),
                 5);
        QCOMPARE(m_model->data(index, BookmarkModel::NotesRole).toString(),
                 QString("Some notes"));
        QCOMPARE(m_model->data(index, BookmarkModel::CategoryRole).toString(),
                 QString("Work"));
    }

private:
    BookmarkModel* m_model = nullptr;
};

QTEST_MAIN(TestBookmarkModel)
#include "test_bookmark_model.moc"
