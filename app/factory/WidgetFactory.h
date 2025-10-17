#ifndef FACTORY_WIDGETFACTORY_H
#define FACTORY_WIDGETFACTORY_H

#include <QMap>
#include <QMessageBox>
#include <QObject>
#include <QPushButton>
#include "../logging/SimpleLogging.h"
#include "controller/PageController.h"

// Forward declarations
class Controller;
class NavigationCommand;

enum actionID { next, prev };

/**
 * @brief WidgetFactory - Factory for creating UI widgets
 *
 * This factory creates and configures UI widgets with proper
 * command connections and parent ownership.
 */
class WidgetFactory : public QObject {
    Q_OBJECT
public:
    WidgetFactory(PageController* controller, QObject* parent = nullptr);
    ~WidgetFactory();

    /**
     * @brief Create a button widget connected to a navigation command
     * @param actionID The action to bind to the button
     * @param text The button text
     * @param parent Optional parent widget for memory management
     * @return Pointer to created button, or nullptr on failure
     */
    QPushButton* createButton(actionID actionID, const QString& text,
                              QWidget* parent = nullptr);

signals:
    void widgetCreated(const QString& widgetType, QWidget* widget);
    void creationError(const QString& widgetType, const QString& error);

private:
    PageController* _controller;
    QMap<actionID, NavigationCommand*> _actionMap;
    SastLogging::CategoryLogger m_logger;
};

#endif  // FACTORY_WIDGETFACTORY_H
