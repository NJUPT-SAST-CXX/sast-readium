#include "CommandFactory.h"
#include <QWidget>
#include "../command/DocumentCommands.h"
#include "../command/NavigationCommands.h"
#include "../controller/DocumentController.h"
#include "../controller/PageController.h"
#include "../ui/core/ViewWidget.h"

// ============================================================================
// CommandFactory Implementation
// ============================================================================

CommandFactory::CommandFactory(QObject* parent)
    : QObject(parent), m_logger("CommandFactory") {
    initializeActionMap();
    m_logger.debug("CommandFactory initialized");
}

CommandFactory::~CommandFactory() {
    m_logger.debug("CommandFactory destroyed");
}

void CommandFactory::initializeActionMap() {
    // Map string names to ActionMap enum values
    // Document operations
    m_actionMap["openFile"] = ActionMap::openFile;
    m_actionMap["openFolder"] = ActionMap::openFolder;
    m_actionMap["save"] = ActionMap::save;
    m_actionMap["saveAs"] = ActionMap::saveAs;
    m_actionMap["saveFile"] = ActionMap::saveFile;
    m_actionMap["closeFile"] = ActionMap::closeFile;
    m_actionMap["exportFile"] = ActionMap::exportFile;
    m_actionMap["printFile"] = ActionMap::printFile;
    m_actionMap["reloadFile"] = ActionMap::reloadFile;
    m_actionMap["showDocumentMetadata"] = ActionMap::showDocumentMetadata;

    // Tab operations
    m_actionMap["newTab"] = ActionMap::newTab;
    m_actionMap["closeTab"] = ActionMap::closeTab;
    m_actionMap["closeCurrentTab"] = ActionMap::closeCurrentTab;
    m_actionMap["closeAllTabs"] = ActionMap::closeAllTabs;
    m_actionMap["nextTab"] = ActionMap::nextTab;
    m_actionMap["prevTab"] = ActionMap::prevTab;
    m_actionMap["switchToTab"] = ActionMap::switchToTab;

    // Sidebar operations
    m_actionMap["toggleSideBar"] = ActionMap::toggleSideBar;
    m_actionMap["showSideBar"] = ActionMap::showSideBar;
    m_actionMap["hideSideBar"] = ActionMap::hideSideBar;

    // View mode operations
    m_actionMap["setSinglePageMode"] = ActionMap::setSinglePageMode;
    m_actionMap["setContinuousScrollMode"] = ActionMap::setContinuousScrollMode;

    // Page navigation operations
    m_actionMap["firstPage"] = ActionMap::firstPage;
    m_actionMap["previousPage"] = ActionMap::previousPage;
    m_actionMap["nextPage"] = ActionMap::nextPage;
    m_actionMap["lastPage"] = ActionMap::lastPage;
    m_actionMap["goToPage"] = ActionMap::goToPage;

    // Zoom operations
    m_actionMap["zoomIn"] = ActionMap::zoomIn;
    m_actionMap["zoomOut"] = ActionMap::zoomOut;
    m_actionMap["fitToWidth"] = ActionMap::fitToWidth;
    m_actionMap["fitToPage"] = ActionMap::fitToPage;
    m_actionMap["fitToHeight"] = ActionMap::fitToHeight;

    // Rotation operations
    m_actionMap["rotateLeft"] = ActionMap::rotateLeft;
    m_actionMap["rotateRight"] = ActionMap::rotateRight;

    // Theme operations
    m_actionMap["toggleTheme"] = ActionMap::toggleTheme;

    // Search operations
    m_actionMap["showSearch"] = ActionMap::showSearch;
    m_actionMap["hideSearch"] = ActionMap::hideSearch;
    m_actionMap["toggleSearch"] = ActionMap::toggleSearch;
    m_actionMap["findNext"] = ActionMap::findNext;
    m_actionMap["findPrevious"] = ActionMap::findPrevious;
    m_actionMap["clearSearch"] = ActionMap::clearSearch;

    // Recent files operations
    m_actionMap["openRecentFile"] = ActionMap::openRecentFile;
    m_actionMap["clearRecentFiles"] = ActionMap::clearRecentFiles;

    // Fullscreen operation
    m_actionMap["fullScreen"] = ActionMap::fullScreen;

    m_logger.debug(QString("Initialized action map with %1 entries")
                       .arg(m_actionMap.size()));
}

