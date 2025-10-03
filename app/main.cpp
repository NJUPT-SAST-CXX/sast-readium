#include <config.h>
#include <QApplication>
#include <QStandardPaths>
#include <QString>
#include <QSysInfo>
#include <QTextStream>
#include <iostream>
#include "MainWindow.h"
#include "logging/SimpleLogging.h"
#include "managers/I18nManager.h"

// Function to print colored text to console
void printColored(const QString& text, const QString& color = "") {
#ifdef Q_OS_WIN
    // Windows console color codes
    if (color == "cyan") {
        std::cout << "\033[96m" << text.toStdString() << "\033[0m";
    } else if (color == "green") {
        std::cout << "\033[92m" << text.toStdString() << "\033[0m";
    } else if (color == "yellow") {
        std::cout << "\033[93m" << text.toStdString() << "\033[0m";
    } else if (color == "blue") {
        std::cout << "\033[94m" << text.toStdString() << "\033[0m";
    } else {
        std::cout << text.toStdString();
    }
#else
    // Unix/Linux console color codes
    if (color == "cyan") {
        std::cout << "\033[36m" << text.toStdString() << "\033[0m";
    } else if (color == "green") {
        std::cout << "\033[32m" << text.toStdString() << "\033[0m";
    } else if (color == "yellow") {
        std::cout << "\033[33m" << text.toStdString() << "\033[0m";
    } else if (color == "blue") {
        std::cout << "\033[34m" << text.toStdString() << "\033[0m";
    } else {
        std::cout << text.toStdString();
    }
#endif
    std::cout.flush();
}

