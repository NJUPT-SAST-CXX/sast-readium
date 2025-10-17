#pragma once

#include <QObject>
#include <QString>
#include <memory>
#include <cstdint>
#include "../logging/SimpleLogging.h"

// Forward declarations
class PageController;
class ViewWidget;
class DocumentModel;

/**
 * @brief Base class for navigation-related commands
 *
 * Provides common functionality for all navigation operations
 * including page navigation, zooming, and view control.
 */
class NavigationCommand : public QObject {
    Q_OBJECT

public:
    explicit NavigationCommand(const QString& name, QObject* parent = nullptr);
    ~NavigationCommand() override = default;

    NavigationCommand(const NavigationCommand&) = delete;
    NavigationCommand& operator=(const NavigationCommand&) = delete;
    NavigationCommand(NavigationCommand&&) = delete;
    NavigationCommand& operator=(NavigationCommand&&) = delete;

    // Command interface
    virtual bool execute() = 0;
    [[nodiscard]] virtual bool canExecute() const { return true; }
    virtual bool undo() { return false; }

    // Command metadata
    [[nodiscard]] QString name() const { return m_name; }
    [[nodiscard]] QString description() const { return m_description; }

    // Keyboard shortcut support
    void setShortcut(const QString& shortcut) { m_shortcut = shortcut; }
    [[nodiscard]] QString shortcut() const { return m_shortcut; }

signals:
    void executed(bool success);
    void navigationChanged(int page);
    void zoomChanged(double zoom);
    void viewModeChanged(const QString& mode);

protected:
    void setDescription(const QString& desc) { m_description = desc; }

private:
    QString m_name;
    QString m_description;
    QString m_shortcut;

protected:
    SastLogging::CategoryLogger m_logger{"NavigationCommand"};
};

/**
 * @brief Command to navigate to next page
 */
class NextPageCommand : public NavigationCommand {
    Q_OBJECT

public:
    explicit NextPageCommand(PageController* controller,
                             QObject* parent = nullptr);

    bool execute() override;
    [[nodiscard]] bool canExecute() const override;
    bool undo() override;

private:
    PageController* m_controller;
    int m_previousPage = -1;
};

/**
 * @brief Command to navigate to previous page
 */
class PreviousPageCommand : public NavigationCommand {
    Q_OBJECT

public:
    explicit PreviousPageCommand(PageController* controller,
                                 QObject* parent = nullptr);

    bool execute() override;
    [[nodiscard]] bool canExecute() const override;
    bool undo() override;

private:
    PageController* m_controller;
    int m_previousPage = -1;
};

/**
 * @brief Command to navigate to specific page
 */
class GoToPageCommand : public NavigationCommand {
    Q_OBJECT

public:
    explicit GoToPageCommand(PageController* controller, int targetPage = 1,
                             QObject* parent = nullptr);

    void setTargetPage(int page) { m_targetPage = page; }
    [[nodiscard]] int targetPage() const { return m_targetPage; }

    bool execute() override;
    [[nodiscard]] bool canExecute() const override;
    bool undo() override;

private:
    PageController* m_controller;
    int m_targetPage;
    int m_previousPage = -1;
};

/**
 * @brief Command to navigate to first page
 */
class FirstPageCommand : public NavigationCommand {
    Q_OBJECT

public:
    explicit FirstPageCommand(PageController* controller,
                              QObject* parent = nullptr);

    bool execute() override;
    [[nodiscard]] bool canExecute() const override;
    bool undo() override;

private:
    PageController* m_controller;
    int m_previousPage = -1;
};

/**
 * @brief Command to navigate to last page
 */
class LastPageCommand : public NavigationCommand {
    Q_OBJECT

public:
    explicit LastPageCommand(PageController* controller,
                             QObject* parent = nullptr);

    bool execute() override;
    [[nodiscard]] bool canExecute() const override;
    bool undo() override;

private:
    PageController* m_controller;
    int m_previousPage = -1;
};

/**
 * @brief Command to zoom in
 */
class ZoomInCommand : public NavigationCommand {
    Q_OBJECT

public:
    explicit ZoomInCommand(ViewWidget* viewWidget, double factor = 1.25,
                           QObject* parent = nullptr);

    void setZoomFactor(double factor) { m_factor = factor; }
    [[nodiscard]] double zoomFactor() const { return m_factor; }

    bool execute() override;
    [[nodiscard]] bool canExecute() const override;
    bool undo() override;

private:
    ViewWidget* m_viewWidget;
    double m_factor;
    double m_previousZoom = 1.0;
};

/**
 * @brief Command to zoom out
 */
class ZoomOutCommand : public NavigationCommand {
    Q_OBJECT

public:
    explicit ZoomOutCommand(ViewWidget* viewWidget, double factor = 0.8,
                            QObject* parent = nullptr);

    void setZoomFactor(double factor) { m_factor = factor; }
    [[nodiscard]] double zoomFactor() const { return m_factor; }

