#include "WidgetFactory.h"
#include "command/NavigationCommands.h"
#include "controller/PageController.h"

WidgetFactory::WidgetFactory(PageController* controller, QObject* parent)
    : QObject(parent), _controller(controller) {
    _actionMap[actionID::next] = new NextPageCommand(_controller, this);
    _actionMap[actionID::prev] = new PreviousPageCommand(_controller, this);
}

QPushButton* WidgetFactory::createButton(actionID actionID,
                                         const QString& text) {
    if (_actionMap.contains(actionID)) {
        QPushButton* button = new QPushButton(text);
        connect(button, &QPushButton::clicked, _actionMap[actionID],
                &NavigationCommand::execute);
        return button;
    }
    return nullptr;
}