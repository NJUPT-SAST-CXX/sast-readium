#include "PageModel.h"
#include "RenderModel.h"
#include "qmessagebox.h"

PageModel::PageModel(int totalPages, QObject* parent)
    : QObject(parent), _totalPages(totalPages), _currentPage(1) {}

PageModel::PageModel(RenderModel* renderModel, QObject* parent):
    _renderModel(renderModel),_currentPage(0),_totalPages(renderModel->getPageCount()){

};

int PageModel::currentPage() const { return _currentPage; }

int PageModel::totalPages() const { return _totalPages; }

void PageModel::setCurrentPage(int pageNum) {
    if (pageNum >= 0 && pageNum <= _totalPages-1 && pageNum != _currentPage) {
        _currentPage = pageNum;
        _renderModel->renderPage(_currentPage);
        emit pageUpdate(_currentPage);
    }
}

void PageModel::nextPage() {
    if (_currentPage < _totalPages-1) {
        int nextPage = _currentPage + 1;
        setCurrentPage(nextPage);
    }else if(_currentPage == _totalPages-1 && _totalPages > 0){
        setCurrentPage(0);
    }
}

void PageModel::prevPage() {
    if (_currentPage > 0) {
        int prevPage = _currentPage - 1;
        setCurrentPage(prevPage);
    }else if (_currentPage == 0 && _totalPages > 0) {
        setCurrentPage(_totalPages-1);
    }
}

void PageModel::updateInfo(Poppler::Document* document) {
    _totalPages = document->numPages();
    _currentPage = 0;
}