std::unique_ptr<DocumentCommand> CommandFactory::createDocumentCommand(
    ActionMap action) {
    if (!validateDependencies()) {
        m_logger.error("Cannot create document command - dependencies not set");
        emit commandCreationFailed(mapActionToString(action),
                                   "Dependencies not set");
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
        case ActionMap::printFile:
            command = createPrintCommand();
            break;
        case ActionMap::reloadFile:
            command = createReloadCommand();
            break;
        case ActionMap::showDocumentMetadata:
            command = createPropertiesCommand();
            break;
        default:
            m_logger.warning(QString("Unknown document action: %1")
                                 .arg(static_cast<int>(action)));
            emit commandCreationFailed(mapActionToString(action),
                                       "Unknown action");
            return nullptr;
    }

    if (command) {
        emit commandCreated(mapActionToString(action), command.get());
    }

    return command;
}

std::unique_ptr<DocumentCommand> CommandFactory::createOpenCommand(
    const QString& filePath) {
    if (!m_documentController) {
        m_logger.error("DocumentController not set");
        return nullptr;
    }

    return DocumentCommandFactory::createOpenCommand(m_documentController,
                                                     filePath);
}

std::unique_ptr<DocumentCommand> CommandFactory::createCloseCommand(int index) {
    if (!m_documentController) {
        m_logger.error("DocumentController not set");
        return nullptr;
    }

    return DocumentCommandFactory::createCloseCommand(m_documentController,
                                                      index);
}

std::unique_ptr<DocumentCommand> CommandFactory::createSaveAsCommand(
    const QString& targetPath) {
    if (!m_documentController) {
        m_logger.error("DocumentController not set");
        return nullptr;
    }

    return DocumentCommandFactory::createSaveAsCommand(m_documentController,
                                                       targetPath);
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

    // Get the main window widget for dialog parent
    QWidget* parentWidget = nullptr;
    if (m_mainWindow) {
        parentWidget = m_mainWindow;
    }

    return std::make_unique<ShowDocumentPropertiesCommand>(m_documentController,
                                                           parentWidget);
}

