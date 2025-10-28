#pragma once

#include <poppler/qt6/poppler-qt6.h>
#include <QList>
#include <QObject>
#include <QString>

/**
 * @brief Digital signature verification (Feature 16)
 */
class SignatureVerifier : public QObject {
    Q_OBJECT

public:
    struct SignatureInfo {
        QString signerName;
        QString reason;
        QString location;
        QDateTime signingTime;
        bool isValid;
        QString certificateInfo;
        QString errorMessage;
    };

    explicit SignatureVerifier(QObject* parent = nullptr);
    ~SignatureVerifier() = default;

    QList<SignatureInfo> verifyDocument(Poppler::Document* document);
    bool hasSignatures(Poppler::Document* document);
    int getSignatureCount(Poppler::Document* document);

signals:
    void verificationCompleted(int totalSignatures, int validSignatures);

private:
    SignatureInfo extractSignatureInfo(
        Poppler::FormFieldSignature* signatureField);
};
