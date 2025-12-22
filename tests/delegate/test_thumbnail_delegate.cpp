#include <QApplication>
#include <QListView>
#include <QPainter>
#include <QPixmap>
#include <QStandardItemModel>
#include <QStyleOptionViewItem>
#include <QTest>
#include "../../app/delegate/ThumbnailDelegate.h"
#include "../TestUtilities.h"

class TestThumbnailDelegate : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void init() {
        m_delegate = new ThumbnailDelegate();
        m_model = new QStandardItemModel();
        m_view = new QListView();
        m_view->setModel(m_model);
        m_view->setItemDelegate(m_delegate);
    }

    void cleanup() {
        delete m_view;
        delete m_model;
        delete m_delegate;
        m_view = nullptr;
        m_model = nullptr;
        m_delegate = nullptr;
    }

    void testConstruction() {
        QVERIFY(m_delegate != nullptr);
        QVERIFY(m_delegate->thumbnailSize().isValid());
        QVERIFY(m_delegate->margins() >= 0);
        QVERIFY(m_delegate->borderRadius() >= 0);
    }

    void testThumbnailSize() {
        QSize originalSize = m_delegate->thumbnailSize();
        QVERIFY(originalSize.isValid());

        QSize newSize(200, 280);
        m_delegate->setThumbnailSize(newSize);
        QCOMPARE(m_delegate->thumbnailSize(), newSize);

        QSize smallSize(50, 70);
        m_delegate->setThumbnailSize(smallSize);
        QCOMPARE(m_delegate->thumbnailSize(), smallSize);
    }

    void testMargins() {
        int originalMargins = m_delegate->margins();
        QVERIFY(originalMargins >= 0);

        m_delegate->setMargins(10);
        QCOMPARE(m_delegate->margins(), 10);

        m_delegate->setMargins(0);
        QCOMPARE(m_delegate->margins(), 0);

        m_delegate->setMargins(20);
        QCOMPARE(m_delegate->margins(), 20);
    }

    void testBorderRadius() {
        int originalRadius = m_delegate->borderRadius();
        QVERIFY(originalRadius >= 0);

        m_delegate->setBorderRadius(8);
        QCOMPARE(m_delegate->borderRadius(), 8);

        m_delegate->setBorderRadius(0);
        QCOMPARE(m_delegate->borderRadius(), 0);

        m_delegate->setBorderRadius(16);
        QCOMPARE(m_delegate->borderRadius(), 16);
    }

    void testShadowEnabled() {
        m_delegate->setShadowEnabled(true);
        QVERIFY(m_delegate->shadowEnabled());

        m_delegate->setShadowEnabled(false);
        QVERIFY(!m_delegate->shadowEnabled());
    }

    void testAnimationEnabled() {
        m_delegate->setAnimationEnabled(true);
        QVERIFY(m_delegate->animationEnabled());

        m_delegate->setAnimationEnabled(false);
        QVERIFY(!m_delegate->animationEnabled());
    }

    void testThemes() {
        m_delegate->setLightTheme();
        m_delegate->setDarkTheme();

        QColor bg(30, 30, 30);
        QColor border(50, 50, 50);
        QColor text(200, 200, 200);
        QColor accent(0, 120, 215);
        m_delegate->setCustomColors(bg, border, text, accent);
    }

    void testRenderCacheEnabled() {
        m_delegate->setRenderCacheEnabled(true);
        QVERIFY(m_delegate->isRenderCacheEnabled());

        m_delegate->setRenderCacheEnabled(false);
        QVERIFY(!m_delegate->isRenderCacheEnabled());
    }

    void testHighQualityRenderingEnabled() {
        m_delegate->setHighQualityRenderingEnabled(true);
        QVERIFY(m_delegate->isHighQualityRenderingEnabled());

        m_delegate->setHighQualityRenderingEnabled(false);
        QVERIFY(!m_delegate->isHighQualityRenderingEnabled());
    }

    void testAntiAliasingEnabled() {
        m_delegate->setAntiAliasingEnabled(true);
        QVERIFY(m_delegate->isAntiAliasingEnabled());

        m_delegate->setAntiAliasingEnabled(false);
        QVERIFY(!m_delegate->isAntiAliasingEnabled());
    }

    void testCacheManagement() {
        m_delegate->setMaxCacheSize(100);
        QCOMPARE(m_delegate->maxCacheSize(), 100);

        m_delegate->setMaxCacheSize(50);
        QCOMPARE(m_delegate->maxCacheSize(), 50);

        m_delegate->clearRenderCache();
    }

    void testPerformanceStats() {
        m_delegate->resetPerformanceStats();

        QVERIFY(m_delegate->averagePaintTime() >= 0.0);
        QVERIFY(m_delegate->cacheHitRate() >= 0.0);
        QVERIFY(m_delegate->cacheHitRate() <= 1.0);
        QVERIFY(m_delegate->totalPaintCalls() >= 0);
    }

    void testSizeHint() {
        QStandardItem* item = new QStandardItem();
        item->setData(1, Qt::UserRole);
        QPixmap thumbnail(150, 200);
        thumbnail.fill(Qt::gray);
        item->setData(thumbnail, Qt::DecorationRole);
        m_model->appendRow(item);

        QStyleOptionViewItem option;
        option.rect = QRect(0, 0, 200, 300);
        option.font = QApplication::font();

        QModelIndex index = m_model->index(0, 0);
        QSize size = m_delegate->sizeHint(option, index);

        QVERIFY(size.width() > 0);
        QVERIFY(size.height() > 0);
    }

    void testPaint() {
        QStandardItem* item = new QStandardItem();
        item->setData(1, Qt::UserRole);
        QPixmap thumbnail(150, 200);
        thumbnail.fill(Qt::gray);
        item->setData(thumbnail, Qt::DecorationRole);
        m_model->appendRow(item);

        QPixmap pixmap(200, 300);
        pixmap.fill(Qt::white);
        QPainter painter(&pixmap);

        QStyleOptionViewItem option;
        option.rect = QRect(0, 0, 200, 300);
        option.font = QApplication::font();
        option.state = QStyle::State_Enabled;

        QModelIndex index = m_model->index(0, 0);
        m_delegate->paint(&painter, option, index);
        painter.end();

        QVERIFY(!pixmap.isNull());
    }

    void testPaintWithSelection() {
        QStandardItem* item = new QStandardItem();
        item->setData(1, Qt::UserRole);
        QPixmap thumbnail(150, 200);
        thumbnail.fill(Qt::gray);
        item->setData(thumbnail, Qt::DecorationRole);
        m_model->appendRow(item);

        QPixmap pixmap(200, 300);
        pixmap.fill(Qt::white);
        QPainter painter(&pixmap);

        QStyleOptionViewItem option;
        option.rect = QRect(0, 0, 200, 300);
        option.font = QApplication::font();
        option.state = QStyle::State_Enabled | QStyle::State_Selected;

        QModelIndex index = m_model->index(0, 0);
        m_delegate->paint(&painter, option, index);
        painter.end();

        QVERIFY(!pixmap.isNull());
    }

    void testPaintWithHover() {
        QStandardItem* item = new QStandardItem();
        item->setData(1, Qt::UserRole);
        QPixmap thumbnail(150, 200);
        thumbnail.fill(Qt::gray);
        item->setData(thumbnail, Qt::DecorationRole);
        m_model->appendRow(item);

        QPixmap pixmap(200, 300);
        pixmap.fill(Qt::white);
        QPainter painter(&pixmap);

        QStyleOptionViewItem option;
        option.rect = QRect(0, 0, 200, 300);
        option.font = QApplication::font();
        option.state = QStyle::State_Enabled | QStyle::State_MouseOver;

        QModelIndex index = m_model->index(0, 0);
        m_delegate->paint(&painter, option, index);
        painter.end();

        QVERIFY(!pixmap.isNull());
    }

    void testMultipleItemsPaint() {
        for (int i = 0; i < 10; ++i) {
            QStandardItem* item = new QStandardItem();
            item->setData(i + 1, Qt::UserRole);
            QPixmap thumbnail(150, 200);
            thumbnail.fill(QColor(i * 20, i * 20, i * 20));
            item->setData(thumbnail, Qt::DecorationRole);
            m_model->appendRow(item);
        }

        QStyleOptionViewItem option;
        option.rect = QRect(0, 0, 200, 300);
        option.font = QApplication::font();
        option.state = QStyle::State_Enabled;

        for (int i = 0; i < m_model->rowCount(); ++i) {
            QPixmap pixmap(200, 300);
            pixmap.fill(Qt::white);
            QPainter painter(&pixmap);

            QModelIndex index = m_model->index(i, 0);
            m_delegate->paint(&painter, option, index);
            painter.end();

            QVERIFY(!pixmap.isNull());
        }
    }

private:
    ThumbnailDelegate* m_delegate = nullptr;
    QStandardItemModel* m_model = nullptr;
    QListView* m_view = nullptr;
};

QTEST_MAIN(TestThumbnailDelegate)
#include "test_thumbnail_delegate.moc"
