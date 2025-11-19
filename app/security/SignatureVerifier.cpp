#include "SignatureVerifier.h"
#include <poppler/qt6/poppler-form.h>

SignatureVerifier::SignatureVerifier(QObject* parent) : QObject(parent) {}

bool SignatureVerifier::hasSignatures(Poppler::Document* document) {
    if (!document) {
        return false;
    }

    for (int i = 0; i < document->numPages(); ++i) {
        std::unique_ptr<Poppler::Page> page(document->page(i));
        if (!page) {
            continue;
        }

        const auto fields = page->formFields();
        for (const auto& fieldPtr : fields) {
            Poppler::FormField* field = fieldPtr.get();
            if (field && field->type() == Poppler::FormField::FormSignature) {
                return true;
            }
        }
    }
    return false;
}

int SignatureVerifier::getSignatureCount(Poppler::Document* document) {
    if (!document) {
        return 0;
    }

    int count = 0;
    for (int i = 0; i < document->numPages(); ++i) {
        std::unique_ptr<Poppler::Page> page(document->page(i));
        if (!page) {
            continue;
        }

        const auto fields = page->formFields();
        for (const auto& fieldPtr : fields) {
            Poppler::FormField* field = fieldPtr.get();
            if (field && field->type() == Poppler::FormField::FormSignature) {
                count++;
            }
        }
    }
    return count;
}

QList<SignatureVerifier::SignatureInfo> SignatureVerifier::verifyDocument(
    Poppler::Document* document) {
    QList<SignatureInfo> signatures;

    if (!document) {
        return signatures;
    }

    for (int i = 0; i < document->numPages(); ++i) {
        std::unique_ptr<Poppler::Page> page(document->page(i));
        if (!page)
            continue;

        const auto fields = page->formFields();
        for (const auto& fieldPtr : fields) {
            Poppler::FormField* field = fieldPtr.get();
            if (field && field->type() == Poppler::FormField::FormSignature) {
                Poppler::FormFieldSignature* sigField =
                    static_cast<Poppler::FormFieldSignature*>(field);
                SignatureInfo info = extractSignatureInfo(sigField);
                signatures.append(info);
            }
        }
    }

    int validCount = 0;
    for (const SignatureInfo& sig : signatures) {
        if (sig.isValid)
            validCount++;
    }

    emit verificationCompleted(signatures.size(), validCount);
    return signatures;
}

SignatureVerifier::SignatureInfo SignatureVerifier::extractSignatureInfo(
    Poppler::FormFieldSignature* signatureField) {
    SignatureInfo info;

    if (!signatureField) {
        info.isValid = false;
        info.errorMessage = "Invalid signature field";
        return info;
    }

    // Extract signature validation status
    Poppler::SignatureValidationInfo validInfo = signatureField->validate(
        Poppler::FormFieldSignature::ValidateVerifyCertificate);

    info.isValid = (validInfo.signatureStatus() ==
                    Poppler::SignatureValidationInfo::SignatureValid);
    info.signerName = validInfo.signerName();
    info.signingTime =
        QDateTime::currentDateTime();  // Would get from signature

    if (!info.isValid) {
        info.errorMessage = "Signature validation failed";
    }

    return info;
}