std::unique_ptr<NavigationCommand> CommandFactory::createNavigationCommand(
    const QString& type) {
    if (!m_pageController) {
        m_logger.error("PageController not set");
        return nullptr;
    }

    return NavigationCommandFactory::createPageNavigationCommand(
        type, m_pageController);
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

std::unique_ptr<NavigationCommand> CommandFactory::createGoToPageCommand(
    int page) {
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

std::unique_ptr<NavigationCommand> CommandFactory::createZoomCommand(
    const QString& type) {
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

std::unique_ptr<NavigationCommand> CommandFactory::createSetZoomCommand(
    double level) {
    if (!m_viewWidget) {
        m_logger.error("ViewWidget not set");
        return nullptr;
    }

    return std::make_unique<SetZoomCommand>(m_viewWidget, level);
}

std::unique_ptr<NavigationCommand> CommandFactory::createViewModeCommand(
    const QString& mode) {
    if (!m_viewWidget) {
        m_logger.error("ViewWidget not set");
        return nullptr;
    }

    // Map mode string to ChangeViewModeCommand::ViewMode
    if (mode == "single-page") {
        return std::make_unique<ChangeViewModeCommand>(
            m_viewWidget, ChangeViewModeCommand::ViewMode::SinglePage);
    }
    if (mode == "continuous") {
        return std::make_unique<ChangeViewModeCommand>(
            m_viewWidget, ChangeViewModeCommand::ViewMode::Continuous);
    }
    if (mode == "facing-pages") {
        return std::make_unique<ChangeViewModeCommand>(
            m_viewWidget, ChangeViewModeCommand::ViewMode::FacingPages);
    } else if (mode == "book-view") {
        return std::make_unique<ChangeViewModeCommand>(
            m_viewWidget, ChangeViewModeCommand::ViewMode::BookView);
    }

    m_logger.warning(QString("Unknown view mode: %1").arg(mode));
    return nullptr;
}

std::unique_ptr<NavigationCommand> CommandFactory::createRotateCommand(
    bool clockwise) {
    if (!m_viewWidget) {
        m_logger.error("ViewWidget not set");
        return nullptr;
    }

    auto direction =
        clockwise ? RotateViewCommand::RotationDirection::Clockwise
                  : RotateViewCommand::RotationDirection::CounterClockwise;
    return std::make_unique<RotateViewCommand>(m_viewWidget, direction);
}

std::unique_ptr<NavigationCommand> CommandFactory::createFullscreenCommand() {
    if (!m_viewWidget) {
        m_logger.error("ViewWidget not set");
        return nullptr;
    }

    // ToggleFullscreenCommand needs a QWidget (main window), not ViewWidget
    // For now, use the viewWidget's parent window
    QWidget* mainWindow = m_viewWidget->window();
    if (!mainWindow) {
        m_logger.error("Cannot get main window from ViewWidget");
        return nullptr;
    }

    return std::make_unique<ToggleFullscreenCommand>(mainWindow);
}

void CommandFactory::registerCommandType(const QString& typeName,
                                         CommandCreator creator) {
    m_customCreators[typeName] = creator;
    m_logger.debug(QString("Registered custom command type: %1").arg(typeName));
}

QObject* CommandFactory::createCustomCommand(const QString& typeName) {
    if (!m_customCreators.contains(typeName)) {
        m_logger.warning(
            QString("Unknown custom command type: %1").arg(typeName));
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
        m_logger.error(QString("Exception creating custom command %1: %2")
                           .arg(typeName)
                           .arg(e.what()));
        emit commandCreationFailed(typeName, e.what());
        return nullptr;
    }
}

QList<QObject*> CommandFactory::createCommandBatch(
    const QStringList& commandNames) {
    QList<QObject*> commands;

    for (const QString& name : commandNames) {
        QObject* command = createCustomCommand(name);
        if (command) {
            commands.append(command);
        } else {
            m_logger.warning(
                QString("Failed to create command in batch: %1").arg(name));
        }
    }

    return commands;
}

void CommandFactory::configureCommand(QObject* command,
                                      const QVariantMap& config) {
    if (!command) {
        m_logger.warning("Cannot configure null command");
        return;
    }

    // Apply configuration properties to command
    for (auto it = config.begin(); it != config.end(); ++it) {
        const QString& propertyName = it.key();
        const QVariant& value = it.value();

        if (!command->setProperty(propertyName.toUtf8().constData(), value)) {
            m_logger.warning(QString("Failed to set property %1 on command")
                                 .arg(propertyName));
        }
    }
}

ActionMap CommandFactory::mapStringToAction(const QString& actionStr) {
    if (!m_actionMap.contains(actionStr)) {
        m_logger.warning(
            QString("Unknown action string '%1' - no mapping available")
                .arg(actionStr));
        // Return a safe default but log the issue
        return ActionMap::openFile;
    }
    return m_actionMap.value(actionStr);
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

CommandBuilder::CommandBuilder() : m_data(std::make_unique<BuilderData>()) {}

CommandBuilder::~CommandBuilder() = default;

CommandBuilder& CommandBuilder::ofType(const QString& type) {
    m_data->type = type;
    return *this;
}

CommandBuilder& CommandBuilder::withAction(ActionMap action) {
    m_data->action = action;
    return *this;
}

CommandBuilder& CommandBuilder::withParameter(const QString& key,
                                              const QVariant& value) {
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
    : m_factory(factory) {}

CommandPrototypeRegistry::~CommandPrototypeRegistry() {
    // Delete all registered prototypes to prevent memory leaks
    qDeleteAll(m_prototypes);
    m_prototypes.clear();
}

void CommandPrototypeRegistry::registerPrototype(const QString& name,
                                                 QObject* prototype) {
    if (m_prototypes.contains(name)) {
        delete m_prototypes[name];
    }
    m_prototypes[name] = prototype;
}

void CommandPrototypeRegistry::registerStandardPrototypes() {
    if (!m_factory) {
        return;
    }

    // Register standard document command prototypes
    // Note: These are prototype instances that can be cloned for creating new
    // commands The actual cloning mechanism would need to be implemented based
    // on specific requirements

    // Document commands
    registerPrototype("open-document",
                      m_factory->createOpenCommand().release());
    registerPrototype("close-document",
                      m_factory->createCloseCommand().release());
    registerPrototype("save-as", m_factory->createSaveAsCommand().release());
    registerPrototype("print", m_factory->createPrintCommand().release());
    registerPrototype("reload", m_factory->createReloadCommand().release());
    registerPrototype("properties",
                      m_factory->createPropertiesCommand().release());

    // Navigation commands
    registerPrototype("next-page",
                      m_factory->createNextPageCommand().release());
    registerPrototype("previous-page",
                      m_factory->createPreviousPageCommand().release());
    registerPrototype("first-page",
                      m_factory->createFirstPageCommand().release());
    registerPrototype("last-page",
                      m_factory->createLastPageCommand().release());
    registerPrototype("goto-page",
                      m_factory->createGoToPageCommand(1).release());

    // Zoom commands
    registerPrototype("zoom-in", m_factory->createZoomInCommand().release());
    registerPrototype("zoom-out", m_factory->createZoomOutCommand().release());
    registerPrototype("zoom-fit-width",
                      m_factory->createFitWidthCommand().release());
    registerPrototype("zoom-fit-page",
                      m_factory->createFitPageCommand().release());
    registerPrototype("zoom-100",
                      m_factory->createSetZoomCommand(1.0).release());

    // Rotation commands
    registerPrototype("rotate-clockwise",
                      m_factory->createRotateCommand(true).release());
    registerPrototype("rotate-counterclockwise",
                      m_factory->createRotateCommand(false).release());

    // View mode commands
    registerPrototype("fullscreen",
                      m_factory->createFullscreenCommand().release());
}

QObject* CommandPrototypeRegistry::cloneCommand(const QString& prototypeName) {
    if (!m_prototypes.contains(prototypeName) || !m_factory) {
        return nullptr;
    }

    // PSEUDO-PROTOTYPE PATTERN IMPLEMENTATION:
    // This is not a true prototype pattern with clone() methods.
    // Since Qt command objects don't implement a clone() interface,
    // we store prototype instances for reference/registration purposes,
    // but create new instances via the factory when "cloning".
    //
    // The stored prototypes serve as a registry of available command types,
    // but the actual "cloning" is just factory recreation based on the name.
    // This approach avoids the complexity of implementing clone() methods
    // on all command classes while still providing a prototype-like API.

    // Document commands
    if (prototypeName == "open-document") {
        return m_factory->createOpenCommand().release();
    }
    if (prototypeName == "close-document") {
        return m_factory->createCloseCommand().release();
    }
    if (prototypeName == "save-as") {
        return m_factory->createSaveAsCommand().release();
    } else if (prototypeName == "print") {
        return m_factory->createPrintCommand().release();
    } else if (prototypeName == "reload") {
        return m_factory->createReloadCommand().release();
    } else if (prototypeName == "properties") {
        return m_factory->createPropertiesCommand().release();
    }
    // Navigation commands
    else if (prototypeName == "next-page") {
        return m_factory->createNextPageCommand().release();
    } else if (prototypeName == "previous-page") {
        return m_factory->createPreviousPageCommand().release();
    } else if (prototypeName == "first-page") {
        return m_factory->createFirstPageCommand().release();
    } else if (prototypeName == "last-page") {
        return m_factory->createLastPageCommand().release();
    } else if (prototypeName == "goto-page") {
        return m_factory->createGoToPageCommand(1).release();
    }
    // Zoom commands
    else if (prototypeName == "zoom-in") {
        return m_factory->createZoomInCommand().release();
    } else if (prototypeName == "zoom-out") {
        return m_factory->createZoomOutCommand().release();
    } else if (prototypeName == "zoom-fit-width") {
        return m_factory->createFitWidthCommand().release();
    } else if (prototypeName == "zoom-fit-page") {
        return m_factory->createFitPageCommand().release();
    } else if (prototypeName == "zoom-100") {
        return m_factory->createSetZoomCommand(1.0).release();
    }
    // Rotation commands
    else if (prototypeName == "rotate-clockwise") {
        return m_factory->createRotateCommand(true).release();
    } else if (prototypeName == "rotate-counterclockwise") {
        return m_factory->createRotateCommand(false).release();
    }
    // View mode commands
    else if (prototypeName == "fullscreen") {
        return m_factory->createFullscreenCommand().release();
    }

    return nullptr;
}

QStringList CommandPrototypeRegistry::availablePrototypes() const {
    return m_prototypes.keys();
}

bool CommandPrototypeRegistry::hasPrototype(const QString& name) const {
    return m_prototypes.contains(name);
}
