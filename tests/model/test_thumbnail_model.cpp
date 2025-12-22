#include <QPixmap>
#include <QSignalSpy>
#include <QTest>
#include "../../app/model/ThumbnailModel.h"
#include "../TestUtilities.h"

class TestThumbnailModel : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void init() { m_model = new ThumbnailModel(); }

    void cleanup() {
        delete m_model;
        m_model = nullptr;
    }

    void testConstruction() {
        QVERIFY(m_model != nullptr);
        QCOMPARE(m_model->rowCount(), 0);
    }

    void testSetPageCount() {
        m_model->setPageCount(10);
        QCOMPARE(m_model->rowCount(), 10);

        m_model->setPageCount(5);
        QCOMPARE(m_model->rowCount(), 5);

        m_model->setPageCount(0);
        QCOMPARE(m_model->rowCount(), 0);
    }

    void testSetThumbnail() {
        m_model->setPageCount(5);

        QPixmap thumbnail(100, 150);
        thumbnail.fill(Qt::gray);

        m_model->setThumbnail(0, thumbnail);
        m_model->setThumbnail(2, thumbnail);
        m_model->setThumbnail(4, thumbnail);
    }

    void testGetThumbnail() {
        m_model->setPageCount(3);

        QPixmap thumbnail(100, 150);
        thumbnail.fill(Qt::blue);

        m_model->setThumbnail(1, thumbnail);

        QPixmap retrieved = m_model->getThumbnail(1);
        QVERIFY(!retrieved.isNull());
    }

    void testClearThumbnails() {
        m_model->setPageCount(5);

        QPixmap thumbnail(100, 150);
        thumbnail.fill(Qt::red);

        for (int i = 0; i < 5; ++i) {
            m_model->setThumbnail(i, thumbnail);
        }

        m_model->clearThumbnails();
    }

    void testThumbnailSize() {
        QSize size = m_model->thumbnailSize();
        QVERIFY(size.isValid());

        m_model->setThumbnailSize(QSize(150, 200));
        QCOMPARE(m_model->thumbnailSize(), QSize(150, 200));
    }

    void testCurrentPage() {
        m_model->setPageCount(10);

        m_model->setCurrentPage(5);
        QCOMPARE(m_model->currentPage(), 5);

        m_model->setCurrentPage(1);
        QCOMPARE(m_model->currentPage(), 1);
    }

    void testModelRoles() {
        m_model->setPageCount(3);

        QModelIndex index = m_model->index(0, 0);
        QVERIFY(index.isValid());

        QVariant pageNumber =
            m_model->data(index, ThumbnailModel::PageNumberRole);
        QCOMPARE(pageNumber.toInt(), 1);
    }

    void testFlags() {
        m_model->setPageCount(3);
        QModelIndex index = m_model->index(0, 0);

        Qt::ItemFlags flags = m_model->flags(index);
        QVERIFY(flags & Qt::ItemIsEnabled);
        QVERIFY(flags & Qt::ItemIsSelectable);
    }

    void testInvalidIndex() {
        m_model->setPageCount(5);

        QModelIndex invalidIndex = m_model->index(10, 0);
        QVariant data = m_model->data(invalidIndex, Qt::DisplayRole);
        QVERIFY(!data.isValid());
    }

    void testThumbnailUpdatedSignal() {
        m_model->setPageCount(5);

        QSignalSpy spy(m_model, &ThumbnailModel::thumbnailUpdated);

        QPixmap thumbnail(100, 150);
        thumbnail.fill(Qt::green);

        m_model->setThumbnail(2, thumbnail);

        QCOMPARE(spy.count(), 1);
        QList<QVariant> args = spy.first();
        QCOMPARE(args.at(0).toInt(), 2);
    }

    void testPageCountChangedSignal() {
        QSignalSpy spy(m_model, &ThumbnailModel::pageCountChanged);

        m_model->setPageCount(10);

        QCOMPARE(spy.count(), 1);
        QList<QVariant> args = spy.first();
        QCOMPARE(args.at(0).toInt(), 10);
    }

    void testMultipleThumbnailUpdates() {
        m_model->setPageCount(10);

        QPixmap thumbnail(100, 150);
        thumbnail.fill(Qt::cyan);

        for (int i = 0; i < 10; ++i) {
            m_model->setThumbnail(i, thumbnail);
        }

        for (int i = 0; i < 10; ++i) {
            QPixmap retrieved = m_model->getThumbnail(i);
            QVERIFY(!retrieved.isNull());
        }
    }

    void testSetThumbnailOutOfRange() {
        m_model->setPageCount(5);

        QPixmap thumbnail(100, 150);
        thumbnail.fill(Qt::magenta);

        m_model->setThumbnail(-1, thumbnail);
        m_model->setThumbnail(100, thumbnail);
    }

private:
    ThumbnailModel* m_model = nullptr;
};

QTEST_MAIN(TestThumbnailModel)
#include "test_thumbnail_model.moc"
