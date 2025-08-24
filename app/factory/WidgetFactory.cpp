#include "WidgetFactory.h"
#include "../command/Commands.h"
#include "../controller/PageController.h"
#include "qmessagebox.h"

WidgetFactory::WidgetFactory(PageController* controller, QObject* parent)
    : QObject(parent), _controller(controller) {
    _actionMap[actionID::next] = new NextPageCommand(_controller, this);
    _actionMap[actionID::prev] = new PrevPageCommand(_controller, this);
}

QPushButton* WidgetFactory::createButton(actionID actionID, const QString& text){
    if(_actionMap.contains(actionID)){
        QPushButton* button = new QPushButton(text);
        connect(button, &QPushButton::clicked, _actionMap[actionID], &Command::execute);
        return button;
    }
    return nullptr;
}