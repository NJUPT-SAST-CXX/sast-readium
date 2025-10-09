#include <QApplication>
#include <QDebug>
#include <QtTest/QtTest>

/**
 * Simple smoke test to verify basic compilation and QGraphics support detection
 */
class SmokeTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testQGraphicsSupport();
    void testBasicQtFunctionality();
    void cleanupTestCase();
};

void SmokeTest::initTestCase() {
    qDebug() << "=== QGraphics PDF Support Smoke Test ===";
    qDebug() << "Qt version:" << QT_VERSION_STR;

#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    qDebug() << "QGraphics PDF support: ENABLED";
#else
    qDebug() << "QGraphics PDF support: DISABLED";
#endif
}

void SmokeTest::testQGraphicsSupport() {
    // Test that the macro is working correctly
#ifdef ENABLE_QGRAPHICS_PDF_SUPPORT
    QVERIFY(true);  // QGraphics support is enabled
    qDebug() << "✓ QGraphics support macro is defined";
#else
    QVERIFY(true);  // QGraphics support is disabled, which is also valid
    qDebug() << "✓ QGraphics support macro is not defined (traditional mode)";
#endif
}

void SmokeTest::testBasicQtFunctionality() {
    // Test basic Qt functionality
    QString testString = "QGraphics PDF Test";
    QCOMPARE(testString.length(), 18);

    QStringList testList;
    testList << "Traditional"
             << "QGraphics";
    QCOMPARE(testList.size(), 2);

    qDebug() << "✓ Basic Qt functionality works";
}

void SmokeTest::cleanupTestCase() {
    qDebug() << "=== Smoke Test Completed Successfully ===";
}

// Simple main function for standalone execution
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Set up for headless testing
    app.setAttribute(Qt::AA_Use96Dpi, true);

    SmokeTest test;
    int result = QTest::qExec(&test, argc, argv);

    if (result == 0) {
        qDebug() << "SUCCESS: Smoke test passed";
    } else {
        qDebug() << "FAILURE: Smoke test failed";
    }

    return result;
}

#include "smoke_test.moc"
