#include "CommandFactory.h"
#include "../command/DocumentCommands.h"
#include "../command/NavigationCommands.h"
#include "../controller/DocumentController.h"
#include "../controller/PageController.h"
#include "../ui/core/ViewWidget.h"
#include <QWidget>

// ============================================================================
// CommandFactory Implementation
// ============================================================================

CommandFactory::CommandFactory(QObject* parent)
    : QObject(parent)
    , m_logger("CommandFactory")
{
    initializeActionMap();
    m_logger.debug("CommandFactory initialized");
}

CommandFactory::~CommandFactory() {
    m_logger.debug("CommandFactory destroyed");
}

void CommandFactory::initializeActionMap() {
    // Map string names to ActionMap enum values
    m_actionMap["openFile"] = ActionMap::openFile;
    m_actionMap["closeFile"] = ActionMap::closeFile;
    m_actionMap["saveAs"] = ActionMap::saveAs;
    m_actionMap["print"] = ActionMap::print;
    m_actionMap["reload"] = ActionMap::reload;
    m_actionMap["properties"] = ActionMap::properties;
    m_actionMap["nextPage"] = ActionMap::nextPage;
    m_actionMap["previousPage"] = ActionMap::previousPage;
    m_actionMap["firstPage"] = ActionMap::firstPage;
    m_actionMap["lastPage"] = ActionMap::lastPage;
    m_actionMap["zoomIn"] = ActionMap::zoomIn;
    m_actionMap["zoomOut"] = ActionMap::zoomOut;
    m_actionMap["fitWidth"] = ActionMap::fitWidth;
    m_actionMap["fitPage"] = ActionMap::fitPage;
}

std::unique_ptr<DocumentCommand> CommandFactory::createDocumentCommand(ActionMap action) {
    if (!validateDependencies()) {
        m_logger.error("Cannot create document command - dependencies not set");
        emit commandCreationFailed(mapActionToString(action), "Dependencies not set");
        return nullptr;
    }
    
    std::unique_ptr<DocumentCommand> command;
    
    switch (action) {
        case ActionMap::openFile:
            command = createOpenCommand();
            break;
        case ActionMap::closeFile:
            command = createCloseCommand();
            break;
        case ActionMap::saveAs:
            command = createSaveAsCommand();
            break;
        case ActionMap::print:
            command = createPrintCommand();
            break;
        case ActionMap::reload:
            command = createReloadCommand();
            break;
        case ActionMap::properties:
            command = createPropertiesCommand();
            break;
        default:
            m_logger.warning(QString("Unknown document action: %1").arg(static_cast<int>(action)));
            emit commandCreationFailed(mapActionToString(action), "Unknown action");
            return nullptr;
    }
    
    if (command) {
        emit commandCreated(mapActionToString(action), command.get());
    }
    
    return command;
}

std::unique_ptr<DocumentCommand> CommandFactory::createOpenCommand(const QString& filePath) {
    if (!m_documentController) {
        m_logger.error("DocumentController not set");
        return nullptr;
    }
    
    return DocumentCommandFactory::createOpenCommand(m_documentController, filePath);
}

std::unique_ptr<DocumentCommand> CommandFactory::createCloseCommand(int index) {
    if (!m_documentController) {
        m_logger.error("DocumentController not set");
        return nullptr;
    }
    
    return DocumentCommandFactory::createCloseCommand(m_documentController, index);
}

std::unique_ptr<DocumentCommand> CommandFactory::createSaveAsCommand(const QString& targetPath) {
    if (!m_documentController) {
        m_logger.error("DocumentController not set");
        return nullptr;
    }
    
    return DocumentCommandFactory::createSaveAsCommand(m_documentController, targetPath);
}

std::unique_ptr<DocumentCommand> CommandFactory::createPrintCommand() {
    if (!m_documentController) {
        m_logger.error("DocumentController not set");
        return nullptr;
    }
    
    return DocumentCommandFactory::createPrintCommand(m_documentController);
}

