#include <QtTest/QtTest>
#include <QObject>
#include <QTemporaryDir>
#include "../../app/logging/LoggingConfig.h"
#include "../TestUtilities.h"

/**
 * Minimal test for LoggingStubs to isolate crash issues
 */
class LoggingStubsTestMinimal : public TestBase
{
    Q_OBJECT

protected:
    void initTestCase() override;
    void cleanupTestCase() override;

private slots:
    void testBasicInstantiation();
    void testTemporaryDir();

private:
    QTemporaryDir* m_tempDir;
};

void LoggingStubsTestMinimal::initTestCase()
{
    qDebug() << "Starting minimal LoggingStubs tests";
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    qDebug() << "Temp dir created:" << m_tempDir->path();
}

void LoggingStubsTestMinimal::cleanupTestCase()
{
    delete m_tempDir;
    qDebug() << "Minimal LoggingStubs tests completed";
}

void LoggingStubsTestMinimal::testBasicInstantiation()
{
    // Test that we can create LoggingConfig without crashing
    LoggingConfig* config = new LoggingConfig();
    QVERIFY(config != nullptr);
    delete config;
}

void LoggingStubsTestMinimal::testTemporaryDir()
{
    // Test that QTemporaryDir works correctly
    QVERIFY(m_tempDir != nullptr);
    QVERIFY(m_tempDir->isValid());
    
    // Test filePath method that was crashing
    QString testPath = m_tempDir->filePath("test.log");
    QVERIFY(!testPath.isEmpty());
    qDebug() << "Test file path:" << testPath;
}

QTEST_MAIN(LoggingStubsTestMinimal)
#include "LoggingStubsTestMinimal.moc"
