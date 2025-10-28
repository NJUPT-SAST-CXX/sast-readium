#ifdef _WIN32
#include <windows.h>
#endif
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
#include "ui/widgets/EnhancedFocusIndicator.h"

// Function to enable Windows console UTF-8 and ANSI support
static void enableWindowsConsoleSupport() {
#ifdef Q_OS_WIN
    // Set console output to UTF-8
    SetConsoleOutputCP(CP_UTF8);

    // Enable ANSI escape sequences on Windows 10+
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
#endif
}

// Function to print colored text to console
static void printColored(const QString& text, const QString& color = "") {
#ifdef Q_OS_WIN
    // Windows console color codes (bright variants for better visibility)
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
static void printLogo() {
    // SAST Readium ASCII Art Logo with gradient effect
    QString logoLines[] = {
        "\n",
        "    "
        "╔════════════════════════════════════════════════════════════════════"
        "╗\n",
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
    // ============================================================================
    // High DPI Display Support Configuration
    // ============================================================================
    // High DPI scaling configuration for crisp rendering on 4K/Retina displays
    // Note: Qt 6 enables high DPI scaling by default, so
    // AA_EnableHighDpiScaling is deprecated and no longer needed

    // Use PassThrough policy for accurate DPI tracking
    // This ensures Qt uses the exact scale factor from the OS without rounding
    // Alternative policies: Round (rounds to nearest integer),
    // RoundPreferFloor, etc.
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    // Note: For testing high DPI on standard displays, set environment
    // variable: QT_SCALE_FACTOR=2.0 (or any desired scale factor)
    // ============================================================================

    // Enable Windows console UTF-8 and ANSI support BEFORE any console output
    enableWindowsConsoleSupport();

    QApplication app(argc, argv);

    // Initialize Qt resources from static library
    // This is required when resources are compiled into a static library
    // (app_lib)
    Q_INIT_RESOURCE(app);

    // Configure application metadata early
    QApplication::setApplicationName(PROJECT_NAME);
    QApplication::setApplicationVersion(PROJECT_VER);
    QApplication::setApplicationDisplayName(APP_NAME);
    QApplication::setStyle("fusion");

    // Print application logo to console (now with proper UTF-8 and ANSI
    // support)
    printLogo();

    // Initialize logging system with simplified interface
    SastLogging::Config logConfig;
    logConfig.level = SastLogging::Level::Info;  // Production mode
    logConfig.logFile = "sast-readium.log";

    // ROBUST FIX: Use multi-tier fallback for log directory
    // Tier 1: Try QStandardPaths (may fail on some systems/environments)
    // Tier 2: Fall back to ./logs (current directory)
    // Tier 3: Fall back to temp directory
    QString logDir;
    try {
        logDir =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (!logDir.isEmpty()) {
            logDir += "/logs";
            std::cout << "[INFO] Using AppData log directory: "
                      << logDir.toStdString() << '\n';
        } else {
            throw std::runtime_error("QStandardPaths returned empty path");
        }
    } catch (const std::exception& e) {
        std::cout << "[WARNING] QStandardPaths failed: " << e.what() << '\n';
        std::cout << "[INFO] Falling back to ./logs" << '\n';
        logDir = "./logs";
    }
    logConfig.logDir = logDir;

    logConfig.console = true;
    logConfig.file = true;
    logConfig.async = false;  // CRITICAL: Disable async logging to prevent
                              // QTimer hang before event loop starts
    logConfig.maxFileSize =
        static_cast<size_t>(50) * 1024 * 1024;  // 50MB per file
    logConfig.maxFiles = 5;                     // Keep 5 rotating files
    logConfig.pattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v";

    // Initialize logging with robust error handling
    if (!SastLogging::init(logConfig)) {
        // Fallback to console-only logging if file logging fails
        std::cerr << "[ERROR] Failed to initialize file logging: "
                  << SastLogging::getLastError().toStdString() << '\n';
        std::cerr << "[INFO] Falling back to console-only logging" << '\n';

        if (!SastLogging::init("", true, SastLogging::Level::Info)) {
            std::cerr << "[CRITICAL] Failed to initialize even console logging!"
                      << '\n';
            std::cerr << "[CRITICAL] Application will continue but logging is "
                         "disabled"
                      << '\n';
        }
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

    // Log high DPI configuration
    qreal devicePixelRatio = qApp->devicePixelRatio();
    SLOG_INFO_F("Device Pixel Ratio: %.2f", devicePixelRatio);
    SLOG_INFO_F("High DPI Scaling: %s", devicePixelRatio > 1.0
                                            ? "Enabled (High DPI Display)"
                                            : "Standard DPI");
    SLOG_DEBUG_F("High DPI Scale Factor Rounding Policy: PassThrough");

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

    // Install FocusManager for global keyboard navigation and focus indicators
    FocusManager::instance().installOnApplication();
    mainLogger.info("FocusManager installed for accessibility support");

    try {
        // Performance timing for startup
        SLOG_TIMER("ApplicationStartup");

        mainLogger.info("========== Creating MainWindow ==========");
        MainWindow w;
        mainLogger.info(
            "========== MainWindow created successfully ==========");

        mainLogger.info("========== Calling w.show() ==========");
        w.show();
        mainLogger.info("========== MainWindow shown successfully ==========");
        SLOG_INFO("Application startup completed successfully");

        // Run the application event loop
        mainLogger.info("========== Starting Qt event loop ==========");
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