std::unique_ptr<DocumentCommand> CommandFactory::createReloadCommand() {
    if (!m_documentController) {
        m_logger.error("DocumentController not set");
        return nullptr;
    }
    
    return DocumentCommandFactory::createReloadCommand(m_documentController);
}

std::unique_ptr<DocumentCommand> CommandFactory::createPropertiesCommand() {
    if (!m_documentController) {
        m_logger.error("DocumentController not set");
        return nullptr;
    }
    
    // Properties command might not exist in DocumentCommandFactory
    // Return a basic implementation or nullptr
    m_logger.warning("Properties command not implemented");
    return nullptr;
}

std::unique_ptr<NavigationCommand> CommandFactory::createNavigationCommand(const QString& type) {
    if (!m_pageController) {
        m_logger.error("PageController not set");
        return nullptr;
    }
    
    return NavigationCommandFactory::createPageNavigationCommand(type, m_pageController);
}

std::unique_ptr<NavigationCommand> CommandFactory::createNextPageCommand() {
    if (!m_pageController) {
        m_logger.error("PageController not set");
        return nullptr;
    }
    
    return std::make_unique<NextPageCommand>(m_pageController);
}

std::unique_ptr<NavigationCommand> CommandFactory::createPreviousPageCommand() {
    if (!m_pageController) {
        m_logger.error("PageController not set");
        return nullptr;
    }
    
    return std::make_unique<PreviousPageCommand>(m_pageController);
}

std::unique_ptr<NavigationCommand> CommandFactory::createGoToPageCommand(int page) {
    if (!m_pageController) {
        m_logger.error("PageController not set");
        return nullptr;
    }
    
    return std::make_unique<GoToPageCommand>(m_pageController, page);
}

std::unique_ptr<NavigationCommand> CommandFactory::createFirstPageCommand() {
    if (!m_pageController) {
        m_logger.error("PageController not set");
        return nullptr;
    }
    
    return std::make_unique<FirstPageCommand>(m_pageController);
}

std::unique_ptr<NavigationCommand> CommandFactory::createLastPageCommand() {
    if (!m_pageController) {
        m_logger.error("PageController not set");
        return nullptr;
    }
    
    return std::make_unique<LastPageCommand>(m_pageController);
}

std::unique_ptr<NavigationCommand> CommandFactory::createZoomCommand(const QString& type) {
    if (!m_viewWidget) {
        m_logger.error("ViewWidget not set");
        return nullptr;
    }
    
    return NavigationCommandFactory::createZoomCommand(type, m_viewWidget);
}

std::unique_ptr<NavigationCommand> CommandFactory::createZoomInCommand() {
    if (!m_viewWidget) {
        m_logger.error("ViewWidget not set");
        return nullptr;
    }
    
    return std::make_unique<ZoomInCommand>(m_viewWidget);
}

std::unique_ptr<NavigationCommand> CommandFactory::createZoomOutCommand() {
    if (!m_viewWidget) {
        m_logger.error("ViewWidget not set");
        return nullptr;
    }
    
    return std::make_unique<ZoomOutCommand>(m_viewWidget);
}

std::unique_ptr<NavigationCommand> CommandFactory::createFitWidthCommand() {
    if (!m_viewWidget) {
        m_logger.error("ViewWidget not set");
        return nullptr;
    }
    
    return std::make_unique<FitWidthCommand>(m_viewWidget);
}

std::unique_ptr<NavigationCommand> CommandFactory::createFitPageCommand() {
    if (!m_viewWidget) {
        m_logger.error("ViewWidget not set");
        return nullptr;
    }
    
    return std::make_unique<FitPageCommand>(m_viewWidget);
}

std::unique_ptr<NavigationCommand> CommandFactory::createSetZoomCommand(double level) {
    if (!m_viewWidget) {
        m_logger.error("ViewWidget not set");
        return nullptr;
    }
    
    return std::make_unique<SetZoomCommand>(m_viewWidget, level);
}

