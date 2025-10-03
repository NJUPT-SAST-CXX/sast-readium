#pragma once

#include <poppler/qt6/poppler-qt6.h>
#include <QAction>
#include <QActionGroup>
#include <QClipboard>
#include <QDir>
#include <QFileDialog>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QStandardPaths>
#include <QStringList>
#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <memory>

class ThumbnailModel;

/**
 * @brief 缩略图右键菜单管理器
 *
 * 提供Chrome风格的右键菜单功能：
 * - 复制页面图像
 * - 导出页面为图片
 * - 打印单页
 * - 页面信息显示
 * - 刷新缩略图
 */
class ThumbnailContextMenu : public QMenu {
    Q_OBJECT

public:
    explicit ThumbnailContextMenu(QWidget* parent = nullptr);
    ~ThumbnailContextMenu() override;

    // 设置上下文
    void setDocument(std::shared_ptr<Poppler::Document> document);
    void setThumbnailModel(ThumbnailModel* model);
    void setCurrentPage(int pageNumber);

    // 菜单显示
    void showForPage(int pageNumber, const QPoint& globalPos);

    // 动作启用/禁用
    void setActionsEnabled(bool enabled);
    void updateActionStates();

    // 自定义动作
    void addCustomAction(QAction* action);
    void removeCustomAction(QAction* action);
    void clearCustomActions();

signals:
    void copyPageRequested(int pageNumber);
    void exportPageRequested(int pageNumber, const QString& filePath);
    void printPageRequested(int pageNumber);
    void refreshPageRequested(int pageNumber);
    void pageInfoRequested(int pageNumber);
    void goToPageRequested(int pageNumber);
    void bookmarkRequested(int pageNumber);

private slots:
    void onCopyPage();
    void onExportPage();
    void onPrintPage();
    void onRefreshPage();
    void onShowPageInfo();
    void onGoToPage();
    void onCopyPageNumber();
    void onSetAsBookmark();

private:
    void createActions();
    void setupMenu();
    void updateMenuStyle();

    void copyPageToClipboard(int pageNumber);
    void exportPageToFile(int pageNumber);
    void exportPageAsPDF(Poppler::Page* page, const QString& filePath);
    void showPageInfoDialog(int pageNumber);

    QString getDefaultExportPath(int pageNumber) const;
    QPixmap getPagePixmap(int pageNumber) const;
    QString getPageInfoText(int pageNumber) const;

private:
    // 核心数据
    std::shared_ptr<Poppler::Document> m_document;
    ThumbnailModel* m_thumbnailModel;
    int m_currentPage;

    // 菜单动作
    QAction* m_copyPageAction;
    QAction* m_exportPageAction;
    QAction* m_printPageAction;
    QAction* m_refreshPageAction;
    QAction* m_pageInfoAction;
    QAction* m_goToPageAction;
    QAction* m_copyPageNumberAction;
    QAction* m_setBookmarkAction;

    // 分隔符
    QAction* m_separator1;
    QAction* m_separator2;

    // 自定义动作
    QList<QAction*> m_customActions;

    // 系统组件
    QClipboard* m_clipboard;

    // 样式设置
    QString m_lightStyleSheet;
    QString m_darkStyleSheet;
    bool m_isDarkTheme;

    // 常量
    static constexpr int EXPORT_DPI = 150;
    static constexpr int COPY_DPI = 96;
    static const QString DEFAULT_EXPORT_FORMAT;
    static const QStringList SUPPORTED_EXPORT_FORMATS;
};
