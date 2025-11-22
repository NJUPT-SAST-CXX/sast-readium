#ifdef _WIN32
#include <windows.h>
#endif

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QStandardPaths>
#include <QString>
#include <QSysInfo>
#include <QTimer>
#include <iostream>

#include "ElaApplication.h"
#include "MainWindow.h"
#include "cache/CacheManager.h"
#include "config.h"
#include "controller/AnnotationController.h"
#include "controller/ServiceLocator.h"
#include "logging/LoggingMacros.h"
#include "logging/LoggingManager.h"
#include "logging/SimpleLogging.h"
#include "managers/I18nManager.h"
#include "plugin/PluginInterface.h"
#include "plugin/PluginManager.h"
#include "utils/ResourcesInit.h"

// Command-line configuration structure (same as original app)
struct CommandLineConfig {
    QString filePath;
    bool hasGeometry = false;
    int windowX = -1;
    int windowY = -1;
    int windowWidth = 1280;
    int windowHeight = 800;
    bool maximized = false;
    bool minimized = false;
    bool fullscreen = false;
    bool hasViewMode = false;
    int viewMode = 0;
    bool hasZoom = false;
    double zoomLevel = 1.0;
    bool hasPage = false;
    int pageNumber = 1;
    bool hasTheme = false;
    QString theme = "light";
    bool hasLanguage = false;
    QString language = "system";
    bool hasLogLevel = false;
    QString logLevel = "info";
    bool hasCacheSize = false;
    qint64 cacheSize = 512 * 1024 * 1024;
};