std::unique_ptr<NavigationCommand> CommandFactory::createViewModeCommand(const QString& mode) {
    if (!m_viewWidget) {
        m_logger.error("ViewWidget not set");
        return nullptr;
    }
    
    return NavigationCommandFactory::createViewModeCommand(mode, m_viewWidget);
}

std::unique_ptr<NavigationCommand> CommandFactory::createRotateCommand(bool clockwise) {
    if (!m_viewWidget) {
        m_logger.error("ViewWidget not set");
        return nullptr;
    }
    
    return NavigationCommandFactory::createRotateCommand(clockwise, m_viewWidget);
}

std::unique_ptr<NavigationCommand> CommandFactory::createFullscreenCommand() {
    if (!m_viewWidget) {
        m_logger.error("ViewWidget not set");
        return nullptr;
    }
    
    return NavigationCommandFactory::createFullscreenCommand(m_viewWidget);
}

void CommandFactory::registerCommandType(const QString& typeName, CommandCreator creator) {
    m_customCreators[typeName] = creator;
    m_logger.debug(QString("Registered custom command type: %1").arg(typeName));
}

QObject* CommandFactory::createCustomCommand(const QString& typeName) {
    if (!m_customCreators.contains(typeName)) {
        m_logger.warning(QString("Unknown custom command type: %1").arg(typeName));
        emit commandCreationFailed(typeName, "Unknown type");
        return nullptr;
    }

    try {
        QObject* command = m_customCreators[typeName](this);
        if (command) {
            emit commandCreated(typeName, command);
        }
        return command;
    } catch (const std::exception& e) {
        m_logger.error(QString("Exception creating custom command %1: %2").arg(typeName).arg(e.what()));
        emit commandCreationFailed(typeName, e.what());
        return nullptr;
    }
}

QList<QObject*> CommandFactory::createCommandBatch(const QStringList& commandNames) {
    QList<QObject*> commands;

    for (const QString& name : commandNames) {
        QObject* command = createCustomCommand(name);
        if (command) {
            commands.append(command);
        } else {
            m_logger.warning(QString("Failed to create command in batch: %1").arg(name));
        }
    }

    return commands;
}

void CommandFactory::configureCommand(QObject* command, const QVariantMap& config) {
    if (!command) {
        m_logger.warning("Cannot configure null command");
        return;
    }

    // Apply configuration properties to command
    for (auto it = config.begin(); it != config.end(); ++it) {
        const QString& propertyName = it.key();
        const QVariant& value = it.value();

        if (!command->setProperty(propertyName.toUtf8().constData(), value)) {
            m_logger.warning(QString("Failed to set property %1 on command").arg(propertyName));
        }
    }
}

ActionMap CommandFactory::mapStringToAction(const QString& actionStr) const {
    return m_actionMap.value(actionStr, ActionMap::openFile);
}

QString CommandFactory::mapActionToString(ActionMap action) const {
    for (auto it = m_actionMap.begin(); it != m_actionMap.end(); ++it) {
        if (it.value() == action) {
            return it.key();
        }
    }
    return "unknown";
}

bool CommandFactory::validateDependencies() const {
    // At minimum, we need a document controller
    return m_documentController != nullptr;
}

// ============================================================================
// GlobalCommandFactory Implementation
// ============================================================================

CommandFactory& GlobalCommandFactory::instance() {
    static CommandFactory instance;
    return instance;
}

QObject* GlobalCommandFactory::createCommand(const QString& type) {
    return instance().createCustomCommand(type);
}

QObject* GlobalCommandFactory::createCommand(ActionMap action) {
    return instance().createDocumentCommand(action).release();
}

