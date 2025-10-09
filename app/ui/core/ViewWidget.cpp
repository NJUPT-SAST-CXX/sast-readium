#include "ViewWidget.h"
#include <QDebug>
#include <QLabel>
#include "../viewer/PDFViewer.h"

ViewWidget::ViewWidget(QWidget* parent)
    : QWidget(parent),
      documentController(nullptr),
      documentModel(nullptr),
      outlineModel(nullptr) {
    setupUI();
}

void ViewWidget::setupUI() {
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 创建标签页组件
    tabWidget = new DocumentTabWidget(this);

    // 创建堆叠组件用于显示不同的PDF查看器
    viewerStack = new QStackedWidget(this);

    // 创建空状态组件
    emptyWidget = new QWidget(this);
    QVBoxLayout* emptyLayout = new QVBoxLayout(emptyWidget);
    QLabel* emptyLabel =
        new QLabel("没有打开的PDF文档\n点击文件菜单打开PDF文档", emptyWidget);
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setStyleSheet("color: gray; font-size: 14px;");
    emptyLayout->addWidget(emptyLabel);

    viewerStack->addWidget(emptyWidget);

    mainLayout->addWidget(tabWidget);
    mainLayout->addWidget(viewerStack, 1);

    // 初始显示空状态
    showEmptyState();

    setupConnections();
}

void ViewWidget::setupConnections() {
    // 标签页信号连接
    connect(tabWidget, &DocumentTabWidget::tabCloseRequested, this,
            &ViewWidget::onTabCloseRequested);
    connect(tabWidget, &DocumentTabWidget::tabSwitched, this,
            &ViewWidget::onTabSwitched);
    connect(tabWidget, &DocumentTabWidget::tabMoved, this,
            &ViewWidget::onTabMoved);
    connect(tabWidget, &DocumentTabWidget::allTabsClosed, this,
            &ViewWidget::onAllDocumentsClosed);
}

void ViewWidget::setDocumentController(DocumentController* controller) {
    documentController = controller;
}

void ViewWidget::setDocumentModel(DocumentModel* model) {
    if (documentModel) {
        // 断开旧模型的连接
        disconnect(documentModel, nullptr, this, nullptr);
    }

    documentModel = model;

    if (documentModel) {
        // 连接新模型的信号
        connect(documentModel, &DocumentModel::documentOpened, this,
                &ViewWidget::onDocumentOpened);
        connect(documentModel, &DocumentModel::documentClosed, this,
                &ViewWidget::onDocumentClosed);
        connect(documentModel, &DocumentModel::currentDocumentChanged, this,
                &ViewWidget::onCurrentDocumentChanged);
        connect(documentModel, &DocumentModel::allDocumentsClosed, this,
                &ViewWidget::onAllDocumentsClosed);
        connect(documentModel, &DocumentModel::loadingStarted, this,
                &ViewWidget::onDocumentLoadingStarted);
        connect(documentModel, &DocumentModel::loadingProgressChanged, this,
                &ViewWidget::onDocumentLoadingProgress);
        connect(documentModel, &DocumentModel::loadingFailed, this,
                &ViewWidget::onDocumentLoadingFailed);
    }
}

void ViewWidget::setOutlineModel(PDFOutlineModel* model) {
    outlineModel = model;
}

void ViewWidget::openDocument(const QString& filePath) {
    if (documentController) {
        documentController->openDocument(filePath);
    }
}

void ViewWidget::closeDocument(int index) {
    if (documentController) {
        documentController->closeDocument(index);
    }
}

void ViewWidget::switchToDocument(int index) {
    if (documentController) {
        documentController->switchToDocument(index);
    }
}

void ViewWidget::goToPage(int pageNumber) {
    int currentIndex = getCurrentDocumentIndex();
    if (currentIndex >= 0 && currentIndex < pdfViewers.size()) {
        PDFViewer* currentViewer = pdfViewers[currentIndex];
        if (currentViewer) {
            currentViewer->goToPage(pageNumber);
        }
    }
}

