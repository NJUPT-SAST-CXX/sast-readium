#include "ThumbnailContextMenu.h"
#include <poppler-qt6.h>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QStandardPaths>
#include <stdexcept>
#include "../../model/ThumbnailModel.h"

// 常量定义
const QString ThumbnailContextMenu::DEFAULT_EXPORT_FORMAT = "PNG";
const QStringList ThumbnailContextMenu::SUPPORTED_EXPORT_FORMATS = {
    "PNG", "JPEG", "PDF", "SVG"};

ThumbnailContextMenu::ThumbnailContextMenu(QWidget* parent)
    : QMenu(parent),
      m_thumbnailModel(nullptr),
      m_currentPage(-1),
      m_isDarkTheme(false) {
    setObjectName("ThumbnailContextMenu");

    m_clipboard = QApplication::clipboard();

    createActions();
    setupMenu();
    updateMenuStyle();
}

ThumbnailContextMenu::~ThumbnailContextMenu() = default;

void ThumbnailContextMenu::createActions() {
    // 跳转到页面
    m_goToPageAction = new QAction("跳转到此页", this);
    m_goToPageAction->setShortcut(QKeySequence(Qt::Key_Return));
    connect(m_goToPageAction, &QAction::triggered, this,
            &ThumbnailContextMenu::onGoToPage);

    // 复制页面
    m_copyPageAction = new QAction("复制页面图像", this);
    m_copyPageAction->setShortcut(QKeySequence::Copy);
    connect(m_copyPageAction, &QAction::triggered, this,
            &ThumbnailContextMenu::onCopyPage);

    // 复制页码
    m_copyPageNumberAction = new QAction("复制页码", this);
    connect(m_copyPageNumberAction, &QAction::triggered, this,
            &ThumbnailContextMenu::onCopyPageNumber);

    // 导出页面
    m_exportPageAction = new QAction("导出页面...", this);
    m_exportPageAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));
    connect(m_exportPageAction, &QAction::triggered, this,
            &ThumbnailContextMenu::onExportPage);

    // 打印页面
    m_printPageAction = new QAction("打印页面", this);
    m_printPageAction->setShortcut(QKeySequence::Print);
    connect(m_printPageAction, &QAction::triggered, this,
            &ThumbnailContextMenu::onPrintPage);

    // 刷新缩略图
    m_refreshPageAction = new QAction("刷新缩略图", this);
    m_refreshPageAction->setShortcut(QKeySequence::Refresh);
    connect(m_refreshPageAction, &QAction::triggered, this,
            &ThumbnailContextMenu::onRefreshPage);

    // 页面信息
    m_pageInfoAction = new QAction("页面信息...", this);
    connect(m_pageInfoAction, &QAction::triggered, this,
            &ThumbnailContextMenu::onShowPageInfo);

    // 设为书签
    m_setBookmarkAction = new QAction("添加书签", this);
    m_setBookmarkAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_B));
    connect(m_setBookmarkAction, &QAction::triggered, this,
            &ThumbnailContextMenu::onSetAsBookmark);

    // 创建分隔符
    m_separator1 = new QAction(this);
    m_separator1->setSeparator(true);

    m_separator2 = new QAction(this);
    m_separator2->setSeparator(true);
}

void ThumbnailContextMenu::setupMenu() {
    // 按逻辑分组添加动作
    addAction(m_goToPageAction);
    addAction(m_separator1);

    addAction(m_copyPageAction);
    addAction(m_copyPageNumberAction);
    addAction(m_exportPageAction);
    addAction(m_printPageAction);
    addAction(m_separator2);

    addAction(m_refreshPageAction);
    addAction(m_pageInfoAction);
    addAction(m_setBookmarkAction);
}

