#include <QIcon>
#include <QTest>
#include "../../app/managers/FileTypeIconManager.h"
#include "../TestUtilities.h"

class TestFileTypeIconManager : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void testSingleton() {
        FileTypeIconManager& instance1 = FileTypeIconManager::instance();
        FileTypeIconManager& instance2 = FileTypeIconManager::instance();
        QVERIFY(&instance1 == &instance2);
    }

    void testGetIconForPDF() {
        FileTypeIconManager& manager = FileTypeIconManager::instance();
        QIcon icon = manager.getIconForExtension("pdf");
        QVERIFY(!icon.isNull());
    }

    void testGetIconForDocument() {
        FileTypeIconManager& manager = FileTypeIconManager::instance();

        QIcon pdfIcon = manager.getIconForExtension("pdf");
        QVERIFY(!pdfIcon.isNull());
    }

    void testGetIconForUnknownType() {
        FileTypeIconManager& manager = FileTypeIconManager::instance();
        QIcon icon = manager.getIconForExtension("xyz123unknown");
    }

    void testGetIconForFile() {
        FileTypeIconManager& manager = FileTypeIconManager::instance();

        QIcon icon = manager.getIconForFile("/path/to/document.pdf");
        QVERIFY(!icon.isNull());
    }

    void testCaseInsensitivity() {
        FileTypeIconManager& manager = FileTypeIconManager::instance();

        QIcon lowerIcon = manager.getIconForExtension("pdf");
        QIcon upperIcon = manager.getIconForExtension("PDF");
        QIcon mixedIcon = manager.getIconForExtension("PdF");

        QVERIFY(!lowerIcon.isNull());
        QVERIFY(!upperIcon.isNull());
        QVERIFY(!mixedIcon.isNull());
    }

    void testGetIconWithPath() {
        FileTypeIconManager& manager = FileTypeIconManager::instance();

        QIcon icon1 = manager.getIconForFile("/path/to/file.pdf");
        QIcon icon2 =
            manager.getIconForFile("C:\\Users\\Test\\Documents\\file.pdf");
        QIcon icon3 = manager.getIconForFile("relative/path/file.pdf");

        QVERIFY(!icon1.isNull());
        QVERIFY(!icon2.isNull());
        QVERIFY(!icon3.isNull());
    }

    void testGetIconForEmptyExtension() {
        FileTypeIconManager& manager = FileTypeIconManager::instance();
        QIcon icon = manager.getIconForExtension("");
    }

    void testGetIconForEmptyPath() {
        FileTypeIconManager& manager = FileTypeIconManager::instance();
        QIcon icon = manager.getIconForFile("");
    }

    void testGetIconForPathWithoutExtension() {
        FileTypeIconManager& manager = FileTypeIconManager::instance();
        QIcon icon = manager.getIconForFile("/path/to/noextension");
    }

    void testGetIconForPathWithDots() {
        FileTypeIconManager& manager = FileTypeIconManager::instance();
        QIcon icon = manager.getIconForFile("/path/to/file.backup.pdf");
        QVERIFY(!icon.isNull());
    }

    void testCacheEfficiency() {
        FileTypeIconManager& manager = FileTypeIconManager::instance();

        for (int i = 0; i < 100; ++i) {
            QIcon icon = manager.getIconForExtension("pdf");
            QVERIFY(!icon.isNull());
        }
    }

    void testGetSupportedExtensions() {
        FileTypeIconManager& manager = FileTypeIconManager::instance();
        QStringList extensions = manager.getSupportedExtensions();
        QVERIFY(extensions.contains("pdf", Qt::CaseInsensitive));
    }

    void testIsExtensionSupported() {
        FileTypeIconManager& manager = FileTypeIconManager::instance();
        QVERIFY(manager.isExtensionSupported("pdf"));
        QVERIFY(manager.isExtensionSupported("PDF"));
    }

    void testGetDefaultIcon() {
        FileTypeIconManager& manager = FileTypeIconManager::instance();
        QIcon defaultIcon = manager.getDefaultIcon();
        QVERIFY(!defaultIcon.isNull());
    }

    void testGetIconSize() {
        FileTypeIconManager& manager = FileTypeIconManager::instance();

        QIcon icon16 = manager.getIconForExtension("pdf", QSize(16, 16));
        QIcon icon32 = manager.getIconForExtension("pdf", QSize(32, 32));
        QIcon icon64 = manager.getIconForExtension("pdf", QSize(64, 64));

        QVERIFY(!icon16.isNull());
        QVERIFY(!icon32.isNull());
        QVERIFY(!icon64.isNull());
    }

    void testClearCache() {
        FileTypeIconManager& manager = FileTypeIconManager::instance();

        manager.getIconForExtension("pdf");
        manager.clearCache();
        manager.getIconForExtension("pdf");
    }

private:
};

QTEST_MAIN(TestFileTypeIconManager)
#include "test_file_type_icon_manager.moc"
