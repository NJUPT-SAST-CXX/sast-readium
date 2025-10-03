#pragma once

#include <QPropertyAnimation>
#include <QSettings>
#include <QTabWidget>
#include <QWidget>
#include <memory>
#include "../../delegate/ThumbnailDelegate.h"
#include "../../model/PDFOutlineModel.h"
#include "../../model/ThumbnailModel.h"
#include "../thumbnail/ThumbnailListView.h"
#include "../viewer/PDFOutlineWidget.h"

// 前向声明
namespace Poppler {
class Document;
}

class SideBar : public QWidget {
    Q_OBJECT
public:
    SideBar(QWidget* parent = nullptr);

    // 显示/隐藏控制
    bool isVisible() const;
    using QWidget::setVisible;  // Bring base class method into scope
    void setVisible(bool visible, bool animated = true);
    void toggleVisibility(bool animated = true);

    // 宽度管理
    int getPreferredWidth() const;
    void setPreferredWidth(int width);
    int getMinimumWidth() const { return minimumWidth; }
    int getMaximumWidth() const { return maximumWidth; }

    // 状态持久化
    void saveState();
    void restoreState();

    // PDF目录相关
    void setOutlineModel(PDFOutlineModel* model);
    PDFOutlineWidget* getOutlineWidget() const { return outlineWidget; }

    // 缩略图相关
    void setDocument(std::shared_ptr<Poppler::Document> document);
    void setThumbnailSize(const QSize& size);
    void refreshThumbnails();
    ThumbnailListView* getThumbnailView() const { return thumbnailView; }
    ThumbnailModel* getThumbnailModel() const { return thumbnailModel.get(); }

public slots:
    void show(bool animated = true);
    void hide(bool animated = true);

signals:
    void visibilityChanged(bool visible);
    void widthChanged(int width);

private slots:
    void onAnimationFinished();

signals:
    void pageClicked(int pageNumber);
    void pageDoubleClicked(int pageNumber);
    void thumbnailSizeChanged(const QSize& size);

private:
    QTabWidget* tabWidget;
    QPropertyAnimation* animation;
    QSettings* settings;
    PDFOutlineWidget* outlineWidget;

    // 缩略图组件
    ThumbnailListView* thumbnailView;
    std::unique_ptr<ThumbnailModel> thumbnailModel;
    std::unique_ptr<ThumbnailDelegate> thumbnailDelegate;

    bool isCurrentlyVisible;
    int preferredWidth;
    int lastWidth;

    static const int minimumWidth = 200;
    static const int maximumWidth = 400;
    static const int defaultWidth = 250;
    static const int animationDuration = 300;

    void initWindow();
    void initContent();
    void initAnimation();
    void initSettings();

    QWidget* createThumbnailsTab();
    QWidget* createBookmarksTab();
};