void ThumbnailContextMenu::updateMenuStyle() {
    // Chrome风格的菜单样式
    m_lightStyleSheet = R"(
        QMenu#ThumbnailContextMenu {
            background-color: #ffffff;
            border: 1px solid rgba(0, 0, 0, 0.15);
            border-radius: 8px;
            padding: 4px 0px;
            color: #333333;
            min-width: 180px;
        }
        QMenu#ThumbnailContextMenu::item {
            padding: 8px 16px;
            border: none;
            background-color: transparent;
            color: #333333;
            font-size: 13px;
        }
        QMenu#ThumbnailContextMenu::item:selected {
            background-color: #f1f3f4;
            color: #1a73e8;
            border-radius: 4px;
            margin: 0px 4px;
        }
        QMenu#ThumbnailContextMenu::item:disabled {
            color: #9aa0a6;
        }
        QMenu#ThumbnailContextMenu::separator {
            height: 1px;
            background-color: #e8eaed;
            margin: 4px 8px;
        }
    )";

    m_darkStyleSheet = R"(
        QMenu#ThumbnailContextMenu {
            background-color: #3c4043;
            border: 1px solid rgba(255, 255, 255, 0.15);
            color: #e8eaed;
            border-radius: 8px;
            padding: 4px 0px;
            min-width: 180px;
        }
        QMenu#ThumbnailContextMenu::item {
            padding: 8px 16px;
            border: none;
            background-color: transparent;
            color: #e8eaed;
            font-size: 13px;
        }
        QMenu#ThumbnailContextMenu::item:selected {
            background-color: #5f6368;
            color: #8ab4f8;
            border-radius: 4px;
            margin: 0px 4px;
        }
        QMenu#ThumbnailContextMenu::item:disabled {
            color: #9aa0a6;
        }
        QMenu#ThumbnailContextMenu::separator {
            background-color: #5f6368;
            height: 1px;
            margin: 4px 8px;
        }
    )";

    setStyleSheet(m_isDarkTheme ? m_darkStyleSheet : m_lightStyleSheet);
}

void ThumbnailContextMenu::setDocument(
    std::shared_ptr<Poppler::Document> document) {
    m_document = document;
    updateActionStates();
}

void ThumbnailContextMenu::setThumbnailModel(ThumbnailModel* model) {
    m_thumbnailModel = model;
    updateActionStates();
}

void ThumbnailContextMenu::setCurrentPage(int pageNumber) {
    m_currentPage = pageNumber;
    updateActionStates();
}

void ThumbnailContextMenu::showForPage(int pageNumber,
                                       const QPoint& globalPos) {
    setCurrentPage(pageNumber);
    updateActionStates();
    popup(globalPos);
}

void ThumbnailContextMenu::setActionsEnabled(bool enabled) {
    for (QAction* action : actions()) {
        if (!action->isSeparator()) {
            action->setEnabled(enabled);
        }
    }
}

void ThumbnailContextMenu::updateActionStates() {
    bool hasDocument = (m_document != nullptr);
    bool hasValidPage = (m_currentPage >= 0);
    bool hasModel = (m_thumbnailModel != nullptr);

    bool canOperate = hasDocument && hasValidPage;

    m_goToPageAction->setEnabled(canOperate);
    m_copyPageAction->setEnabled(canOperate);
    m_copyPageNumberAction->setEnabled(hasValidPage);
    m_exportPageAction->setEnabled(canOperate);
    m_printPageAction->setEnabled(canOperate);
    m_refreshPageAction->setEnabled(hasModel && hasValidPage);
    m_pageInfoAction->setEnabled(canOperate);
    m_setBookmarkAction->setEnabled(canOperate);

    // 更新动作文本
    if (hasValidPage) {
        m_goToPageAction->setText(
            QString("跳转到第 %1 页").arg(m_currentPage + 1));
        m_copyPageAction->setText(
            QString("复制第 %1 页图像").arg(m_currentPage + 1));
        m_exportPageAction->setText(
            QString("导出第 %1 页...").arg(m_currentPage + 1));
        m_printPageAction->setText(
            QString("打印第 %1 页").arg(m_currentPage + 1));
        m_refreshPageAction->setText(
            QString("刷新第 %1 页缩略图").arg(m_currentPage + 1));
        m_pageInfoAction->setText(
            QString("第 %1 页信息...").arg(m_currentPage + 1));
        m_setBookmarkAction->setText(
            QString("在第 %1 页添加书签").arg(m_currentPage + 1));
    }
}

void ThumbnailContextMenu::addCustomAction(QAction* action) {
    if (action && !m_customActions.contains(action)) {
        addAction(action);
        m_customActions.append(action);
    }
}

void ThumbnailContextMenu::removeCustomAction(QAction* action) {
    if (action && m_customActions.contains(action)) {
        removeAction(action);
        m_customActions.removeOne(action);
    }
}

void ThumbnailContextMenu::clearCustomActions() {
    for (QAction* action : m_customActions) {
        removeAction(action);
    }
    m_customActions.clear();
}

