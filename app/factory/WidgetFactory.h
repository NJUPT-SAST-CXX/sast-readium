#ifndef FACTORY_WIDGETFACTORY_H
#define FACTORY_WIDGETFACTORY_H

#include <QObject>
#include <QPushButton>
#include <QMap>
#include "../controller/PageController.h"
#include <QMessageBox>

class Controller;
class Command;

enum actionID{
    next,
    prev
};
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

#endif // FACTORY_WIDGETFACTORY_H