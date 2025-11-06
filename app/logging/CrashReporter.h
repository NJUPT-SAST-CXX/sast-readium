/**
 * @file CrashReporter.h
 * @brief User-friendly crash reporting dialog
 * @author SAST Readium Project
 * @version 1.0
 * @date 2025-10-31
 */

#pragma once

#include <QDialog>
#include <QString>
#include <memory>

// Forward declarations
class QTextEdit;
class QPushButton;
class QLabel;
struct CrashInfo;

/**
 * @brief User-friendly crash reporting dialog
 *
 * This dialog displays crash information to the user and provides
 * options to view details, copy information, and save the crash report.
 */
class CrashReporter : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param crashInfo Crash information to display
     * @param parent Parent widget
     */
    explicit CrashReporter(const CrashInfo& crashInfo,
                           QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~CrashReporter() override;

    /**
     * @brief Show crash reporter dialog
     * @param crashInfo Crash information
     * @param parent Parent widget
     * @return Dialog result
     */
    static int showCrashReport(const CrashInfo& crashInfo,
                               QWidget* parent = nullptr);

private slots:
    void onShowDetails();
    void onCopyToClipboard();
    void onOpenLogFile();

private:
    void setupUi();
    void populateInfo();
    QString formatCrashReport() const;

    const CrashInfo& m_crashInfo;

    // UI components
    QLabel* m_iconLabel;
    QLabel* m_messageLabel;
    QLabel* m_detailsLabel;
    QTextEdit* m_detailsText;
    QPushButton* m_showDetailsButton;
    QPushButton* m_copyButton;
    QPushButton* m_openLogButton;
    QPushButton* m_closeButton;

    bool m_detailsVisible;
};
