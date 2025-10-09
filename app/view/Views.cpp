#include "Views.h"
#include "controller/PageController.h"
#include "model/PageModel.h"

Views::Views(WidgetFactory* factory, PageModel* model,
             PageNavigationDelegate* delegate, QWidget* parent)
    : QWidget(parent), _factory(factory), _model(model), _delegate(delegate) {
    initUI();
}

void Views::initUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);

    _pageLabel =
        new QLabel("Page: " + QString::number(_model->currentPage()), this);
    _delegate = new PageNavigationDelegate(_pageLabel, this);
    connect(_model, &PageModel::pageUpdate, _delegate,
            &PageNavigationDelegate::viewUpdate);

    layout->addWidget(_pageLabel);
}