void ThumbnailContextMenu::onGoToPage() {
    if (m_currentPage >= 0) {
        emit goToPageRequested(m_currentPage);
    }
}

void ThumbnailContextMenu::onCopyPage() {
    if (m_currentPage >= 0) {
        copyPageToClipboard(m_currentPage);
        emit copyPageRequested(m_currentPage);
    }
}

void ThumbnailContextMenu::onCopyPageNumber() {
    if (m_currentPage >= 0 && m_clipboard) {
        QString pageText = QString::number(m_currentPage + 1);
        m_clipboard->setText(pageText);

        // 显示提示消息
        if (parentWidget()) {
            QMessageBox::information(
                parentWidget(), "复制成功",
                QString("页码 %1 已复制到剪贴板").arg(pageText));
        }
    }
}

void ThumbnailContextMenu::onExportPage() {
    if (m_currentPage >= 0) {
        QString defaultPath = getDefaultExportPath(m_currentPage);

        QString filePath = QFileDialog::getSaveFileName(
            parentWidget(), QString("导出第 %1 页").arg(m_currentPage + 1),
            defaultPath,
            "PNG图像 (*.png);;JPEG图像 (*.jpg);;PDF文档 (*.pdf);;所有文件 "
            "(*.*)");

        if (!filePath.isEmpty()) {
            exportPageToFile(m_currentPage);
            emit exportPageRequested(m_currentPage, filePath);
        }
    }
}

void ThumbnailContextMenu::onPrintPage() {
    if (m_currentPage >= 0) {
        emit printPageRequested(m_currentPage);
    }
}

void ThumbnailContextMenu::onRefreshPage() {
    if (m_currentPage >= 0 && m_thumbnailModel) {
        m_thumbnailModel->refreshThumbnail(m_currentPage);
        emit refreshPageRequested(m_currentPage);
    }
}

void ThumbnailContextMenu::onShowPageInfo() {
    if (m_currentPage >= 0) {
        showPageInfoDialog(m_currentPage);
        emit pageInfoRequested(m_currentPage);
    }
}

void ThumbnailContextMenu::onSetAsBookmark() {
    if (m_currentPage >= 0) {
        // Emit signal to notify the application to add bookmark
        emit bookmarkRequested(m_currentPage);

        // Show confirmation message
        QMessageBox::information(
            parentWidget(), "书签",
            QString("已在第 %1 页添加书签").arg(m_currentPage + 1));
    }
}

void ThumbnailContextMenu::copyPageToClipboard(int pageNumber) {
    if (!m_document || !m_clipboard || pageNumber < 0) {
        return;
    }

    try {
        std::unique_ptr<Poppler::Page> page(m_document->page(pageNumber));
        if (!page) {
            QMessageBox::warning(parentWidget(), "错误", "无法获取页面内容");
            return;
        }

        // 渲染页面为图像
        QImage image = page->renderToImage(COPY_DPI, COPY_DPI);
        if (image.isNull()) {
            QMessageBox::warning(parentWidget(), "错误", "无法渲染页面图像");
            return;
        }

        // 复制到剪贴板
        QPixmap pixmap = QPixmap::fromImage(image);
        m_clipboard->setPixmap(pixmap);

        // 显示成功消息
        QMessageBox::information(
            parentWidget(), "复制成功",
            QString("第 %1 页图像已复制到剪贴板").arg(pageNumber + 1));

    } catch (const std::exception& e) {
        QMessageBox::critical(parentWidget(), "错误",
                              QString("复制页面时发生错误: %1").arg(e.what()));
    }
}