void ViewWidget::setCurrentViewMode(int mode) {
    int currentIndex = getCurrentDocumentIndex();
    if (currentIndex >= 0 && currentIndex < pdfViewers.size()) {
        PDFViewer* currentViewer = pdfViewers[currentIndex];
        if (currentViewer) {
            PDFViewMode viewMode = static_cast<PDFViewMode>(mode);
            currentViewer->setViewMode(viewMode);
        }
    }
}

void ViewWidget::executePDFAction(ActionMap action) {
    int currentIndex = getCurrentDocumentIndex();
    if (currentIndex < 0 || currentIndex >= pdfViewers.size()) {
        return;  // 没有当前文档
    }

    PDFViewer* currentViewer = pdfViewers[currentIndex];
    if (!currentViewer)
        return;

    // 执行相应的PDF操作
    switch (action) {
        case ActionMap::firstPage:
            currentViewer->firstPage();
            break;
        case ActionMap::previousPage:
            currentViewer->previousPage();
            break;
        case ActionMap::nextPage:
            currentViewer->nextPage();
            break;
        case ActionMap::lastPage:
            currentViewer->lastPage();
            break;
        case ActionMap::zoomIn:
            currentViewer->zoomIn();
            break;
        case ActionMap::zoomOut:
            currentViewer->zoomOut();
            break;
        case ActionMap::fitToWidth:
            currentViewer->zoomToWidth();
            break;
        case ActionMap::fitToPage:
            currentViewer->zoomToFit();
            break;
        case ActionMap::fitToHeight:
            currentViewer->zoomToHeight();
            break;
        case ActionMap::rotateLeft:
            currentViewer->rotateLeft();
            break;
        case ActionMap::rotateRight:
            currentViewer->rotateRight();
            break;
        default:
            qWarning() << "Unhandled PDF action in ViewWidget:"
                       << static_cast<int>(action);
            break;
    }
}

bool ViewWidget::hasDocuments() const {
    return documentModel && !documentModel->isEmpty();
}

int ViewWidget::getCurrentDocumentIndex() const {
    return documentModel ? documentModel->getCurrentDocumentIndex() : -1;
}

PDFOutlineModel* ViewWidget::getCurrentOutlineModel() const {
    int currentIndex = getCurrentDocumentIndex();
    if (currentIndex >= 0 && currentIndex < outlineModels.size()) {
        return outlineModels[currentIndex];
    }
    return nullptr;
}

int ViewWidget::getCurrentPage() const {
    int currentIndex = getCurrentDocumentIndex();
    if (currentIndex >= 0 && currentIndex < pdfViewers.size()) {
        PDFViewer* viewer = pdfViewers[currentIndex];
        return viewer ? viewer->getCurrentPage() : 0;
    }
    return 0;
}

int ViewWidget::getCurrentPageCount() const {
    int currentIndex = getCurrentDocumentIndex();
    if (currentIndex >= 0 && currentIndex < pdfViewers.size()) {
        PDFViewer* viewer = pdfViewers[currentIndex];
        return viewer ? viewer->getPageCount() : 0;
    }
    return 0;
}

double ViewWidget::getCurrentZoom() const {
    int currentIndex = getCurrentDocumentIndex();
    if (currentIndex >= 0 && currentIndex < pdfViewers.size()) {
        PDFViewer* viewer = pdfViewers[currentIndex];
        return viewer ? viewer->getCurrentZoom() : 1.0;
    }
    return 1.0;
}

