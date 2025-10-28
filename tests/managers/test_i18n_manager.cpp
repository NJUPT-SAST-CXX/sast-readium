#include <QSignalSpy>
#include <QtTest/QtTest>
#include "../../app/managers/I18nManager.h"
#include "../TestUtilities.h"

/**
 * @brief Comprehensive tests for I18nManager
 *
 * Tests internationalization functionality including language loading,
 * switching, singleton pattern, and signal emissions.
 */
class I18nManagerTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Singleton tests
    void testSingletonInstance();
    void testSingletonConsistency();

    // Initialization tests
    void testInitialize();
    void testInitializeMultipleTimes();

    // Language loading tests
    void testLoadEnglish();
    void testLoadChinese();
    void testLoadSystemLanguage();
    void testLoadLanguageByCode();
    void testLoadInvalidLanguageCode();

    // Language query tests
    void testCurrentLanguage();
    void testCurrentLanguageCode();
    void testCurrentLanguageName();
    void testAvailableLanguages();

    // Language utility tests
    void testLanguageToCode();
    void testCodeToLanguage();
    void testLanguageToName();

    // Signal emission tests
    void testLanguageChangedSignalEnum();
    void testLanguageChangedSignalString();
    void testNoSignalOnSameLanguage();

    // Edge cases
    void testRapidLanguageSwitching();
    void testLanguagePersistence();

private:
    I18nManager* m_manager;
};

void I18nManagerTest::initTestCase() {
    // Singleton instance will be created on first access
}

void I18nManagerTest::cleanupTestCase() {
    // Singleton cleanup handled automatically
}

void I18nManagerTest::init() { m_manager = &I18nManager::instance(); }

void I18nManagerTest::cleanup() {
    // Don't delete singleton
    m_manager = nullptr;
}

void I18nManagerTest::testSingletonInstance() {
    I18nManager& instance1 = I18nManager::instance();
    I18nManager& instance2 = I18nManager::instance();

    QCOMPARE(&instance1, &instance2);
}

void I18nManagerTest::testSingletonConsistency() {
    I18nManager& instance = I18nManager::instance();

    // Set a language
    instance.loadLanguage(I18nManager::Language::English);

    // Get instance again and verify state persists
    I18nManager& instance2 = I18nManager::instance();
    QCOMPARE(instance2.currentLanguage(), I18nManager::Language::English);
}

void I18nManagerTest::testInitialize() {
    bool result = m_manager->initialize();
    QVERIFY(result);
}

void I18nManagerTest::testInitializeMultipleTimes() {
    bool result1 = m_manager->initialize();
    bool result2 = m_manager->initialize();

    QVERIFY(result1);
    QVERIFY(result2);  // Should handle multiple initializations gracefully
}

void I18nManagerTest::testLoadEnglish() {
    QSignalSpy spy(m_manager, QOverload<I18nManager::Language>::of(
                                  &I18nManager::languageChanged));

    bool result = m_manager->loadLanguage(I18nManager::Language::English);

    QVERIFY(result);
    QCOMPARE(m_manager->currentLanguage(), I18nManager::Language::English);
    QVERIFY(spy.count() > 0);
}

void I18nManagerTest::testLoadChinese() {
    QSignalSpy spy(m_manager, QOverload<I18nManager::Language>::of(
                                  &I18nManager::languageChanged));

    bool result = m_manager->loadLanguage(I18nManager::Language::Chinese);

    QVERIFY(result);
    QCOMPARE(m_manager->currentLanguage(), I18nManager::Language::Chinese);
    QVERIFY(spy.count() > 0);
}

void I18nManagerTest::testLoadSystemLanguage() {
    bool result = m_manager->loadLanguage(I18nManager::Language::System);

    QVERIFY(result);
    // System language should resolve to either English or Chinese
    auto lang = m_manager->currentLanguage();
    QVERIFY(lang == I18nManager::Language::English ||
            lang == I18nManager::Language::Chinese);
}

void I18nManagerTest::testLoadLanguageByCode() {
    bool result = m_manager->loadLanguage("en");
    QVERIFY(result);
    QCOMPARE(m_manager->currentLanguageCode(), QString("en"));

    result = m_manager->loadLanguage("zh");
    QVERIFY(result);
    QCOMPARE(m_manager->currentLanguageCode(), QString("zh"));
}

void I18nManagerTest::testLoadInvalidLanguageCode() {
    // Save current language
    auto currentLang = m_manager->currentLanguage();

    bool result = m_manager->loadLanguage("invalid_code");

    // Should fail or fallback gracefully
    QVERIFY(!result || m_manager->currentLanguage() == currentLang);
}

