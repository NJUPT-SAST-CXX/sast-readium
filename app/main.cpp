#include <config.h>
#include <QApplication>
#include "MainWindow.h"
#include "utils/LoggingConfig.h"
#include "utils/LoggingMacros.h"
#include "utils/LoggingManager.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    // Initialize logging system early
    LoggingConfigBuilder configBuilder;
    configBuilder.useDevelopmentPreset()
        .setGlobalLevel(Logger::LogLevel::Debug)
        .addConsoleSink("console", Logger::LogLevel::Debug)
        .addCategory("main", Logger::LogLevel::Debug)
        .addCategory("ui", Logger::LogLevel::Info);

    auto loggingConfig = configBuilder.buildUnique();
    LoggingManager::instance().initialize(
        LoggingManager::LoggingConfiguration{});

    LOG_INFO("Starting SAST Readium application");
    LOG_INFO("Application name: {}", PROJECT_NAME);
    LOG_INFO("Application version: {}", PROJECT_VER);
    LOG_INFO("Qt version: {}", QT_VERSION_STR);

    app.setStyle("fusion");
    LOG_DEBUG("Set application style to 'fusion'");

    app.setApplicationName(PROJECT_NAME);
    app.setApplicationVersion(PROJECT_VER);
    app.setApplicationDisplayName(APP_NAME);

    LOG_DEBUG("Application metadata configured");

    try {
        MainWindow w;
        w.show();
        LOG_INFO("Main window created and shown successfully");

        int result = QApplication::exec();
        LOG_INFO("Application exiting with code: {}", result);

        // Shutdown logging system
        LoggingManager::instance().shutdown();

        return result;

    } catch (const std::exception& e) {
        LOG_CRITICAL("Fatal error during application startup: {}", e.what());
        LoggingManager::instance().shutdown();
        return -1;
    } catch (...) {
        LOG_CRITICAL("Unknown fatal error during application startup");
        LoggingManager::instance().shutdown();
        return -1;
    }
}
