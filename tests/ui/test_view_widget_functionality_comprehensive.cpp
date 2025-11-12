// SPDX-License-Identifier: MIT

#include <QtTest/QtTest>

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QStringList>
#include <QVector>

#include <memory>

#include <poppler/qt6/poppler-qt6.h>

#include "../../app/controller/DocumentController.h"
#include "../../app/controller/tool.hpp"
#include "../../app/managers/RecentFilesManager.h"
#include "../../app/model/DocumentModel.h"
#include "../../app/ui/core/ViewWidget.h"

#include "../TestUtilities.h"

namespace {

class StubRecentFilesManager : public RecentFilesManager {
    Q_OBJECT

public:
    explicit StubRecentFilesManager(QObject* parent = nullptr)
        : RecentFilesManager(parent) {}

    void addRecentFile(const QString& filePath) override {
        m_addedFiles.append(filePath);
        emit recentFileAdded(filePath);
    }

    [[nodiscard]] QStringList addedFiles() const { return m_addedFiles; }

private:
    QStringList m_addedFiles;
};

class StubDocumentModel : public DocumentModel {
    Q_OBJECT

public:
    explicit StubDocumentModel(QObject* parent = nullptr)
        : DocumentModel(), m_currentIndex(-1) {
        setParent(parent);
    }

    bool openFromFile(const QString& filePath) override {
        if (filePath.isEmpty() || !QFile::exists(filePath)) {
            emit loadingFailed(tr("File does not exist"), filePath);
            return false;
        }

        auto document = std::unique_ptr<Poppler::Document>(
            Poppler::Document::load(filePath));
        if (!document) {
            emit loadingFailed(tr("Unable to load PDF"), filePath);
            return false;
        }

        document->setRenderHint(Poppler::Document::Antialiasing, true);
        document->setRenderHint(Poppler::Document::TextAntialiasing, true);

        emit loadingStarted(filePath);

        const int newIndex = m_documents.size();
        m_paths.insert(newIndex, filePath);
        m_documents.insert(newIndex, std::move(document));
        m_currentIndex = newIndex;

        emit documentOpened(newIndex, QFileInfo(filePath).fileName());
        emit currentDocumentChanged(newIndex);
        emit loadingProgressChanged(100);
        return true;
    }

    bool openFromFiles(const QStringList& filePaths) override {
        bool anyOpened = false;
        for (const QString& path : filePaths) {
            anyOpened = openFromFile(path) || anyOpened;
        }
        return anyOpened;
    }

    bool closeDocument(int index) override {
        if (!isValidIndex(index)) {
            return false;
        }

        emit documentClosed(index);

        m_paths.removeAt(index);
        m_documents.removeAt(index);

        if (m_paths.isEmpty()) {
            m_currentIndex = -1;
            emit allDocumentsClosed();
        } else {
            if (m_currentIndex == index) {
                m_currentIndex = qMin(index, m_paths.size() - 1);
                emit currentDocumentChanged(m_currentIndex);
            } else if (index < m_currentIndex) {
                m_currentIndex -= 1;
                emit currentDocumentChanged(m_currentIndex);
            }
        }

        return true;
    }

    bool closeCurrentDocument() override {
        if (m_currentIndex < 0) {
            return false;
        }
        return closeDocument(m_currentIndex);
    }

    void switchToDocument(int index) override {
        if (!isValidIndex(index) || index == m_currentIndex) {
            return;
        }
        m_currentIndex = index;
        emit currentDocumentChanged(index);
    }

    int getDocumentCount() const override { return m_paths.size(); }

    int getCurrentDocumentIndex() const override { return m_currentIndex; }

    QString getCurrentFilePath() const override {
        return isValidIndex(m_currentIndex) ? m_paths[m_currentIndex]
                                            : QString();
    }

    QString getCurrentFileName() const override {
        return QFileInfo(getCurrentFilePath()).fileName();
    }

    QString getDocumentFileName(int index) const override {
        return isValidIndex(index) ? QFileInfo(m_paths[index]).fileName()
                                   : QString();
    }

    QString getDocumentFilePath(int index) const override {
        return isValidIndex(index) ? m_paths[index] : QString();
    }

    Poppler::Document* getCurrentDocument() const override {
        return getDocument(m_currentIndex);
    }

    Poppler::Document* getDocument(int index) const override {
        return isValidIndex(index) ? m_documents[index].get() : nullptr;
    }

    bool isEmpty() const override { return m_paths.isEmpty(); }

    bool isValidIndex(int index) const override {
        return index >= 0 && index < m_paths.size();
    }

    bool isNULL() override { return m_currentIndex < 0; }

private:
    QVector<QString> m_paths;
    QVector<std::unique_ptr<Poppler::Document>> m_documents;
    int m_currentIndex;
};

}  // namespace

