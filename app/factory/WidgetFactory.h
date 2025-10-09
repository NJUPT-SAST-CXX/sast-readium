#ifndef FACTORY_WIDGETFACTORY_H
#define FACTORY_WIDGETFACTORY_H

#include <QMap>
#include <QMessageBox>
#include <QObject>
#include <QPushButton>
#include "controller/PageController.h"

class Controller;
class Command;

enum actionID { next, prev };
class WidgetFactory : public QObject {
    Q_OBJECT
public:
    WidgetFactory(PageController* controller, QObject* parent = nullptr);

    QPushButton* createButton(actionID actionID, const QString& text);

    ~WidgetFactory() {}

private:
    PageController* _controller;
    QMap<actionID, Command*> _actionMap;
};

#endif  // FACTORY_WIDGETFACTORY_H