void I18nManagerTest::testCurrentLanguage() {
    m_manager->loadLanguage(I18nManager::Language::English);
    QCOMPARE(m_manager->currentLanguage(), I18nManager::Language::English);

    m_manager->loadLanguage(I18nManager::Language::Chinese);
    QCOMPARE(m_manager->currentLanguage(), I18nManager::Language::Chinese);
}

void I18nManagerTest::testCurrentLanguageCode() {
    m_manager->loadLanguage(I18nManager::Language::English);
    QCOMPARE(m_manager->currentLanguageCode(), QString("en"));

    m_manager->loadLanguage(I18nManager::Language::Chinese);
    QCOMPARE(m_manager->currentLanguageCode(), QString("zh"));
}

void I18nManagerTest::testCurrentLanguageName() {
    m_manager->loadLanguage(I18nManager::Language::English);
    QString name = m_manager->currentLanguageName();
    QVERIFY(!name.isEmpty());
    QVERIFY(name.contains("English", Qt::CaseInsensitive));

    m_manager->loadLanguage(I18nManager::Language::Chinese);
    name = m_manager->currentLanguageName();
    QVERIFY(!name.isEmpty());
}

void I18nManagerTest::testAvailableLanguages() {
    QStringList languages = m_manager->availableLanguages();

    QVERIFY(!languages.isEmpty());
    QVERIFY(languages.contains("en"));
    QVERIFY(languages.contains("zh"));
}

void I18nManagerTest::testLanguageToCode() {
    QCOMPARE(I18nManager::languageToCode(I18nManager::Language::English),
             QString("en"));
    QCOMPARE(I18nManager::languageToCode(I18nManager::Language::Chinese),
             QString("zh"));
}

void I18nManagerTest::testCodeToLanguage() {
    QCOMPARE(I18nManager::codeToLanguage("en"), I18nManager::Language::English);
    QCOMPARE(I18nManager::codeToLanguage("zh"), I18nManager::Language::Chinese);
}

void I18nManagerTest::testLanguageToName() {
    QString name = I18nManager::languageToName(I18nManager::Language::English);
    QVERIFY(!name.isEmpty());

    name = I18nManager::languageToName(I18nManager::Language::Chinese);
    QVERIFY(!name.isEmpty());
}

void I18nManagerTest::testLanguageChangedSignalEnum() {
    QSignalSpy spy(m_manager, QOverload<I18nManager::Language>::of(
                                  &I18nManager::languageChanged));

    m_manager->loadLanguage(I18nManager::Language::English);
    QCOMPARE(spy.count(), 1);

    m_manager->loadLanguage(I18nManager::Language::Chinese);
    QCOMPARE(spy.count(), 2);
}

void I18nManagerTest::testLanguageChangedSignalString() {
    QSignalSpy spy(m_manager, QOverload<const QString&>::of(
                                  &I18nManager::languageChanged));

    m_manager->loadLanguage("en");
    QCOMPARE(spy.count(), 1);

    m_manager->loadLanguage("zh");
    QCOMPARE(spy.count(), 2);
}

void I18nManagerTest::testNoSignalOnSameLanguage() {
    m_manager->loadLanguage(I18nManager::Language::English);

    QSignalSpy spy(m_manager, QOverload<I18nManager::Language>::of(
                                  &I18nManager::languageChanged));

    // Load same language again
    m_manager->loadLanguage(I18nManager::Language::English);

    // Should not emit signal if language didn't change
    QCOMPARE(spy.count(), 0);
}

void I18nManagerTest::testRapidLanguageSwitching() {
    // Test rapid switching doesn't cause issues
    for (int i = 0; i < 10; ++i) {
        m_manager->loadLanguage(I18nManager::Language::English);
        m_manager->loadLanguage(I18nManager::Language::Chinese);
    }

    // Should still be in valid state
    auto lang = m_manager->currentLanguage();
    QVERIFY(lang == I18nManager::Language::English ||
            lang == I18nManager::Language::Chinese);
}

void I18nManagerTest::testLanguagePersistence() {
    m_manager->loadLanguage(I18nManager::Language::Chinese);

    // Get instance again
    I18nManager& instance = I18nManager::instance();

    // Language should persist
    QCOMPARE(instance.currentLanguage(), I18nManager::Language::Chinese);
}

QTEST_MAIN(I18nManagerTest)
#include "test_i18n_manager.moc"