class ViewWidgetFunctionalityTest : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override {
        TestBase::initTestCase();
        QString baseTemp =
            QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        QVERIFY2(!baseTemp.isEmpty(), "Temp location is unavailable");
        m_tempDir = QDir(baseTemp).filePath("sast_readium_viewwidget_tests");
        QVERIFY(QDir().mkpath(m_tempDir));
    }

    void cleanupTestCase() override {
        for (const QString& path : std::as_const(m_createdFiles)) {
            QFile::remove(path);
        }
        m_createdFiles.clear();
    }

    void init() override {
        m_recentFilesManager = std::make_unique<StubRecentFilesManager>();
        m_documentModel = std::make_unique<StubDocumentModel>();
        m_documentModel->setRecentFilesManager(m_recentFilesManager.get());

        m_documentController =
            std::make_unique<DocumentController>(m_documentModel.get());
        m_documentController->setRecentFilesManager(m_recentFilesManager.get());

        m_viewWidget = std::make_unique<ViewWidget>();
        m_viewWidget->setDocumentController(m_documentController.get());
        m_viewWidget->setDocumentModel(m_documentModel.get());
        m_viewWidget->resize(800, 600);
    }

    void cleanup() override {
        if (m_viewWidget) {
            m_viewWidget->close();
        }
        m_viewWidget.reset();
        m_documentController.reset();
        m_documentModel.reset();
        m_recentFilesManager.reset();

        for (const QString& path : std::as_const(m_createdFiles)) {
            QFile::remove(path);
        }
        m_createdFiles.clear();
    }

    void testOpenDocumentAddsRecentEntry();
    void testOpenDocumentSwitchesExisting();
    void testCloseDocumentClearsState();
    void testSwitchDocumentUpdatesCurrentIndex();
    void testExecutePdfActionEmitsZoomSignal();

private:
    QString createTestPdf(const QString& fileName, int pageCount = 3) {
        const QString pdfPath = QDir(m_tempDir).filePath(fileName);
        auto* document =
            TestDataGenerator::createTestPdfWithoutText(pageCount, pdfPath);
        QVERIFY2(document != nullptr, "Failed to create test PDF");
        delete document;
        m_createdFiles.append(pdfPath);
        return pdfPath;
    }

    std::unique_ptr<ViewWidget> m_viewWidget;
    std::unique_ptr<DocumentController> m_documentController;
    std::unique_ptr<StubDocumentModel> m_documentModel;
    std::unique_ptr<StubRecentFilesManager> m_recentFilesManager;
    QString m_tempDir;
    QStringList m_createdFiles;
};

void ViewWidgetFunctionalityTest::testOpenDocumentAddsRecentEntry() {
    const QString pdfPath = createTestPdf("open_document.pdf");

    QSignalSpy spy(m_documentModel.get(), &DocumentModel::documentOpened);

    m_viewWidget->openDocument(pdfPath);

    QVERIFY_TIMEOUT(spy.count() == 1, 1000);
    QVERIFY_TIMEOUT(m_viewWidget->hasDocuments(), 1000);
    QCOMPARE(m_documentModel->getDocumentCount(), 1);
    QCOMPARE(m_viewWidget->getCurrentDocumentIndex(), 0);
    QCOMPARE(m_recentFilesManager->addedFiles(), QStringList{pdfPath});
}

void ViewWidgetFunctionalityTest::testOpenDocumentSwitchesExisting() {
    const QString firstPath = createTestPdf("first_document.pdf");
    const QString secondPath = createTestPdf("second_document.pdf");

    m_viewWidget->openDocument(firstPath);
    QVERIFY_TIMEOUT(m_documentModel->getDocumentCount() == 1, 1000);

    m_viewWidget->openDocument(secondPath);
    QVERIFY_TIMEOUT(m_documentModel->getDocumentCount() == 2, 1000);
    QCOMPARE(m_viewWidget->getCurrentDocumentIndex(), 1);

    m_viewWidget->openDocument(firstPath);
    QVERIFY_TIMEOUT(m_viewWidget->getCurrentDocumentIndex() == 0, 1000);
    QCOMPARE(m_documentModel->getDocumentCount(), 2);
}

void ViewWidgetFunctionalityTest::testCloseDocumentClearsState() {
    const QString pdfPath = createTestPdf("close_document.pdf");

    m_viewWidget->openDocument(pdfPath);
    QVERIFY_TIMEOUT(m_viewWidget->hasDocuments(), 1000);

    m_viewWidget->closeDocument(0);

    QVERIFY_TIMEOUT(!m_viewWidget->hasDocuments(), 1000);
    QCOMPARE(m_documentModel->getDocumentCount(), 0);
    QCOMPARE(m_viewWidget->getCurrentDocumentIndex(), -1);
}

void ViewWidgetFunctionalityTest::testSwitchDocumentUpdatesCurrentIndex() {
    const QString firstPath = createTestPdf("switch_first.pdf");
    const QString secondPath = createTestPdf("switch_second.pdf");

    m_viewWidget->openDocument(firstPath);
    m_viewWidget->openDocument(secondPath);
    QVERIFY_TIMEOUT(m_documentModel->getCurrentDocumentIndex() == 1, 1000);

    QSignalSpy changeSpy(m_documentModel.get(),
                         &DocumentModel::currentDocumentChanged);

    m_viewWidget->switchToDocument(0);

    QVERIFY_TIMEOUT(m_documentModel->getCurrentDocumentIndex() == 0, 1000);
    QVERIFY_TIMEOUT(changeSpy.count() >= 1, 1000);
}

void ViewWidgetFunctionalityTest::testExecutePdfActionEmitsZoomSignal() {
    const QString pdfPath = createTestPdf("zoom_action.pdf");

    m_viewWidget->openDocument(pdfPath);
    QVERIFY_TIMEOUT(m_viewWidget->hasDocuments(), 1000);

    const double initialZoom = m_viewWidget->getCurrentZoom();

    QSignalSpy zoomSpy(m_viewWidget.get(),
                       &ViewWidget::currentViewerZoomChanged);
    m_viewWidget->executePDFAction(ActionMap::zoomIn);

    QVERIFY_TIMEOUT(zoomSpy.count() > 0, 1500);
    QVERIFY(m_viewWidget->getCurrentZoom() > initialZoom);
}

QTEST_MAIN(ViewWidgetFunctionalityTest)
#include "test_view_widget_functionality_comprehensive.moc"