void GlobalCommandFactory::initialize(DocumentController* docController,
                                     PageController* pageController,
                                     ViewWidget* viewWidget,
                                     QWidget* mainWindow) {
    CommandFactory& factory = instance();
    factory.setDocumentController(docController);
    factory.setPageController(pageController);
    factory.setViewWidget(viewWidget);
    factory.setMainWindow(mainWindow);
}

// ============================================================================
// CommandBuilder Implementation
// ============================================================================

struct CommandBuilder::BuilderData {
    QString type;
    ActionMap action = ActionMap::openFile;
    QVariantMap parameters;
    QString shortcut;
    QString description;
    QString iconPath;
    bool undoable = false;
};

CommandBuilder::CommandBuilder()
    : m_data(std::make_unique<BuilderData>())
{}

CommandBuilder::~CommandBuilder() = default;

CommandBuilder& CommandBuilder::ofType(const QString& type) {
    m_data->type = type;
    return *this;
}

CommandBuilder& CommandBuilder::withAction(ActionMap action) {
    m_data->action = action;
    return *this;
}

CommandBuilder& CommandBuilder::withParameter(const QString& key, const QVariant& value) {
    m_data->parameters[key] = value;
    return *this;
}

CommandBuilder& CommandBuilder::withParameters(const QVariantMap& params) {
    m_data->parameters = params;
    return *this;
}

CommandBuilder& CommandBuilder::withShortcut(const QString& shortcut) {
    m_data->shortcut = shortcut;
    return *this;
}

CommandBuilder& CommandBuilder::withDescription(const QString& description) {
    m_data->description = description;
    return *this;
}

CommandBuilder& CommandBuilder::withIcon(const QString& iconPath) {
    m_data->iconPath = iconPath;
    return *this;
}

CommandBuilder& CommandBuilder::asUndoable(bool undoable) {
    m_data->undoable = undoable;
    return *this;
}

std::unique_ptr<QObject> CommandBuilder::build() {
    QObject* command = buildRaw();
    return std::unique_ptr<QObject>(command);
}

QObject* CommandBuilder::buildRaw() {
    CommandFactory& factory = GlobalCommandFactory::instance();

    QObject* command = nullptr;

    if (!m_data->type.isEmpty()) {
        command = factory.createCustomCommand(m_data->type);
    } else {
        command = factory.createDocumentCommand(m_data->action).release();
    }

    if (command) {
        factory.configureCommand(command, m_data->parameters);

        // Set additional properties
        if (!m_data->shortcut.isEmpty()) {
            command->setProperty("shortcut", m_data->shortcut);
        }
        if (!m_data->description.isEmpty()) {
            command->setProperty("description", m_data->description);
        }
        if (!m_data->iconPath.isEmpty()) {
            command->setProperty("icon", m_data->iconPath);
        }
        command->setProperty("undoable", m_data->undoable);
    }

    return command;
}

// ============================================================================
// CommandPrototypeRegistry Implementation
// ============================================================================

CommandPrototypeRegistry::CommandPrototypeRegistry(CommandFactory* factory)
    : m_factory(factory)
{}

void CommandPrototypeRegistry::registerPrototype(const QString& name, QObject* prototype) {
    if (m_prototypes.contains(name)) {
        delete m_prototypes[name];
    }
    m_prototypes[name] = prototype;
}

void CommandPrototypeRegistry::registerStandardPrototypes() {
    // Register standard command prototypes
    // This would typically create prototype instances of common commands
    // For now, this is a placeholder
}

QObject* CommandPrototypeRegistry::cloneCommand(const QString& prototypeName) {
    if (!m_prototypes.contains(prototypeName)) {
        return nullptr;
    }

    // Qt doesn't have built-in object cloning
    // This would require implementing a custom cloning mechanism
    // For now, return nullptr as a placeholder
    return nullptr;
}

QStringList CommandPrototypeRegistry::availablePrototypes() const {
    return m_prototypes.keys();
}

bool CommandPrototypeRegistry::hasPrototype(const QString& name) const {
    return m_prototypes.contains(name);
}

