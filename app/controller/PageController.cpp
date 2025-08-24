#include "PageController.h"
#include "../model/PageModel.h"

PageController::PageController(PageModel* model, QObject* parent)
    : QObject(parent), _model(model) {
    }

void PageController::goToNextPage() {
    if (_model) {
        _model->nextPage();
    } else {
        // 显示警告对话框，提示用户没有加载模型
        QMessageBox::warning(nullptr, "Warning", "No model has been loaded!");
    }
}

void PageController::goToPrevPage() {
    if (_model) {
        _model->prevPage();
    } else {
        // 显示警告对话框，提示用户没有加载模型
        QMessageBox::warning(nullptr, "Warning", "No model has been loaded!");
    }
}