#ifdef _WIN32
#include <windows.h>
#endif
#include <config.h>
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QString>
#include <QSysInfo>
#include <QTextStream>
#include <QTimer>
#include <iostream>
#include "MainWindow.h"
#include "cache/CacheManager.h"
#include "logging/SimpleLogging.h"
#include "managers/I18nManager.h"
#include "managers/StyleManager.h"
#include "ui/widgets/EnhancedFocusIndicator.h"

// ============================================================================
// Command-Line Configuration Structure
// ============================================================================

struct CommandLineConfig {
    // File to open
    QString filePath;

    // Window geometry
    bool hasGeometry = false;
    int windowX = -1;
    int windowY = -1;
    int windowWidth = 1280;
    int windowHeight = 800;
    bool maximized = false;
    bool minimized = false;
    bool fullscreen = false;

    // View settings
    bool hasViewMode = false;
    int viewMode = 0;  // 0=SinglePage, 1=Continuous, 2=TwoPages, 3=BookView
    bool hasZoom = false;
    double zoomLevel = 1.0;
    bool hasPage = false;
    int pageNumber = 1;

    // Application settings
    bool hasTheme = false;
    QString theme = "light";
    bool hasLanguage = false;
    QString language = "system";
    bool hasLogLevel = false;
    QString logLevel = "info";

    // Cache settings
    bool hasCacheSize = false;
    qint64 cacheSize = 512 * 1024 * 1024;  // 512MB default
};

// ============================================================================
// Command-Line Validation Functions
// ============================================================================

static bool validateFilePath(const QString& path, QString& errorMsg) {
    if (path.isEmpty()) {
        return true;  // Empty is valid (no file to open)
    }

    QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        errorMsg = QString("File does not exist: %1").arg(path);
        return false;
    }

    if (!fileInfo.isReadable()) {
        errorMsg = QString("File is not readable: %1").arg(path);
        return false;
    }

    if (!fileInfo.isFile()) {
        errorMsg = QString("Path is not a file: %1").arg(path);
        return false;
    }

    QString suffix = fileInfo.suffix().toLower();
    if (suffix != "pdf") {
        errorMsg = QString("File is not a PDF: %1").arg(path);
        return false;
    }

    return true;
}

static bool validateViewMode(int mode, QString& errorMsg) {
    if (mode < 0 || mode > 3) {
        errorMsg = QString("Invalid view mode: %1 (must be 0-3)").arg(mode);
        return false;
    }
    return true;
}

static bool validateZoomLevel(double zoom, QString& errorMsg) {
    if (zoom < 0.1 || zoom > 10.0) {
        errorMsg =
            QString("Invalid zoom level: %1 (must be 0.1-10.0)").arg(zoom);
        return false;
    }
    return true;
}

static bool validatePageNumber(int page, QString& errorMsg) {
    if (page < 1) {
        errorMsg = QString("Invalid page number: %1 (must be >= 1)").arg(page);
        return false;
    }
    return true;
}

static bool validateTheme(const QString& theme, QString& errorMsg) {
    if (theme != "light" && theme != "dark") {
        errorMsg =
            QString("Invalid theme: %1 (must be 'light' or 'dark')").arg(theme);
        return false;
    }
    return true;
}

static bool validateLanguage(const QString& lang, QString& errorMsg) {
    if (lang != "en" && lang != "zh" && lang != "system") {
        errorMsg =
            QString("Invalid language: %1 (must be 'en', 'zh', or 'system')")
                .arg(lang);
        return false;
    }
    return true;
}

static bool validateLogLevel(const QString& level, QString& errorMsg) {
    QString lower = level.toLower();
    if (lower != "trace" && lower != "debug" && lower != "info" &&
        lower != "warning" && lower != "error" && lower != "critical" &&
        lower != "off") {
        errorMsg = QString(
                       "Invalid log level: %1 (must be trace, debug, info, "
                       "warning, error, critical, or off)")
                       .arg(level);
        return false;
    }
    return true;
}