void ViewWidget::onDocumentOpened(int index, const QString& fileName) {
    if (!documentModel)
        return;

    QString filePath = documentModel->getDocumentFilePath(index);
    Poppler::Document* document = documentModel->getDocument(index);

    // 创建新的PDF查看器
    PDFViewer* viewer = createPDFViewer();
    viewer->setDocument(document);

    // 创建目录模型并解析目录
    PDFOutlineModel* docOutlineModel = new PDFOutlineModel(this);
    docOutlineModel->parseOutline(document);

    // 检查是否已经有加载中的占位组件需要替换
    bool hasLoadingWidget = false;
    for (int i = 0; i < tabWidget->count(); i++) {
        if (tabWidget->getTabFilePath(i) == filePath) {
            // 更新标签页文本，移除"加载中"状态
            tabWidget->setTabLoadingState(i, false);
            tabWidget->setTabText(i, fileName);

            // 替换加载组件为实际的PDF查看器
            QWidget* loadingWidget = viewerStack->widget(i + 1);
            if (loadingWidget) {
                viewerStack->removeWidget(loadingWidget);
                loadingWidget->deleteLater();
            }

            viewerStack->insertWidget(i + 1, viewer);
            hasLoadingWidget = true;
            break;
        }
    }

    // 如果没有找到加载中的组件，按原来的方式添加
    if (!hasLoadingWidget) {
        viewerStack->insertWidget(index + 1, viewer);
        tabWidget->addDocumentTab(fileName, filePath);
    }

    // 添加到查看器列表
    pdfViewers.insert(index, viewer);
    outlineModels.insert(index, docOutlineModel);

    // 切换到新文档
    hideEmptyState();
    updateCurrentViewer();

    qDebug() << "Document opened:" << fileName << "at index" << index;
}

void ViewWidget::onDocumentClosed(int index) {
    if (index < 0 || index >= pdfViewers.size())
        return;

    // 移除PDF查看器和目录模型
    removePDFViewer(index);

    if (index < outlineModels.size()) {
        PDFOutlineModel* model = outlineModels.takeAt(index);
        model->deleteLater();
    }

    // 移除标签页
    tabWidget->removeDocumentTab(index);

    // 如果没有文档了，显示空状态
    if (pdfViewers.isEmpty()) {
        showEmptyState();
    } else {
        updateCurrentViewer();
    }

    qDebug() << "Document closed at index" << index;
}

void ViewWidget::onCurrentDocumentChanged(int index) {
    tabWidget->setCurrentTab(index);
    updateCurrentViewer();

    // 更新目录模型
    if (outlineModel && index >= 0 && index < outlineModels.size()) {
        // 这里可以通知侧边栏更新目录显示
        // 实际的目录切换将在MainWindow中处理
    }

    qDebug() << "Current document changed to index" << index;
}

void ViewWidget::onAllDocumentsClosed() {
    // 清理所有PDF查看器
    for (PDFViewer* viewer : pdfViewers) {
        viewerStack->removeWidget(viewer);
        viewer->deleteLater();
    }
    pdfViewers.clear();

    showEmptyState();
    qDebug() << "All documents closed";
}

void ViewWidget::onDocumentLoadingStarted(const QString& filePath) {
    // 为正在加载的文档创建占位标签页
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.baseName();

    // 检查是否已经有这个文件的标签页
    bool tabExists = false;
    for (int i = 0; i < tabWidget->count(); i++) {
        if (tabWidget->getTabFilePath(i) == filePath) {
            // 更新现有标签页为加载状态
            tabWidget->setTabLoadingState(i, true);
            tabExists = true;
            break;
        }
    }

    // 如果没有现有标签页，创建新的
    if (!tabExists) {
        int tabIndex =
            tabWidget->addDocumentTab(fileName + " (加载中...)", filePath);

        // 创建加载中的占位组件
        QWidget* loadingWidget = createLoadingWidget(fileName);
        viewerStack->insertWidget(tabIndex + 1, loadingWidget);

        // 如果这是第一个文档，切换到加载界面
        if (pdfViewers.isEmpty()) {
            hideEmptyState();
            viewerStack->setCurrentWidget(loadingWidget);
        }
    }

    qDebug() << "Document loading started:" << fileName;
}

void ViewWidget::onDocumentLoadingProgress(int progress) {
    // 更新当前加载文档的进度
    // 这里可以更新加载界面的进度条
    qDebug() << "Loading progress:" << progress << "%";
}

void ViewWidget::onDocumentLoadingFailed(const QString& error,
                                         const QString& filePath) {
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.baseName();

    qDebug() << "Document loading failed:" << fileName << "Error:" << error;

    // 可以显示错误消息或移除失败的标签页
    // 这里暂时只输出调试信息
}

void ViewWidget::onTabCloseRequested(int index) { closeDocument(index); }

