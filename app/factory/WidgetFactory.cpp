#include "WidgetFactory.h"
#include "command/NavigationCommands.h"
#include "controller/PageController.h"

WidgetFactory::WidgetFactory(PageController* controller, QObject* parent)
    : QObject(parent), _controller(controller), m_logger("WidgetFactory") {
    m_logger.debug("WidgetFactory created");

    // Initialize command map
    _actionMap[actionID::next] = new NextPageCommand(_controller, this);
    _actionMap[actionID::prev] = new PreviousPageCommand(_controller, this);

    m_logger.debug(
        QString("Registered %1 action commands").arg(_actionMap.size()));
}

WidgetFactory::~WidgetFactory() {
    m_logger.debug("WidgetFactory destroyed");
    // Commands are deleted automatically via Qt parent-child ownership
}

QPushButton* WidgetFactory::createButton(actionID actionID, const QString& text,
                                         QWidget* parent) {
    try {
        m_logger.debug(QString("Creating button for action %1 with text '%2'")
                           .arg(static_cast<int>(actionID))
                           .arg(text));

        if (!_actionMap.contains(actionID)) {
            QString error = QString("Unknown action ID: %1")
                                .arg(static_cast<int>(actionID));
            m_logger.error(error);
            emit creationError("QPushButton", error);
            return nullptr;
        }

        if (!_controller) {
            QString error = "PageController not set - cannot create button";
            m_logger.error(error);
            emit creationError("QPushButton", error);
            return nullptr;
        }

        QPushButton* button = new QPushButton(text, parent);
        connect(button, &QPushButton::clicked, _actionMap[actionID],
                &NavigationCommand::execute);

        // Set object name for debugging
        QString objectName =
            QString("Button_Action%1").arg(static_cast<int>(actionID));
        button->setObjectName(objectName);

        emit widgetCreated("QPushButton", button);
        m_logger.debug(
            QString("Button created successfully: %1").arg(objectName));

        return button;

    } catch (const std::exception& e) {
        QString error = QString("Exception creating button: %1").arg(e.what());
        m_logger.error(error);
        emit creationError("QPushButton", error);
        return nullptr;
    } catch (...) {
        QString error = "Unknown exception creating button";
        m_logger.error(error);
        emit creationError("QPushButton", error);
        return nullptr;
    }
}
