#include <QLabel>
#include <QTest>
#include "../../app/delegate/PageNavigationDelegate.h"
#include "../TestUtilities.h"

class TestPageNavigationDelegate : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void init() {
        m_pageLabel = new QLabel();
        m_delegate = new PageNavigationDelegate(m_pageLabel);
    }

    void cleanup() {
        delete m_delegate;
        m_delegate = nullptr;
        delete m_pageLabel;
        m_pageLabel = nullptr;
    }

    void testConstruction() {
        QVERIFY(m_delegate != nullptr);
        QVERIFY(m_pageLabel != nullptr);
    }

    void testConstructionWithNullLabel() {
        PageNavigationDelegate* delegate = new PageNavigationDelegate(nullptr);
        QVERIFY(delegate != nullptr);
        delete delegate;
    }

    void testViewUpdate() {
        m_delegate->viewUpdate(1);
        QVERIFY(!m_pageLabel->text().isEmpty());
    }

    void testViewUpdateMultiplePages() {
        m_delegate->viewUpdate(5);
        QString text1 = m_pageLabel->text();
        QVERIFY(!text1.isEmpty());

        m_delegate->viewUpdate(10);
        QString text2 = m_pageLabel->text();
        QVERIFY(!text2.isEmpty());
    }

    void testViewUpdateZeroPage() {
        m_delegate->viewUpdate(0);
        QVERIFY(!m_pageLabel->text().isEmpty());
    }

    void testViewUpdateNegativePage() {
        m_delegate->viewUpdate(-1);
        QVERIFY(!m_pageLabel->text().isEmpty());
    }

    void testViewUpdateLargePageNumber() {
        m_delegate->viewUpdate(999999);
        QVERIFY(!m_pageLabel->text().isEmpty());
    }

    void testRepeatedUpdates() {
        for (int i = 1; i <= 100; ++i) {
            m_delegate->viewUpdate(i);
        }
        QVERIFY(!m_pageLabel->text().isEmpty());
    }

private:
    QLabel* m_pageLabel = nullptr;
    PageNavigationDelegate* m_delegate = nullptr;
};

QTEST_MAIN(TestPageNavigationDelegate)
#include "test_page_navigation_delegate.moc"