void ThumbnailContextMenu::exportPageToFile(int pageNumber) {
    if (!m_document || pageNumber < 0) {
        return;
    }

    QString defaultPath = getDefaultExportPath(pageNumber);

    QString filePath = QFileDialog::getSaveFileName(
        parentWidget(), QString("导出第 %1 页").arg(pageNumber + 1),
        defaultPath,
        "PNG图像 (*.png);;JPEG图像 (*.jpg);;PDF文档 (*.pdf);;所有文件 (*.*)");

    if (filePath.isEmpty()) {
        return;
    }

    try {
        std::unique_ptr<Poppler::Page> page(m_document->page(pageNumber));
        if (!page) {
            QMessageBox::warning(parentWidget(), "错误", "无法获取页面内容");
            return;
        }

        QFileInfo fileInfo(filePath);
        QString extension = fileInfo.suffix().toLower();

        if (extension == "pdf") {
            // 导出为PDF - 创建单页PDF
            exportPageAsPDF(page.get(), filePath);
        } else {
            // 导出为图像格式
            QImage image = page->renderToImage(EXPORT_DPI, EXPORT_DPI);
            if (image.isNull()) {
                QMessageBox::warning(parentWidget(), "错误",
                                     "无法渲染页面图像");
                return;
            }

            QString format =
                (extension == "jpg" || extension == "jpeg") ? "JPEG" : "PNG";
            if (!image.save(filePath, format.toUtf8().constData())) {
                QMessageBox::critical(parentWidget(), "错误", "保存文件失败");
                return;
            }
        }

        QMessageBox::information(parentWidget(), "导出成功",
                                 QString("第 %1 页已成功导出到:\n%2")
                                     .arg(pageNumber + 1)
                                     .arg(filePath));

    } catch (const std::exception& e) {
        QMessageBox::critical(parentWidget(), "错误",
                              QString("导出页面时发生错误: %1").arg(e.what()));
    }
}

void ThumbnailContextMenu::showPageInfoDialog(int pageNumber) {
    if (!m_document || pageNumber < 0) {
        return;
    }

    QString infoText = getPageInfoText(pageNumber);

    QMessageBox infoDialog(parentWidget());
    infoDialog.setWindowTitle(QString("第 %1 页信息").arg(pageNumber + 1));
    infoDialog.setText(infoText);
    infoDialog.setIcon(QMessageBox::Information);
    infoDialog.setStandardButtons(QMessageBox::Ok);
    infoDialog.exec();
}

QString ThumbnailContextMenu::getDefaultExportPath(int pageNumber) const {
    QString documentsPath =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString fileName =
        QString("page_%1.png").arg(pageNumber + 1, 3, 10, QChar('0'));
    return QDir(documentsPath).filePath(fileName);
}

QPixmap ThumbnailContextMenu::getPagePixmap(int pageNumber) const {
    if (m_thumbnailModel) {
        QModelIndex index = m_thumbnailModel->index(pageNumber);
        return index.data(ThumbnailModel::PixmapRole).value<QPixmap>();
    }
    return QPixmap();
}

QString ThumbnailContextMenu::getPageInfoText(int pageNumber) const {
    if (!m_document) {
        return "无文档信息";
    }

    try {
        std::unique_ptr<Poppler::Page> page(m_document->page(pageNumber));
        if (!page) {
            return "无法获取页面信息";
        }

        QSizeF pageSize = page->pageSizeF();
        QString orientation =
            (pageSize.width() > pageSize.height()) ? "横向" : "纵向";

        QString info =
            QString(
                "页码: %1\n"
                "尺寸: %.1f × %.1f 点\n"
                "方向: %2\n"
                "旋转: %3°")
                .arg(pageNumber + 1)
                .arg(pageSize.width())
                .arg(pageSize.height())
                .arg(orientation)
                .arg(0);  // Poppler::Page没有rotation()方法，使用默认值

        return info;

    } catch (const std::exception& e) {
        return QString("获取页面信息时发生错误: %1").arg(e.what());
    }
}

void ThumbnailContextMenu::exportPageAsPDF(Poppler::Page* page,
                                           const QString& filePath) {
    if (!page) {
        return;
    }

    // For PDF export without QPrinter, we'll save as high-quality PNG instead
    // and inform the user about the format change
    QImage image = page->renderToImage(300, 300);  // High DPI for quality
    if (image.isNull()) {
        throw std::runtime_error("Failed to render page for export");
    }

    // Change extension to PNG since we can't create PDF without QPrinter
    QString pngFilePath = filePath;
    pngFilePath.replace(
        QRegularExpression("\\.pdf$",
                           QRegularExpression::CaseInsensitiveOption),
        ".png");

    if (!image.save(pngFilePath, "PNG")) {
        throw std::runtime_error("Failed to save image file");
    }

    // Inform user about format change if needed
    if (pngFilePath != filePath) {
        QMessageBox::information(
            parentWidget(), "格式提示",
            QString("PDF导出功能暂不可用，已保存为高质量PNG格式:\n%1")
                .arg(pngFilePath));
    }
}