static bool validateCacheSize(qint64 size, QString& errorMsg) {
    const qint64 minSize = 1 * 1024 * 1024;            // 1MB
    const qint64 maxSize = 10 * 1024 * 1024 * 1024LL;  // 10GB
    if (size < minSize || size > maxSize) {
        errorMsg = QString("Invalid cache size: %1 MB (must be 1-10240 MB)")
                       .arg(size / (1024 * 1024));
        return false;
    }
    return true;
}

static bool validateWindowDimensions(int width, int height, QString& errorMsg) {
    if (width < 100 || width > 10000) {
        errorMsg =
            QString("Invalid window width: %1 (must be 100-10000)").arg(width);
        return false;
    }
    if (height < 100 || height > 10000) {
        errorMsg = QString("Invalid window height: %1 (must be 100-10000)")
                       .arg(height);
        return false;
    }
    return true;
}

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

// Helper function to print error messages (supports i18n after QApplication is
// created)
static void printError(const QString& message) {
    // Use QCoreApplication::translate for i18n support
    // Context is "CommandLine" for all command-line error messages
    QString translated = QCoreApplication::translate(
        "CommandLine", message.toUtf8().constData());
    std::cerr << "[ERROR] " << translated.toStdString() << '\n';
}

// Function to print the application logo
static void printLogo() {
    // SAST Readium ASCII Art Logo with gradient effect
    // Each line is a complete string with newline already included
    QString logoLines[] = {
        "\n",
        "    "
        "╔════════════════════════════════════════════════════════════════════╗"
        "\n",
        "    ║                                                                 "
        "   ║\n",
        "    ║   ███████╗ █████╗ ███████╗████████╗    ██████╗ ███████╗ █████╗  "
        " ║\n",
        "    ║   ██╔════╝██╔══██╗██╔════╝╚══██╔══╝    ██╔══██╗██╔════╝██╔══██╗ "
        " ║\n",
        "    ║   ███████╗███████║███████╗   ██║       ██████╔╝█████╗  ███████║ "
        " ║\n",
        "    ║   ╚════██║██╔══██║╚════██║   ██║       ██╔══██╗██╔══╝  ██╔══██║ "
        " ║\n",
        "    ║   ███████║██║  ██║███████║   ██║       ██║  ██║███████╗██║  ██║ "
        " ║\n",
        "    ║   ╚══════╝╚═╝  ╚═╝╚══════╝   ╚═╝       ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝ "
        " ║\n",
        "    ║                                                                 "
        "   ║\n",
        "    ║            ██████╗ ███████╗ █████╗ ██████╗ ██╗██╗   ██╗███╗   "
        "███╗║\n",
        "    ║            ██╔══██╗██╔════╝██╔══██╗██╔══██╗██║██║   ██║████╗ "
        "████║║\n",
        "    ║            ██████╔╝█████╗  ███████║██║  ██║██║██║   "
        "██║██╔████╔██║║\n",
        "    ║            ██╔══██╗██╔══╝  ██╔══██║██║  ██║██║██║   "
        "██║██║╚██╔╝██║║\n",
        "    ║            ██║  ██║███████╗██║  ██║██████╔╝██║╚██████╔╝██║ ╚═╝ "
        "██║║\n",
        "    ║            ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚═════╝ ╚═╝ ╚═════╝ ╚═╝     "
        "╚═╝║\n",
        "    ║                                                                 "
        "   ║\n",
        "    "
        "╚════════════════════════════════════════════════════════════════════╝"
        "\n"};

    // Print logo with color gradient (cyan to blue)
    for (int i = 0; i < 18; ++i) {
        if (i < 3 || i > 15) {
            printColored(logoLines[i], "blue");
        } else if (i >= 3 && i <= 8) {
            printColored(logoLines[i], "cyan");
        } else {
            printColored(logoLines[i], "green");
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

    // ============================================================================
    // Command-Line Argument Parsing
    // ============================================================================

    // Configure application metadata for command-line parser
    QApplication::setApplicationName(PROJECT_NAME);
    QApplication::setApplicationVersion(PROJECT_VER);
    QApplication::setApplicationDisplayName(APP_NAME);

    // Setup command-line parser
    QCommandLineParser parser;
    parser.setApplicationDescription(
        "SAST Readium - A Modern PDF Reader powered by Qt6 & Poppler");
    parser.addHelpOption();
    parser.addVersionOption();

    // File argument (positional)
    parser.addPositionalArgument("file", "PDF file to open", "[file]");

    // Window geometry options
    QCommandLineOption geometryOption(
        QStringList() << "g" << "geometry",
        "Set window geometry as WIDTHxHEIGHT+X+Y (e.g., 1280x800+100+100)",
        "geometry");
    parser.addOption(geometryOption);

    QCommandLineOption maximizedOption(QStringList() << "m" << "maximized",
                                       "Start with maximized window");
    parser.addOption(maximizedOption);

    QCommandLineOption minimizedOption("minimized",
                                       "Start with minimized window");
    parser.addOption(minimizedOption);

    QCommandLineOption fullscreenOption(QStringList() << "f" << "fullscreen",
                                        "Start in fullscreen mode");
    parser.addOption(fullscreenOption);

    // View mode options
    QCommandLineOption viewModeOption("view-mode",
                                      "Set initial view mode: single (0), "
                                      "continuous (1), two-pages (2), book (3)",
                                      "mode", "single");
    parser.addOption(viewModeOption);

    // Zoom level option
    QCommandLineOption zoomOption(
        QStringList() << "z" << "zoom",
        "Set initial zoom level (0.1-10.0, or fit-width, fit-height, fit-page)",
        "zoom");
    parser.addOption(zoomOption);

    // Page number option
    QCommandLineOption pageOption(QStringList() << "p" << "page",
                                  "Open to specific page number", "page");
    parser.addOption(pageOption);

    // Theme option
    QCommandLineOption themeOption(QStringList() << "t" << "theme",
                                   "Set theme: light or dark", "theme");
    parser.addOption(themeOption);

    // Language option
    QCommandLineOption languageOption(QStringList() << "l" << "language",
                                      "Set language: en, zh, or system",
                                      "language");
    parser.addOption(languageOption);

    // Logging level option
    QCommandLineOption logLevelOption(
        "log-level",
        "Set logging level: trace, debug, info, warning, error, critical, off",
        "level");
    parser.addOption(logLevelOption);

    // Cache size option
    QCommandLineOption cacheSizeOption(
        "cache-size", "Set cache size in MB (1-10240)", "size");
    parser.addOption(cacheSizeOption);

    // Parse command-line arguments
    parser.process(app);

    // ============================================================================
    // Parse and Validate Command-Line Arguments
    // ============================================================================

    CommandLineConfig cmdConfig;
    QString errorMsg;

    // Parse positional file argument
    const QStringList positionalArgs = parser.positionalArguments();
    if (!positionalArgs.isEmpty()) {
        cmdConfig.filePath = positionalArgs.first();
        if (!validateFilePath(cmdConfig.filePath, errorMsg)) {
            printError(errorMsg);
            return 1;
        }
    }

    // Parse geometry option
    if (parser.isSet(geometryOption)) {
        QString geometry = parser.value(geometryOption);
        QRegularExpression geomRegex(R"((\d+)x(\d+)(?:\+(-?\d+)\+(-?\d+))?)");
        QRegularExpressionMatch match = geomRegex.match(geometry);

        if (match.hasMatch()) {
            cmdConfig.windowWidth = match.captured(1).toInt();
            cmdConfig.windowHeight = match.captured(2).toInt();
            if (match.lastCapturedIndex() >= 4) {
                cmdConfig.windowX = match.captured(3).toInt();
                cmdConfig.windowY = match.captured(4).toInt();
            }

            if (!validateWindowDimensions(cmdConfig.windowWidth,
                                          cmdConfig.windowHeight, errorMsg)) {
                printError(errorMsg);
                return 1;
            }
            cmdConfig.hasGeometry = true;
        } else {
            printError(QString("Invalid geometry format: %1 (expected "
                               "WIDTHxHEIGHT or WIDTHxHEIGHT+X+Y)")
                           .arg(geometry));
            return 1;
        }
    }

    // Parse window state options (maximized takes precedence over geometry)
    if (parser.isSet(maximizedOption)) {
        cmdConfig.maximized = true;
        cmdConfig.hasGeometry = true;  // Override geometry
    }
    if (parser.isSet(minimizedOption)) {
        cmdConfig.minimized = true;
    }
    if (parser.isSet(fullscreenOption)) {
        cmdConfig.fullscreen = true;
        cmdConfig.hasGeometry = true;  // Override geometry
    }

    // Parse view mode option
    if (parser.isSet(viewModeOption)) {
        QString viewModeStr = parser.value(viewModeOption).toLower();
        if (viewModeStr == "single" || viewModeStr == "0") {
            cmdConfig.viewMode = 0;
        } else if (viewModeStr == "continuous" || viewModeStr == "1") {
            cmdConfig.viewMode = 1;
        } else if (viewModeStr == "two-pages" || viewModeStr == "2") {
            cmdConfig.viewMode = 2;
        } else if (viewModeStr == "book" || viewModeStr == "3") {
            cmdConfig.viewMode = 3;
        } else {
            printError(QString("Invalid view mode: %1 (must be single, "
                               "continuous, two-pages, book, or 0-3)")
                           .arg(viewModeStr));
            return 1;
        }

        if (!validateViewMode(cmdConfig.viewMode, errorMsg)) {
            printError(errorMsg);
            return 1;
        }
        cmdConfig.hasViewMode = true;
    }

    // Parse zoom option
    if (parser.isSet(zoomOption)) {
        QString zoomStr = parser.value(zoomOption).toLower();
        if (zoomStr == "fit-width" || zoomStr == "fit-height" ||
            zoomStr == "fit-page") {
            // Special zoom modes - will be handled after document is loaded
            // For now, just set to 1.0 and let the view mode handle it
            cmdConfig.zoomLevel = 1.0;
            cmdConfig.hasZoom = true;
        } else {
            bool ok = false;
            double zoom = zoomStr.toDouble(&ok);
            if (!ok) {
                printError(QString("Invalid zoom value: %1 (must be a number "
                                   "or fit-width, fit-height, fit-page)")
                               .arg(zoomStr));
                return 1;
            }

            if (!validateZoomLevel(zoom, errorMsg)) {
                printError(errorMsg);
                return 1;
            }
            cmdConfig.zoomLevel = zoom;
            cmdConfig.hasZoom = true;
        }
    }

    // Parse page option
    if (parser.isSet(pageOption)) {
        bool ok = false;
        int page = parser.value(pageOption).toInt(&ok);
        if (!ok) {
            printError(QString("Invalid page number: %1")
                           .arg(parser.value(pageOption)));
            return 1;
        }

        if (!validatePageNumber(page, errorMsg)) {
            printError(errorMsg);
            return 1;
        }
        cmdConfig.pageNumber = page;
        cmdConfig.hasPage = true;
    }

    // Parse theme option
    if (parser.isSet(themeOption)) {
        cmdConfig.theme = parser.value(themeOption).toLower();
        if (!validateTheme(cmdConfig.theme, errorMsg)) {
            printError(errorMsg);
            return 1;
        }
        cmdConfig.hasTheme = true;
    }

    // Parse language option
    if (parser.isSet(languageOption)) {
        cmdConfig.language = parser.value(languageOption).toLower();
        if (!validateLanguage(cmdConfig.language, errorMsg)) {
            printError(errorMsg);
            return 1;
        }
        cmdConfig.hasLanguage = true;
    }

    // Parse log level option
    if (parser.isSet(logLevelOption)) {
        cmdConfig.logLevel = parser.value(logLevelOption).toLower();
        if (!validateLogLevel(cmdConfig.logLevel, errorMsg)) {
            printError(errorMsg);
            return 1;
        }
        cmdConfig.hasLogLevel = true;
    }

    // Parse cache size option
    if (parser.isSet(cacheSizeOption)) {
        bool ok = false;
        qint64 sizeMB = parser.value(cacheSizeOption).toLongLong(&ok);
        if (!ok) {
            printError(QString("Invalid cache size: %1")
                           .arg(parser.value(cacheSizeOption)));
            return 1;
        }

        cmdConfig.cacheSize = sizeMB * 1024 * 1024;  // Convert MB to bytes
        if (!validateCacheSize(cmdConfig.cacheSize, errorMsg)) {
            printError(errorMsg);
            return 1;
        }
        cmdConfig.hasCacheSize = true;
    }

    // ============================================================================
    // End of Command-Line Argument Parsing
    // ============================================================================

    // Initialize Qt resources from static library
    // This is required when resources are compiled into a static library
    // (app_lib)
    Q_INIT_RESOURCE(app);

    // Configure application style
    QApplication::setStyle("fusion");

    // Print application logo to console (now with proper UTF-8 and ANSI
    // support)
    printLogo();

    // ============================================================================
    // Initialize Logging System with Command-Line Configuration
    // ============================================================================

    SastLogging::Config logConfig;

    // Apply command-line log level if specified
    if (cmdConfig.hasLogLevel) {
        QString level = cmdConfig.logLevel.toLower();
        if (level == "trace") {
            logConfig.level = SastLogging::Level::Trace;
        } else if (level == "debug") {
            logConfig.level = SastLogging::Level::Debug;
        } else if (level == "info") {
            logConfig.level = SastLogging::Level::Info;
        } else if (level == "warning") {
            logConfig.level = SastLogging::Level::Warning;
        } else if (level == "error") {
            logConfig.level = SastLogging::Level::Error;
        } else if (level == "critical") {
            logConfig.level = SastLogging::Level::Critical;
        } else if (level == "off") {
            logConfig.level = SastLogging::Level::Off;
        }
    } else {
        logConfig.level = SastLogging::Level::Info;  // Default production mode
    }

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

    // ============================================================================
    // Initialize I18n System with Command-Line Configuration
    // ============================================================================

    if (cmdConfig.hasLanguage) {
        // Load specific language from command line
        mainLogger.info("Loading language from command line: " +
                        cmdConfig.language);
        if (!I18nManager::instance().loadLanguage(cmdConfig.language)) {
            mainLogger.error("Failed to load language: " + cmdConfig.language);
            // Fall back to system language
            if (!I18nManager::instance().initialize()) {
                mainLogger.error("Failed to initialize i18n system");
            }
        } else {
            mainLogger.info("Language loaded successfully: " +
                            cmdConfig.language);
        }
    } else {
        // Use default initialization (system language)
        if (!I18nManager::instance().initialize()) {
            mainLogger.error("Failed to initialize i18n system");
        } else {
            mainLogger.info("I18n system initialized successfully");
        }
    }

    // Install FocusManager for global keyboard navigation and focus indicators
    FocusManager::instance().installOnApplication();
    mainLogger.info("FocusManager installed for accessibility support");

    // ============================================================================
    // Configure Cache System with Command-Line Configuration
    // ============================================================================

    if (cmdConfig.hasCacheSize) {
        mainLogger.info(
            QString("Configuring cache size from command line: %1 MB")
                .arg(cmdConfig.cacheSize / (1024 * 1024)));

        CacheManager::GlobalCacheConfig cacheConfig =
            CacheManager::instance().getGlobalConfig();
        cacheConfig.totalMemoryLimit = cmdConfig.cacheSize;

        // Distribute cache size proportionally across cache types
        qint64 searchCache = cmdConfig.cacheSize * 100 / 512;    // ~20%
        qint64 pageTextCache = cmdConfig.cacheSize * 50 / 512;   // ~10%
        qint64 highlightCache = cmdConfig.cacheSize * 25 / 512;  // ~5%
        qint64 renderCache = cmdConfig.cacheSize * 256 / 512;    // ~50%
        qint64 thumbnailCache = cmdConfig.cacheSize * 81 / 512;  // ~15%

        cacheConfig.searchResultCacheLimit = searchCache;
        cacheConfig.pageTextCacheLimit = pageTextCache;
        cacheConfig.searchHighlightCacheLimit = highlightCache;
        cacheConfig.pdfRenderCacheLimit = renderCache;
        cacheConfig.thumbnailCacheLimit = thumbnailCache;

        CacheManager::instance().setGlobalConfig(cacheConfig);
        mainLogger.info("Cache configuration applied successfully");
    }

    try {
        // Performance timing for startup
        SLOG_TIMER("ApplicationStartup");

        mainLogger.info("========== Creating MainWindow ==========");
        MainWindow w;
        mainLogger.info(
            "========== MainWindow created successfully ==========");

        // ====================================================================
        // Apply Command-Line Window Configuration
        // ====================================================================

        if (cmdConfig.hasGeometry) {
            if (cmdConfig.maximized) {
                mainLogger.info("Setting window to maximized state");
                // Don't call showMaximized() yet, just set the state
                // We'll show it properly below
            } else if (cmdConfig.fullscreen) {
                mainLogger.info("Setting window to fullscreen state");
                // Don't call showFullScreen() yet
            } else {
                // Apply custom geometry
                if (cmdConfig.windowX >= 0 && cmdConfig.windowY >= 0) {
                    mainLogger.info(
                        QString("Setting window geometry: %1x%2 at (%3,%4)")
                            .arg(cmdConfig.windowWidth)
                            .arg(cmdConfig.windowHeight)
                            .arg(cmdConfig.windowX)
                            .arg(cmdConfig.windowY));
                    w.setGeometry(cmdConfig.windowX, cmdConfig.windowY,
                                  cmdConfig.windowWidth,
                                  cmdConfig.windowHeight);
                } else {
                    mainLogger.info(QString("Setting window size: %1x%2")
                                        .arg(cmdConfig.windowWidth)
                                        .arg(cmdConfig.windowHeight));
                    w.resize(cmdConfig.windowWidth, cmdConfig.windowHeight);
                }
            }
        }

        // ====================================================================
        // Apply Command-Line Theme Configuration
        // ====================================================================

        if (cmdConfig.hasTheme) {
            mainLogger.info("Applying theme from command line: " +
                            cmdConfig.theme);
            Theme themeEnum =
                (cmdConfig.theme == "dark") ? Theme::Dark : Theme::Light;
            StyleManager::instance().setTheme(themeEnum);
        }

        mainLogger.info("========== Calling w.show() ==========");

        // Show window with appropriate state
        if (cmdConfig.fullscreen) {
            w.showFullScreen();
            mainLogger.info("Window shown in fullscreen mode");
        } else if (cmdConfig.maximized) {
            w.showMaximized();
            mainLogger.info("Window shown maximized");
        } else if (cmdConfig.minimized) {
            w.showMinimized();
            mainLogger.info("Window shown minimized");
        } else {
            w.show();
            mainLogger.info("Window shown normally");
        }

        mainLogger.info("========== MainWindow shown successfully ==========");
        SLOG_INFO("Application startup completed successfully");

        // ====================================================================
        // Apply Command-Line File Opening and View Configuration
        // ====================================================================

        // Use QTimer::singleShot to defer file opening and view configuration
        // until after event loop starts. This ensures MainWindow and all
        // controllers are fully initialized before we attempt to open files
        // or change view settings.
        if (!cmdConfig.filePath.isEmpty() || cmdConfig.hasViewMode ||
            cmdConfig.hasZoom || cmdConfig.hasPage) {
            QTimer::singleShot(100, &w, [&w, cmdConfig, &mainLogger]() {
                // Apply view mode first (before opening file)
                if (cmdConfig.hasViewMode) {
                    mainLogger.info(
                        QString("Setting view mode from command line: %1")
                            .arg(cmdConfig.viewMode));
                    w.setViewModeFromCommandLine(cmdConfig.viewMode);
                }

                // Apply zoom level (before opening file)
                if (cmdConfig.hasZoom) {
                    mainLogger.info(
                        QString("Setting zoom level from command line: %1")
                            .arg(cmdConfig.zoomLevel));
                    w.setZoomLevelFromCommandLine(cmdConfig.zoomLevel);
                }

                // Open file if specified
                if (!cmdConfig.filePath.isEmpty()) {
                    mainLogger.info("Opening file from command line: " +
                                    cmdConfig.filePath);
                    w.openFileFromCommandLine(cmdConfig.filePath);
                }

                // Navigate to specific page (after file is opened)
                // Add a small delay to ensure document is loaded
                if (cmdConfig.hasPage && !cmdConfig.filePath.isEmpty()) {
                    QTimer::singleShot(500, &w, [&w, cmdConfig, &mainLogger]() {
                        mainLogger.info(
                            QString("Navigating to page from command line: %1")
                                .arg(cmdConfig.pageNumber));
                        w.goToPageFromCommandLine(cmdConfig.pageNumber);
                    });
                }
            });
        }

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
