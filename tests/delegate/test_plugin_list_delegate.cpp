#include <QApplication>
#include <QListView>
#include <QPainter>
#include <QPixmap>
#include <QStandardItemModel>
#include <QStyleOptionViewItem>
#include <QTest>
#include "../../app/delegate/PluginListDelegate.h"
#include "../TestUtilities.h"

class TestPluginListDelegate : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void init() {
        m_delegate = new PluginListDelegate();
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
        QCOMPARE(m_delegate->displayMode(), PluginListDelegate::NormalMode);
        QVERIFY(m_delegate->showIcons());
        QVERIFY(m_delegate->showStatus());
        QVERIFY(m_delegate->highlightErrors());
    }

    void testDisplayModes() {
        m_delegate->setDisplayMode(PluginListDelegate::CompactMode);
        QCOMPARE(m_delegate->displayMode(), PluginListDelegate::CompactMode);

        m_delegate->setDisplayMode(PluginListDelegate::NormalMode);
        QCOMPARE(m_delegate->displayMode(), PluginListDelegate::NormalMode);

        m_delegate->setDisplayMode(PluginListDelegate::DetailedMode);
        QCOMPARE(m_delegate->displayMode(), PluginListDelegate::DetailedMode);
    }

    void testShowIcons() {
        QVERIFY(m_delegate->showIcons());

        m_delegate->setShowIcons(false);
        QVERIFY(!m_delegate->showIcons());

        m_delegate->setShowIcons(true);
        QVERIFY(m_delegate->showIcons());
    }

    void testShowStatus() {
        QVERIFY(m_delegate->showStatus());

        m_delegate->setShowStatus(false);
        QVERIFY(!m_delegate->showStatus());

        m_delegate->setShowStatus(true);
        QVERIFY(m_delegate->showStatus());
    }

    void testHighlightErrors() {
        QVERIFY(m_delegate->highlightErrors());

        m_delegate->setHighlightErrors(false);
        QVERIFY(!m_delegate->highlightErrors());

        m_delegate->setHighlightErrors(true);
        QVERIFY(m_delegate->highlightErrors());
    }

    void testColors() {
        QColor loadedColor(0, 200, 0);
        m_delegate->setLoadedColor(loadedColor);
        QCOMPARE(m_delegate->loadedColor(), loadedColor);

        QColor disabledColor(128, 128, 128);
        m_delegate->setDisabledColor(disabledColor);
        QCOMPARE(m_delegate->disabledColor(), disabledColor);

        QColor errorColor(255, 0, 0);
        m_delegate->setErrorColor(errorColor);
        QCOMPARE(m_delegate->errorColor(), errorColor);
    }

    void testSizeHint() {
        QStandardItem* item = new QStandardItem("Test Plugin");
        item->setData("Test Plugin", Qt::DisplayRole);
        item->setData("1.0.0", Qt::UserRole + 1);
        item->setData("Test Author", Qt::UserRole + 2);
        item->setData(true, Qt::UserRole + 3);
        item->setData(true, Qt::UserRole + 4);
        item->setData(false, Qt::UserRole + 5);
        m_model->appendRow(item);

        QStyleOptionViewItem option;
        option.rect = QRect(0, 0, 300, 100);
        option.font = QApplication::font();

        QModelIndex index = m_model->index(0, 0);
        QSize size = m_delegate->sizeHint(option, index);

        QVERIFY(size.width() > 0);
        QVERIFY(size.height() > 0);
    }

    void testSizeHintDifferentModes() {
        QStandardItem* item = new QStandardItem("Test Plugin");
        m_model->appendRow(item);

        QStyleOptionViewItem option;
        option.rect = QRect(0, 0, 300, 100);
        option.font = QApplication::font();
        QModelIndex index = m_model->index(0, 0);

        m_delegate->setDisplayMode(PluginListDelegate::CompactMode);
        QSize compactSize = m_delegate->sizeHint(option, index);

        m_delegate->setDisplayMode(PluginListDelegate::NormalMode);
        QSize normalSize = m_delegate->sizeHint(option, index);

        m_delegate->setDisplayMode(PluginListDelegate::DetailedMode);
        QSize detailedSize = m_delegate->sizeHint(option, index);

        QVERIFY(compactSize.height() <= normalSize.height());
        QVERIFY(normalSize.height() <= detailedSize.height());
    }

    void testPaint() {
        QStandardItem* item = new QStandardItem("Test Plugin");
        item->setData("Test Plugin", Qt::DisplayRole);
        item->setData("1.0.0", Qt::UserRole + 1);
        item->setData("Test Author", Qt::UserRole + 2);
        item->setData(true, Qt::UserRole + 3);
        item->setData(true, Qt::UserRole + 4);
        item->setData(false, Qt::UserRole + 5);
        m_model->appendRow(item);

        QPixmap pixmap(300, 100);
        pixmap.fill(Qt::white);
        QPainter painter(&pixmap);

        QStyleOptionViewItem option;
        option.rect = QRect(0, 0, 300, 100);
        option.font = QApplication::font();
        option.state = QStyle::State_Enabled;

        QModelIndex index = m_model->index(0, 0);
        m_delegate->paint(&painter, option, index);
        painter.end();

        QVERIFY(!pixmap.isNull());
    }

    void testPaintWithSelection() {
        QStandardItem* item = new QStandardItem("Test Plugin");
        m_model->appendRow(item);

        QPixmap pixmap(300, 100);
        pixmap.fill(Qt::white);
        QPainter painter(&pixmap);

        QStyleOptionViewItem option;
        option.rect = QRect(0, 0, 300, 100);
        option.font = QApplication::font();
        option.state = QStyle::State_Enabled | QStyle::State_Selected;

        QModelIndex index = m_model->index(0, 0);
        m_delegate->paint(&painter, option, index);
        painter.end();

        QVERIFY(!pixmap.isNull());
    }

    void testPaintWithHover() {
        QStandardItem* item = new QStandardItem("Test Plugin");
        m_model->appendRow(item);

        QPixmap pixmap(300, 100);
        pixmap.fill(Qt::white);
        QPainter painter(&pixmap);

        QStyleOptionViewItem option;
        option.rect = QRect(0, 0, 300, 100);
        option.font = QApplication::font();
        option.state = QStyle::State_Enabled | QStyle::State_MouseOver;

        QModelIndex index = m_model->index(0, 0);
        m_delegate->paint(&painter, option, index);
        painter.end();

        QVERIFY(!pixmap.isNull());
    }

    void testPaintAllModes() {
        QStandardItem* item = new QStandardItem("Test Plugin");
        m_model->appendRow(item);

        QStyleOptionViewItem option;
        option.rect = QRect(0, 0, 300, 100);
        option.font = QApplication::font();
        option.state = QStyle::State_Enabled;
        QModelIndex index = m_model->index(0, 0);

        QList<PluginListDelegate::DisplayMode> modes = {
            PluginListDelegate::CompactMode, PluginListDelegate::NormalMode,
            PluginListDelegate::DetailedMode};

        for (auto mode : modes) {
            m_delegate->setDisplayMode(mode);

            QPixmap pixmap(300, 100);
            pixmap.fill(Qt::white);
            QPainter painter(&pixmap);

            m_delegate->paint(&painter, option, index);
            painter.end();

            QVERIFY(!pixmap.isNull());
        }
    }

private:
    PluginListDelegate* m_delegate = nullptr;
    QStandardItemModel* m_model = nullptr;
    QListView* m_view = nullptr;
};

QTEST_MAIN(TestPluginListDelegate)
#include "test_plugin_list_delegate.moc"
