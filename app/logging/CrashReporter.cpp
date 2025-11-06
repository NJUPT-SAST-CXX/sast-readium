/**
 * @file CrashReporter.cpp
 * @brief User-friendly crash reporting dialog implementation
 * @author SAST Readium Project
 * @version 1.0
 * @date 2025-10-31
 */

#include "CrashReporter.h"
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QTextEdit>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include "CrashHandler.h"

CrashReporter::CrashReporter(const CrashInfo& crashInfo, QWidget* parent)
    : QDialog(parent), m_crashInfo(crashInfo), m_detailsVisible(false) {
    setupUi();
    populateInfo();

    setWindowTitle(tr("Application Crash"));
    setWindowIcon(style()->standardIcon(QStyle::SP_MessageBoxCritical));
    setModal(true);

    // Set minimum size
    setMinimumWidth(500);
}

CrashReporter::~CrashReporter() = default;

int CrashReporter::showCrashReport(const CrashInfo& crashInfo,
                                   QWidget* parent) {
    CrashReporter dialog(crashInfo, parent);
    return dialog.exec();
}

void CrashReporter::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Header with icon and message
    auto* headerLayout = new QHBoxLayout();

    m_iconLabel = new QLabel(this);
    QIcon icon = style()->standardIcon(QStyle::SP_MessageBoxCritical);
    m_iconLabel->setPixmap(icon.pixmap(48, 48));
    m_iconLabel->setAlignment(Qt::AlignTop);
    headerLayout->addWidget(m_iconLabel);

    m_messageLabel = new QLabel(this);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setTextFormat(Qt::RichText);
    headerLayout->addWidget(m_messageLabel, 1);

    mainLayout->addLayout(headerLayout);

    // Details label
    m_detailsLabel = new QLabel(this);
    m_detailsLabel->setWordWrap(true);
    mainLayout->addWidget(m_detailsLabel);

    // Details text (initially hidden)
    m_detailsText = new QTextEdit(this);
    m_detailsText->setReadOnly(true);
    m_detailsText->setFont(QFont("Courier New", 9));
    m_detailsText->setMinimumHeight(300);
    m_detailsText->setVisible(false);
    mainLayout->addWidget(m_detailsText);

    // Button layout
    auto* buttonLayout = new QHBoxLayout();

    m_showDetailsButton = new QPushButton(tr("Show Details >>"), this);
    connect(m_showDetailsButton, &QPushButton::clicked, this,
            &CrashReporter::onShowDetails);
    buttonLayout->addWidget(m_showDetailsButton);

    m_copyButton = new QPushButton(tr("Copy to Clipboard"), this);
    connect(m_copyButton, &QPushButton::clicked, this,
            &CrashReporter::onCopyToClipboard);
    buttonLayout->addWidget(m_copyButton);

    m_openLogButton = new QPushButton(tr("Open Log File"), this);
    connect(m_openLogButton, &QPushButton::clicked, this,
            &CrashReporter::onOpenLogFile);
    buttonLayout->addWidget(m_openLogButton);

    buttonLayout->addStretch();

    m_closeButton = new QPushButton(tr("Close"), this);
    m_closeButton->setDefault(true);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(m_closeButton);

    mainLayout->addLayout(buttonLayout);
}

void CrashReporter::populateInfo() {
    // Main message
    QString message =
        QString(
            "<h3>%1</h3>"
            "<p>%2</p>")
            .arg(tr("The application has encountered a critical error"),
                 tr("The application needs to close. We apologize for the "
                    "inconvenience."));
    m_messageLabel->setText(message);

    // Details label
    QString details =
        QString(
            "<b>%1:</b> %2<br>"
            "<b>%3:</b> %4<br>"
            "<b>%5:</b> %6")
            .arg(tr("Error Type"), m_crashInfo.exceptionType, tr("Time"),
                 m_crashInfo.timestamp.toString("yyyy-MM-dd HH:mm:ss"),
                 tr("Log File"), QFileInfo(m_crashInfo.logFilePath).fileName());
    m_detailsLabel->setText(details);

    // Details text
    m_detailsText->setPlainText(formatCrashReport());
}

QString CrashReporter::formatCrashReport() const {
    QString report;

    report +=
        "======================================================================"
        "==========\n";
    report += "                        CRASH REPORT\n";
    report +=
        "======================================================================"
        "==========\n\n";

    report += QString("Time: %1\n")
                  .arg(m_crashInfo.timestamp.toString("yyyy-MM-dd HH:mm:ss"));
    report += QString("Exception Type: %1\n").arg(m_crashInfo.exceptionType);
    report +=
        QString("Exception Message: %1\n\n").arg(m_crashInfo.exceptionMessage);

    report += "Application Information:\n";
    report += QString("  Version: %1\n").arg(m_crashInfo.applicationVersion);
    report += QString("  Qt Version: %1\n").arg(m_crashInfo.qtVersion);
    report += QString("  Platform: %1\n").arg(m_crashInfo.platform);
    report += QString("  Architecture: %1\n\n").arg(m_crashInfo.architecture);

    report += QString("Thread: %1\n\n").arg(m_crashInfo.threadInfo);

    if (!m_crashInfo.lastOperation.isEmpty()) {
        report +=
            QString("Last Operation: %1\n\n").arg(m_crashInfo.lastOperation);
    }

    if (!m_crashInfo.customData.isEmpty()) {
        report += "Context Data:\n";
        for (auto it = m_crashInfo.customData.constBegin();
             it != m_crashInfo.customData.constEnd(); ++it) {
            report += QString("  %1: %2\n").arg(it.key(), it.value());
        }
        report += "\n";
    }

    report += "Stack Trace:\n";
    report += m_crashInfo.stackTrace;
    report += "\n";

    report += QString("Log File: %1\n").arg(m_crashInfo.logFilePath);

    return report;
}

void CrashReporter::onShowDetails() {
    m_detailsVisible = !m_detailsVisible;
    m_detailsText->setVisible(m_detailsVisible);

    if (m_detailsVisible) {
        m_showDetailsButton->setText(tr("Hide Details <<"));
        resize(width(), height() + 300);
    } else {
        m_showDetailsButton->setText(tr("Show Details >>"));
        resize(width(), height() - 300);
    }
}

void CrashReporter::onCopyToClipboard() {
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(formatCrashReport());

    // Provide visual feedback
    m_copyButton->setText(tr("Copied!"));
    QTimer::singleShot(2000, this, [this]() {
        m_copyButton->setText(tr("Copy to Clipboard"));
    });
}

void CrashReporter::onOpenLogFile() {
    if (!m_crashInfo.logFilePath.isEmpty()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_crashInfo.logFilePath));
    }
}