// Enable Windows console UTF-8 and ANSI support
static void enableWindowsConsoleSupport() {
#ifdef Q_OS_WIN
    SetConsoleOutputCP(CP_UTF8);
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

// Print colored text to console
static void printColored(const QString& text, const QString& color = "") {
#ifdef Q_OS_WIN
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

// Print application logo
static void printLogo() {
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

    for (int i = 0; i < 18; ++i) {
        if (i < 3 || i > 15) {
            printColored(logoLines[i], "blue");
        } else if (i >= 3 && i <= 8) {
            printColored(logoLines[i], "cyan");
        } else {
            printColored(logoLines[i], "green");
        }
    }

    printColored("\n", "");
    printColored("                     🚀 ", "yellow");
    printColored("ElaWidgetTools Edition", "cyan");
    printColored(" • ", "yellow");
    printColored("Modern Fluent Design", "green");
    printColored(" 🚀\n", "yellow");
    printColored("\n", "");
}

namespace {

bool loggingAvailable() { return LoggingManager::instance().isInitialized(); }

void logToStderrIfLoggingUnavailable(const QString& message) {
    if (!loggingAvailable()) {
        std::cerr << message.toStdString() << '\n';
    }
}

SastLogging::Config createLoggingConfig() {
    SastLogging::Config config;
    config.level = SastLogging::Level::Info;
    config.logFile = "sast-readium-ela.log";
    config.console = true;
    config.file = true;
    config.async = false;
    config.maxFileSize = static_cast<size_t>(50) * 1024 * 1024;
    config.maxFiles = 5;
    config.pattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v";

    QString logDir;
    try {
        logDir =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (logDir.isEmpty()) {
            logDir = "./logs";
        } else {
            logDir += "/logs";
        }
    } catch (...) {
        logDir = "./logs";
    }
    config.logDir = logDir;
    return config;
}

void initializeLoggingSystem() {
    const SastLogging::Config config = createLoggingConfig();
    if (!SastLogging::init(config)) {
        logToStderrIfLoggingUnavailable(
            QStringLiteral("[ERROR] Failed to initialize logging"));
    } else {
        LOG_INFO("Logging system initialized successfully");
    }
}

bool initializeElaApplication() {
    LOG_TRACE("Initializing ElaApplication singleton...");
    try {
        eApp->init();
        LOG_DEBUG("ElaApplication initialized successfully");
        return true;
    } catch (const std::exception& e) {
        LOG_CRITICAL("ElaApplication init failed: {}", e.what());
        logToStderrIfLoggingUnavailable(
            QString("[ERROR] ElaApplication init failed: %1").arg(e.what()));
    } catch (...) {
        LOG_CRITICAL("ElaApplication init failed with unknown error");
        logToStderrIfLoggingUnavailable(QStringLiteral(
            "[ERROR] ElaApplication init failed with unknown error"));
    }
    return false;
}

void initializeQtResources() {
    LOG_TRACE("Initializing Qt resources...");
    SastResources::ensureInitialized();
    LOG_DEBUG("Qt resources initialized successfully");
}

void initializeI18nSystem() {
    LOG_TRACE("Initializing i18n system...");
    if (!I18nManager::instance().initialize()) {
        LOG_ERROR("Failed to initialize i18n system");
        logToStderrIfLoggingUnavailable(
            QStringLiteral("[ERROR] Failed to initialize i18n system"));
    } else {
        LOG_DEBUG("I18n system initialized successfully");
    }
}

void initializeCacheSystem() {
    LOG_TRACE("Initializing cache system...");
    try {
        CacheManager::instance();
        LOG_DEBUG("Cache system initialized successfully");
    } catch (const std::exception& e) {
        LOG_ERROR("Cache system init failed: {}", e.what());
        logToStderrIfLoggingUnavailable(
            QString("[ERROR] Cache system init failed: %1").arg(e.what()));
    } catch (...) {
        LOG_ERROR("Cache system init failed with unknown error");
        logToStderrIfLoggingUnavailable(QStringLiteral(
            "[ERROR] Cache system init failed with unknown error"));
    }
}

void initializePluginSystem() {
    LOG_TRACE("Initializing plugin system...");
    try {
        // Get PluginManager instance
        PluginManager& pluginManager = PluginManager::instance();

        // Register PluginManager with ServiceLocator for global access
        ServiceLocator::instance().registerService<PluginManager>(
            &pluginManager);

        // Set plugin directories
        QStringList pluginDirs;
        pluginDirs << QApplication::applicationDirPath() + "/plugins";
        pluginDirs << QStandardPaths::writableLocation(
                          QStandardPaths::AppDataLocation) +
                          "/plugins";
        pluginManager.setPluginDirectories(pluginDirs);

        // Register built-in extension points
        // Note: Menu and Toolbar extension points need MainWindow to be created
        // first They will be registered in MainWindow initialization
        LOG_DEBUG("Registering plugin extension points...");

        // Register document handler extension point (doesn't require UI)
        static DocumentHandlerExtensionPoint documentHandlerEP;
        pluginManager.registerExtensionPoint(&documentHandlerEP);

        LOG_DEBUG("Extension points registered");

        // Register standard workflow hooks
        pluginManager.registerStandardHooks();
        LOG_DEBUG("Standard hooks registered");

        // Scan for available plugins
        pluginManager.scanForPlugins();

        // Load settings (enabled/disabled state)
        pluginManager.loadSettings();

        // Load all enabled plugins
        pluginManager.loadAllPlugins();

        LOG_INFO(
            "Plugin system initialized successfully. Found {} plugins, loaded "
            "{} plugins",
            pluginManager.getAvailablePlugins().size(),
            pluginManager.getLoadedPlugins().size());

    } catch (const std::exception& e) {
        LOG_ERROR("Plugin system init failed: {}", e.what());
        logToStderrIfLoggingUnavailable(
            QString("[ERROR] Plugin system init failed: %1").arg(e.what()));
    } catch (...) {
        LOG_ERROR("Plugin system init failed with unknown error");
        logToStderrIfLoggingUnavailable(QStringLiteral(
            "[ERROR] Plugin system init failed with unknown error"));
    }
}

void initializeAnnotationSystem() {
    LOG_TRACE("Initializing annotation system...");
    try {
        // Create and register AnnotationController with ServiceLocator
        auto* annotationController = new AnnotationController();
        ServiceLocator::instance().registerService<AnnotationController>(
            annotationController);

        // Set default author from system
        QString defaultAuthor = qgetenv("USERNAME");
        if (defaultAuthor.isEmpty()) {
            defaultAuthor = qgetenv("USER");
        }
        if (defaultAuthor.isEmpty()) {
            defaultAuthor = "User";
        }
        annotationController->setDefaultAuthor(defaultAuthor);

        // Enable auto-save by default
        annotationController->setAutoSave(true);

        LOG_INFO("Annotation system initialized successfully (Author: {})",
                 defaultAuthor.toStdString());

    } catch (const std::exception& e) {
        LOG_ERROR("Annotation system init failed: {}", e.what());
        logToStderrIfLoggingUnavailable(
            QString("[ERROR] Annotation system init failed: %1").arg(e.what()));
    } catch (...) {
        LOG_ERROR("Annotation system init failed with unknown error");
        logToStderrIfLoggingUnavailable(QStringLiteral(
            "[ERROR] Annotation system init failed with unknown error"));
    }
}

int runMainWindow() {
    try {
        LOG_INFO("Creating MainWindow");
        MainWindow mainWindow;
        LOG_INFO("MainWindow created successfully");

        mainWindow.show();
        LOG_INFO("MainWindow shown successfully");

        LOG_INFO("Starting Qt event loop");
        int result = QApplication::exec();
        LOG_INFO("Application exiting with code: {}", result);

        SastLogging::flush();
        SastLogging::shutdown();
        return result;

    } catch (const std::exception& e) {
        LOG_CRITICAL("Fatal error: {}", e.what());
        logToStderrIfLoggingUnavailable(
            QString("[FATAL] Exception caught: %1").arg(e.what()));
    } catch (...) {
        LOG_CRITICAL("Unknown fatal error");
        logToStderrIfLoggingUnavailable(
            QStringLiteral("[FATAL] Unknown exception caught"));
    }

    SastLogging::flush();
    SastLogging::shutdown();
    return -1;
}

}  // namespace

int main(int argc, char** argv) {
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    enableWindowsConsoleSupport();

    QApplication app(argc, argv);

    QApplication::setApplicationName(PROJECT_NAME);
    QApplication::setApplicationVersion(PROJECT_VER);
    QApplication::setApplicationDisplayName("SAST Readium - Ela Edition");

    printLogo();

    initializeLoggingSystem();

    const auto startMessage =
        QStringLiteral("Starting SAST Readium - ElaWidgetTools Edition");
    LOG_INFO("{}", startMessage.toStdString());
    logToStderrIfLoggingUnavailable(startMessage);

    const auto versionMessage =
        QStringLiteral("Version: %1").arg(QString::fromLatin1(PROJECT_VER));
    LOG_INFO("Version: {}", PROJECT_VER);
    logToStderrIfLoggingUnavailable(versionMessage);

    const auto qtVersionMessage = QStringLiteral("Qt Version: %1")
                                      .arg(QString::fromLatin1(QT_VERSION_STR));
    LOG_INFO("Qt Version: {}", QT_VERSION_STR);
    logToStderrIfLoggingUnavailable(qtVersionMessage);

    const auto platformMessage =
        QStringLiteral("Platform: %1").arg(QSysInfo::prettyProductName());
    LOG_INFO("Platform: {}", QSysInfo::prettyProductName().toStdString());
    logToStderrIfLoggingUnavailable(platformMessage);

    if (!initializeElaApplication()) {
        SastLogging::flush();
        SastLogging::shutdown();
        return -1;
    }

    initializeQtResources();
    initializeI18nSystem();
    initializeCacheSystem();
    initializePluginSystem();
    initializeAnnotationSystem();

    return runMainWindow();
}
