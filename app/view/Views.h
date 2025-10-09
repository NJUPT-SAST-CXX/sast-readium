#pragma once

#include <QMainWindow>
#include <QPushButton>

#include <QLabel>
#include <QMap>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

#include "delegate/PageNavigationDelegate.h"
#include "factory/WidgetFactory.h"

class Controller;
class Command;
class PageModel;

class Views : public QWidget {
    Q_OBJECT
public:
    Views(WidgetFactory* factory, PageModel* model,
          PageNavigationDelegate* delegate, QWidget* parent = nullptr);

private:
    void initUI();

    WidgetFactory* _factory;
    PageModel* _model;
    PageNavigationDelegate* _delegate;

    QLabel* _pageLabel;
};