// Function to print the application logo
void printLogo() {
    // SAST Readium ASCII Art Logo with gradient effect
    QString logoLines[] = {
        "\n",
        "    "
        "╔════════════════════════════════════════════════════════════════════"
        "╗",
        "    ║                                                                 "
        "   ║",
        "    ║   ███████╗ █████╗ ███████╗████████╗    ██████╗ ███████╗ █████╗  "
        " ║",
        "    ║   ██╔════╝██╔══██╗██╔════╝╚══██╔══╝    ██╔══██╗██╔════╝██╔══██╗ "
        " ║",
        "    ║   ███████╗███████║███████╗   ██║       ██████╔╝█████╗  ███████║ "
        " ║",
        "    ║   ╚════██║██╔══██║╚════██║   ██║       ██╔══██╗██╔══╝  ██╔══██║ "
        " ║",
        "    ║   ███████║██║  ██║███████║   ██║       ██║  ██║███████╗██║  ██║ "
        " ║",
        "    ║   ╚══════╝╚═╝  ╚═╝╚══════╝   ╚═╝       ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝ "
        " ║",
        "    ║                                                                 "
        "   ║",
        "    ║            ██████╗ ███████╗ █████╗ ██████╗ ██╗██╗   ██╗███╗   "
        "███╗║",
        "    ║            ██╔══██╗██╔════╝██╔══██╗██╔══██╗██║██║   ██║████╗ "
        "████║║",
        "    ║            ██████╔╝█████╗  ███████║██║  ██║██║██║   "
        "██║██╔████╔██║║",
        "    ║            ██╔══██╗██╔══╝  ██╔══██║██║  ██║██║██║   "
        "██║██║╚██╔╝██║║",
        "    ║            ██║  ██║███████╗██║  ██║██████╔╝██║╚██████╔╝██║ ╚═╝ "
        "██║║",
        "    ║            ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚═════╝ ╚═╝ ╚═════╝ ╚═╝     "
        "╚═╝║",
        "    ║                                                                 "
        "   ║",
        "    "
        "╚════════════════════════════════════════════════════════════════════"
        "╝"};

    // Print logo with color gradient (cyan to blue)
    for (int i = 0; i < 18; ++i) {
        if (i < 3 || i > 15) {
            printColored(logoLines[i] + "\n", "blue");
        } else if (i >= 3 && i <= 8) {
            printColored(logoLines[i] + "\n", "cyan");
        } else {
            printColored(logoLines[i] + "\n", "green");
        }
    }

    // Print tagline
    printColored("\n", "");
    printColored("                     🚀 ", "yellow");
    printColored("A Modern PDF Reader", "cyan");
    printColored(" • ", "yellow");
    printColored("Powered by Qt6 & Poppler", "green");
    printColored(" 🚀\n", "yellow");
    printColored("\n", "");
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    // Configure application metadata early
    app.setApplicationName(PROJECT_NAME);
    app.setApplicationVersion(PROJECT_VER);
    app.setApplicationDisplayName(APP_NAME);
    app.setStyle("fusion");

    // Print application logo to console
    printLogo();

    // Initialize logging system with simplified interface
    SastLogging::Config logConfig;
    logConfig.level = SastLogging::Level::Debug;  // Development mode
    logConfig.logFile = "sast-readium.log";
    logConfig.logDir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
        "/logs";
    logConfig.console = true;
    logConfig.file = true;
    logConfig.async = true;  // Enable async logging for better performance
    logConfig.maxFileSize = 50 * 1024 * 1024;  // 50MB per file
    logConfig.maxFiles = 5;                    // Keep 5 rotating files
    logConfig.pattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v";

    // Initialize logging
    if (!SastLogging::init(logConfig)) {
        // Fallback to console-only logging if file logging fails
        qWarning() << "Failed to initialize file logging:"
                   << SastLogging::getLastError();
        SastLogging::init("", true, SastLogging::Level::Debug);
    }

    // Log application startup information
    SLOG_INFO("Starting SAST Readium Application");
    SLOG_INFO_F("Version: %s", PROJECT_VER);
    SLOG_INFO_F("Qt Version: %s", QT_VERSION_STR);
    SLOG_INFO_F("Build Date: %s %s", __DATE__, __TIME__);
    SLOG_INFO_F("Platform: %s",
                QSysInfo::prettyProductName().toStdString().c_str());
    SLOG_INFO_F("Architecture: %s",
                QSysInfo::currentCpuArchitecture().toStdString().c_str());
    SLOG_DEBUG("Application style: fusion");
    SLOG_DEBUG("Log file: " + SastLogging::getCurrentLogFile());
    SLOG_INFO("──────────────────────────────────────────");

    // Create category loggers for different components
    SastLogging::CategoryLogger mainLogger("Main");
    mainLogger.setLevel(SastLogging::Level::Debug);
    mainLogger.debug("Application metadata configured");

    // Initialize i18n system
    if (!I18nManager::instance().initialize()) {
        mainLogger.error("Failed to initialize i18n system");
    } else {
        mainLogger.info("I18n system initialized successfully");
    }

    try {
        // Performance timing for startup
        SLOG_TIMER("ApplicationStartup");

        MainWindow w;
        mainLogger.info("Main window created successfully");

        w.show();
        mainLogger.info("Main window shown");
        SLOG_INFO("Application startup completed successfully");

        // Run the application event loop
        int result = QApplication::exec();

        SLOG_INFO_F("Application exiting with code: %d", result);
        SLOG_INFO("──────────────────────────────────────────");
        SLOG_INFO("SAST Readium Application Shutdown Complete");
        SLOG_INFO("Thank you for using SAST Readium!");

        // Flush and shutdown logging system
        SastLogging::flush();
        SastLogging::shutdown();

        return result;

    } catch (const std::exception& e) {
        SLOG_CRITICAL_F("Fatal error during application startup: %s", e.what());
        SastLogging::flush();
        SastLogging::shutdown();
        return -1;
    } catch (...) {
        SLOG_CRITICAL("Unknown fatal error during application startup");
        SastLogging::flush();
        SastLogging::shutdown();
        return -1;
    }
}
