#include "PageModel.h"
#include "RenderModel.h"

PageModel::PageModel(int totalPages, QObject* parent)
    : QObject(parent),
      _totalPages(totalPages),
      _currentPage(1) {}  // 页数从1开始计数

PageModel::PageModel(RenderModel* renderModel, QObject* parent)
    : _renderModel(renderModel),
      _currentPage(1),
      _totalPages(renderModel->getPageCount()) {}

int PageModel::currentPage() const { return _currentPage; }

int PageModel::totalPages() const { return _totalPages; }

void PageModel::setCurrentPage(int pageNum) {
    if (pageNum >= 1 && pageNum <= _totalPages && pageNum != _currentPage) {
        _currentPage = pageNum;
        _renderModel->renderPage(_currentPage - 1);  // 调用renderPage时页数减1
        emit pageUpdate(_currentPage, _totalPages);
    }
}

void PageModel::nextPage() {
    if (_currentPage < _totalPages) {
        int nextPage = _currentPage + 1;
        setCurrentPage(nextPage);
    } else if (_currentPage == _totalPages && _totalPages > 0) {
        setCurrentPage(1);
    }
}

void PageModel::prevPage() {
    if (_currentPage > 1) {
        int prevPage = _currentPage - 1;
        setCurrentPage(prevPage);
    } else if (_currentPage == 1 && _totalPages > 0) {
        setCurrentPage(_totalPages);
    }
}

void PageModel::updateInfo(Poppler::Document* document) {
    _totalPages = document->numPages();
    _currentPage = 1;
    if (_renderModel && _totalPages > 0) {
        // 文档加载后，自动渲染首页
        _renderModel->renderPage(
            _currentPage -
            1);  // poppler::document从0开始计数，但为方便page从1开始计数，此处需要-1
    }
}
