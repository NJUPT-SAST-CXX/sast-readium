#pragma once

#include <QMainWindow>
#include <memory>
#include "logging/SimpleLogging.h"

// Forward declarations
class ApplicationController;
class ViewDelegate;

/**
 * @brief MainWindow - Simplified main window using modular architecture
 *
 * This class now acts as a thin shell that delegates all functionality
 * to specialized components following SOLID principles.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() noexcept;

    // Command-line integration methods
    void openFileFromCommandLine(const QString& filePath);
    void setViewModeFromCommandLine(int mode);
    void setZoomLevelFromCommandLine(double zoom);
    void goToPageFromCommandLine(int page);

protected:
    // Override for custom close behavior
    void closeEvent(QCloseEvent* event) override;

private:
    // Core components using dependency injection
    std::unique_ptr<ApplicationController> m_applicationController;
    std::unique_ptr<ViewDelegate> m_viewDelegate;

    // Logging
    SastLogging::CategoryLogger m_logger;

private slots:
    void onInitializationCompleted();
    void onInitializationFailed(const QString& error);
    void onApplicationError(const QString& context, const QString& error);
};
