#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <functional>
#include <memory>
#include "../controller/tool.hpp"
#include "../logging/SimpleLogging.h"

// Forward declarations
class DocumentController;
class PageController;
class ViewWidget;
class QWidget;
class DocumentCommand;
class NavigationCommand;

/**
 * @brief CommandFactory - Factory for creating command objects
 *
 * This factory centralizes the creation of all command objects,
 * ensuring they are properly configured with their dependencies.
 * It follows the Factory Method pattern and supports registration
 * of custom command types.
 */
class CommandFactory : public QObject {
    Q_OBJECT

public:
    explicit CommandFactory(QObject* parent = nullptr);
    ~CommandFactory();

    // Set dependencies for command creation
    void setDocumentController(DocumentController* controller) {
        m_documentController = controller;
    }
    void setPageController(PageController* controller) {
        m_pageController = controller;
    }
    void setViewWidget(ViewWidget* widget) { m_viewWidget = widget; }
    void setMainWindow(QWidget* window) { m_mainWindow = window; }

    // Factory methods for document commands
    std::unique_ptr<DocumentCommand> createDocumentCommand(ActionMap action);
    std::unique_ptr<DocumentCommand> createOpenCommand(
        const QString& filePath = QString());
    std::unique_ptr<DocumentCommand> createCloseCommand(int index = -1);
    std::unique_ptr<DocumentCommand> createSaveAsCommand(
        const QString& targetPath = QString());
    std::unique_ptr<DocumentCommand> createPrintCommand();
    std::unique_ptr<DocumentCommand> createReloadCommand();
    std::unique_ptr<DocumentCommand> createPropertiesCommand();

    // Factory methods for navigation commands
    std::unique_ptr<NavigationCommand> createNavigationCommand(
        const QString& type);
    std::unique_ptr<NavigationCommand> createNextPageCommand();
    std::unique_ptr<NavigationCommand> createPreviousPageCommand();
    std::unique_ptr<NavigationCommand> createGoToPageCommand(int page);
    std::unique_ptr<NavigationCommand> createFirstPageCommand();
    std::unique_ptr<NavigationCommand> createLastPageCommand();

    // Factory methods for zoom commands
    std::unique_ptr<NavigationCommand> createZoomCommand(const QString& type);
    std::unique_ptr<NavigationCommand> createZoomInCommand();
    std::unique_ptr<NavigationCommand> createZoomOutCommand();
    std::unique_ptr<NavigationCommand> createFitWidthCommand();
    std::unique_ptr<NavigationCommand> createFitPageCommand();
    std::unique_ptr<NavigationCommand> createSetZoomCommand(double level);

    // Factory methods for view commands
    std::unique_ptr<NavigationCommand> createViewModeCommand(
        const QString& mode);
    std::unique_ptr<NavigationCommand> createRotateCommand(bool clockwise);
    std::unique_ptr<NavigationCommand> createFullscreenCommand();

    // Custom command registration
    using CommandCreator = std::function<QObject*(CommandFactory*)>;
    void registerCommandType(const QString& typeName, CommandCreator creator);
    QObject* createCustomCommand(const QString& typeName);

    // Batch command creation
    QList<QObject*> createCommandBatch(const QStringList& commandNames);

    // Command configuration
    void configureCommand(QObject* command, const QVariantMap& config);

signals:
    void commandCreated(const QString& type, QObject* command);
    void commandCreationFailed(const QString& type, const QString& error);

private:
    // Helper methods
    ActionMap mapStringToAction(const QString& actionStr) const;
    QString mapActionToString(ActionMap action) const;
    bool validateDependencies() const;

    // Dependencies
    DocumentController* m_documentController = nullptr;
    PageController* m_pageController = nullptr;
    ViewWidget* m_viewWidget = nullptr;
    QWidget* m_mainWindow = nullptr;

    // Custom command registry
    QHash<QString, CommandCreator> m_customCreators;

    // Action mapping
    QHash<QString, ActionMap> m_actionMap;

    // Logging
    SastLogging::CategoryLogger m_logger;

    // Initialize action mappings
    void initializeActionMap();
};

/**
 * @brief GlobalCommandFactory - Singleton factory for global command creation
 */
class GlobalCommandFactory {
public:
    static CommandFactory& instance();

    // Quick access methods
    static QObject* createCommand(const QString& type);
    static QObject* createCommand(ActionMap action);

    // Dependency injection
    static void initialize(DocumentController* docController,
                           PageController* pageController,
                           ViewWidget* viewWidget, QWidget* mainWindow);

private:
    GlobalCommandFactory() = default;
    ~GlobalCommandFactory() = default;
    GlobalCommandFactory(const GlobalCommandFactory&) = delete;
    GlobalCommandFactory& operator=(const GlobalCommandFactory&) = delete;
};

/**
 * @brief CommandBuilder - Builder pattern for complex command configuration
 */
class CommandBuilder {
public:
    CommandBuilder();
    ~CommandBuilder();

    // Fluent interface for configuration
    CommandBuilder& ofType(const QString& type);
    CommandBuilder& withAction(ActionMap action);
    CommandBuilder& withParameter(const QString& key, const QVariant& value);
    CommandBuilder& withParameters(const QVariantMap& params);
    CommandBuilder& withShortcut(const QString& shortcut);
    CommandBuilder& withDescription(const QString& description);
    CommandBuilder& withIcon(const QString& iconPath);
    CommandBuilder& asUndoable(bool undoable = true);

    // Build the command
    std::unique_ptr<QObject> build();
    QObject* buildRaw();

private:
    struct BuilderData;
    std::unique_ptr<BuilderData> m_data;
};

/**
 * @brief CommandPrototypeRegistry - Registry for command prototypes
 */
class CommandPrototypeRegistry {
public:
    explicit CommandPrototypeRegistry(CommandFactory* factory);

    // Register prototypes
    void registerPrototype(const QString& name, QObject* prototype);
    void registerStandardPrototypes();

    // Clone from prototype
    QObject* cloneCommand(const QString& prototypeName);

    // List available prototypes
    QStringList availablePrototypes() const;
    bool hasPrototype(const QString& name) const;

private:
    CommandFactory* m_factory;
    QHash<QString, QObject*> m_prototypes;
};
