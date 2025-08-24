#include "Commands.h"
#include "../controller/PageController.h"
#include "qmessagebox.h"
Command::Command(QObject* parent) : QObject(parent) {}

PrevPageCommand::PrevPageCommand(PageController* controller, QObject* parent)
    : Command(parent), _controller(controller) {}

void PrevPageCommand::execute() {
    if (_controller) {
        _controller->goToPrevPage();
    }
}

NextPageCommand::NextPageCommand(PageController* controller, QObject* parent)
    : Command(parent), _controller(controller) {}

void NextPageCommand::execute() {
    if (_controller) {
        _controller->goToNextPage();
    }
}