void ViewWidget::onTabSwitched(int index) { switchToDocument(index); }

void ViewWidget::onTabMoved(int from, int to) {
    // 标签页移动时，需要同步移动PDF查看器
    if (from < 0 || to < 0 || from >= pdfViewers.size() ||
        to >= pdfViewers.size()) {
        return;
    }

    // 移动PDF查看器
    PDFViewer* viewer = pdfViewers.takeAt(from);
    pdfViewers.insert(to, viewer);

    // 移动堆叠组件中的widget
    viewerStack->removeWidget(viewer);
    viewerStack->insertWidget(to + 1, viewer);  // +1 因为第0个是emptyWidget

    updateCurrentViewer();
    qDebug() << "Tab moved from" << from << "to" << to;
}

PDFViewer* ViewWidget::createPDFViewer() {
    PDFViewer* viewer = new PDFViewer(this);

    // 连接PDF查看器的信号
    connect(viewer, &PDFViewer::pageChanged, this,
            &ViewWidget::onPDFPageChanged);
    connect(viewer, &PDFViewer::zoomChanged, this,
            &ViewWidget::onPDFZoomChanged);

    return viewer;
}

QWidget* ViewWidget::createLoadingWidget(const QString& fileName) {
    QWidget* loadingWidget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(loadingWidget);
    layout->setAlignment(Qt::AlignCenter);

    // 添加加载图标或动画
    QLabel* iconLabel = new QLabel(loadingWidget);
    iconLabel->setText("⏳");  // 使用简单的emoji作为加载图标
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet("font-size: 48px; color: #666;");

    // 添加加载文本
    QLabel* textLabel =
        new QLabel(QString("正在加载 %1...").arg(fileName), loadingWidget);
    textLabel->setAlignment(Qt::AlignCenter);
    textLabel->setStyleSheet("font-size: 16px; color: #666; margin-top: 10px;");

    // 添加进度条
    QProgressBar* progressBar = new QProgressBar(loadingWidget);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setMaximumWidth(300);
    progressBar->setStyleSheet("margin-top: 10px;");

    layout->addWidget(iconLabel);
    layout->addWidget(textLabel);
    layout->addWidget(progressBar);

    return loadingWidget;
}

void ViewWidget::removePDFViewer(int index) {
    if (index < 0 || index >= pdfViewers.size())
        return;

    PDFViewer* viewer = pdfViewers.takeAt(index);
    viewerStack->removeWidget(viewer);
    viewer->deleteLater();
}

void ViewWidget::updateCurrentViewer() {
    if (!documentModel || documentModel->isEmpty()) {
        showEmptyState();
        return;
    }

    int currentIndex = documentModel->getCurrentDocumentIndex();
    if (currentIndex >= 0 && currentIndex < pdfViewers.size()) {
        viewerStack->setCurrentWidget(pdfViewers[currentIndex]);
        hideEmptyState();
    }
}

void ViewWidget::showEmptyState() {
    viewerStack->setCurrentWidget(emptyWidget);
    tabWidget->hide();
}

void ViewWidget::hideEmptyState() { tabWidget->show(); }

void ViewWidget::onPDFPageChanged(int pageNumber) {
    // 只有当前活动的PDF查看器的信号才需要处理
    PDFViewer* sender = qobject_cast<PDFViewer*>(QObject::sender());
    int currentIndex = getCurrentDocumentIndex();

    if (currentIndex >= 0 && currentIndex < pdfViewers.size() &&
        pdfViewers[currentIndex] == sender) {
        int totalPages = getCurrentPageCount();
        emit currentViewerPageChanged(pageNumber, totalPages);
    }
}

void ViewWidget::onPDFZoomChanged(double zoomFactor) {
    // 只有当前活动的PDF查看器的信号才需要处理
    PDFViewer* sender = qobject_cast<PDFViewer*>(QObject::sender());
    int currentIndex = getCurrentDocumentIndex();

    if (currentIndex >= 0 && currentIndex < pdfViewers.size() &&
        pdfViewers[currentIndex] == sender) {
        emit currentViewerZoomChanged(zoomFactor);
    }
}
