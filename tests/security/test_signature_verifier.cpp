#include <QSignalSpy>
#include <QTest>
#include "../../app/security/SignatureVerifier.h"
#include "../TestUtilities.h"

class TestSignatureVerifier : public TestBase {
    Q_OBJECT

private slots:
    void initTestCase() override { TestBase::initTestCase(); }

    void init() { m_verifier = new SignatureVerifier(); }

    void cleanup() {
        delete m_verifier;
        m_verifier = nullptr;
    }

    void testConstruction() { QVERIFY(m_verifier != nullptr); }

    void testHasSignaturesWithNull() {
        bool result = m_verifier->hasSignatures(nullptr);
        QVERIFY(!result);
    }

    void testGetSignatureCountWithNull() {
        int count = m_verifier->getSignatureCount(nullptr);
        QCOMPARE(count, 0);
    }

    void testVerifyDocumentWithNull() {
        QList<SignatureVerifier::SignatureInfo> signatures =
            m_verifier->verifyDocument(nullptr);
        QVERIFY(signatures.isEmpty());
    }

    void testSignatureInfoStruct() {
        SignatureVerifier::SignatureInfo info;
        QVERIFY(info.signerName.isEmpty());
        QVERIFY(info.reason.isEmpty());
        QVERIFY(info.location.isEmpty());
        QVERIFY(!info.signingTime.isValid());
        QVERIFY(!info.isValid);
        QVERIFY(info.certificateInfo.isEmpty());
        QVERIFY(info.errorMessage.isEmpty());
    }

    void testSignatureInfoPopulated() {
        SignatureVerifier::SignatureInfo info;
        info.signerName = "Test Signer";
        info.reason = "Document approval";
        info.location = "Test Location";
        info.signingTime = QDateTime::currentDateTime();
        info.isValid = true;
        info.certificateInfo = "Test Certificate";
        info.errorMessage = "";

        QCOMPARE(info.signerName, QString("Test Signer"));
        QCOMPARE(info.reason, QString("Document approval"));
        QCOMPARE(info.location, QString("Test Location"));
        QVERIFY(info.signingTime.isValid());
        QVERIFY(info.isValid);
        QCOMPARE(info.certificateInfo, QString("Test Certificate"));
        QVERIFY(info.errorMessage.isEmpty());
    }

    void testVerificationCompletedSignal() {
        QSignalSpy spy(m_verifier, &SignatureVerifier::verificationCompleted);
        QVERIFY(spy.isValid());
    }

    void testWithTestPdf() {
        auto* doc = TestDataGenerator::createTestPdfWithoutText(1);
        if (!doc) {
            QSKIP("Could not create test PDF");
            return;
        }

        bool hasSignatures = m_verifier->hasSignatures(doc);
        QVERIFY(!hasSignatures);

        int signatureCount = m_verifier->getSignatureCount(doc);
        QCOMPARE(signatureCount, 0);

        QList<SignatureVerifier::SignatureInfo> signatures =
            m_verifier->verifyDocument(doc);
        QVERIFY(signatures.isEmpty());

        delete doc;
    }

    void testMultipleVerifications() {
        auto* doc = TestDataGenerator::createTestPdfWithoutText(3);
        if (!doc) {
            QSKIP("Could not create test PDF");
            return;
        }

        for (int i = 0; i < 5; ++i) {
            m_verifier->hasSignatures(doc);
            m_verifier->getSignatureCount(doc);
            m_verifier->verifyDocument(doc);
        }

        delete doc;
    }

private:
    SignatureVerifier* m_verifier = nullptr;
};

QTEST_MAIN(TestSignatureVerifier)
#include "test_signature_verifier.moc"