    bool execute() override;
    [[nodiscard]] bool canExecute() const override;
    bool undo() override;

private:
    ViewWidget* m_viewWidget;
    double m_factor;
    double m_previousZoom = 1.0;
};

/**
 * @brief Command to set specific zoom level
 */
class SetZoomCommand : public NavigationCommand {
    Q_OBJECT

public:
    explicit SetZoomCommand(ViewWidget* viewWidget, double zoomLevel = 1.0,
                            QObject* parent = nullptr);

    void setZoomLevel(double level) { m_zoomLevel = level; }
    [[nodiscard]] double zoomLevel() const { return m_zoomLevel; }

    bool execute() override;
    [[nodiscard]] bool canExecute() const override;
    bool undo() override;

private:
    ViewWidget* m_viewWidget;
    double m_zoomLevel;
    double m_previousZoom = 1.0;
};

/**
 * @brief Command to fit page to window width
 */
class FitWidthCommand : public NavigationCommand {
    Q_OBJECT

public:
    explicit FitWidthCommand(ViewWidget* viewWidget, QObject* parent = nullptr);

    bool execute() override;
    [[nodiscard]] bool canExecute() const override;
    bool undo() override;

private:
    ViewWidget* m_viewWidget;
    double m_previousZoom = 1.0;
};

/**
 * @brief Command to fit entire page in window
 */
class FitPageCommand : public NavigationCommand {
    Q_OBJECT

public:
    explicit FitPageCommand(ViewWidget* viewWidget, QObject* parent = nullptr);

    bool execute() override;
    [[nodiscard]] bool canExecute() const override;
    bool undo() override;

private:
    ViewWidget* m_viewWidget;
    double m_previousZoom = 1.0;
};

/**
 * @brief Command to rotate view
 */
class RotateViewCommand : public NavigationCommand {
    Q_OBJECT

public:
    enum class RotationDirection : std::uint8_t { Clockwise, CounterClockwise };

    explicit RotateViewCommand(ViewWidget* viewWidget,
                               RotationDirection direction = RotationDirection::Clockwise,
                               int degrees = 90, QObject* parent = nullptr);

    void setDirection(RotationDirection dir) { m_direction = dir; }
    void setDegrees(int degrees) { m_degrees = degrees; }

    bool execute() override;
    [[nodiscard]] bool canExecute() const override;
    bool undo() override;

private:
    ViewWidget* m_viewWidget;
    RotationDirection m_direction;
    int m_degrees;
    int m_previousRotation = 0;
};

/**
 * @brief Command to toggle fullscreen mode
 */
class ToggleFullscreenCommand : public NavigationCommand {
    Q_OBJECT

public:
    explicit ToggleFullscreenCommand(QWidget* mainWindow,
                                     QObject* parent = nullptr);

    bool execute() override;
    [[nodiscard]] bool canExecute() const override;
    bool undo() override;

private:
    QWidget* m_mainWindow;
    bool m_wasFullscreen = false;
};

/**
 * @brief Command to change view mode (single page, continuous, etc.)
 */
class ChangeViewModeCommand : public NavigationCommand {
    Q_OBJECT

public:
    enum class ViewMode : std::uint8_t { SinglePage, Continuous, FacingPages, BookView };

    explicit ChangeViewModeCommand(ViewWidget* viewWidget, ViewMode mode,
                                   QObject* parent = nullptr);

    void setViewMode(ViewMode mode) { m_mode = mode; }
    [[nodiscard]] ViewMode viewMode() const { return m_mode; }

    bool execute() override;
    [[nodiscard]] bool canExecute() const override;
    bool undo() override;

private:
    ViewWidget* m_viewWidget;
    ViewMode m_mode;
    ViewMode m_previousMode = ViewMode::SinglePage;
};

/**
 * @brief Command to scroll to specific position
 */
class ScrollToPositionCommand : public NavigationCommand {
    Q_OBJECT

public:
    enum class ScrollDirection : std::uint8_t { Top, Bottom, Left, Right };

    explicit ScrollToPositionCommand(ViewWidget* viewWidget,
                                     ScrollDirection direction,
                                     QObject* parent = nullptr);

    void setDirection(ScrollDirection dir) { m_direction = dir; }
    void setPosition(int xpos, int ypos) {
        m_x = xpos;
        m_y = ypos;
    }

    bool execute() override;
    [[nodiscard]] bool canExecute() const override;
    bool undo() override;

private:
    ViewWidget* m_viewWidget;
    ScrollDirection m_direction;
    int m_x = 0;
    int m_y = 0;
    QPoint m_previousPosition;
};

/**
 * @brief Command factory for creating navigation commands
 */
class NavigationCommandFactory {
public:
    static std::unique_ptr<NavigationCommand> createPageNavigationCommand(
        const QString& type, PageController* controller);

    static std::unique_ptr<NavigationCommand> createZoomCommand(
        const QString& type, ViewWidget* viewWidget);

    static std::unique_ptr<NavigationCommand> createViewCommand(
        const QString& type, ViewWidget* viewWidget);

    static void registerShortcuts(QWidget* widget);
};
