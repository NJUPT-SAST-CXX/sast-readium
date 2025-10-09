#pragma once

#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QStatusBar>
#include <QString>
#include "../../factory/WidgetFactory.h"

class StatusBar : public QStatusBar {
    Q_OBJECT
public:
    explicit StatusBar(QWidget* parent = nullptr);
    StatusBar(WidgetFactory* factory, QWidget* parent = nullptr);

    // 状态信息更新接口
    void setDocumentInfo(const QString& fileName, int currentPage,
                         int totalPages, double zoomLevel);
    void setPageInfo(int current, int total);
    void setZoomLevel(int percent);
    void setZoomLevel(double percent);
    void setFileName(const QString& fileName);
    void setMessage(const QString& message);

    // 页码输入功能
    void enablePageInput(bool enabled);
    void setPageInputRange(int min, int max);

    // 清空状态信息
    void clearDocumentInfo();

    // 加载进度相关方法
    void showLoadingProgress(const QString& message = "正在加载...");
    void updateLoadingProgress(int progress);
    void setLoadingMessage(const QString& message);
    void hideLoadingProgress();

signals:
    void pageJumpRequested(int pageNumber);

private slots:
    void onPageInputReturnPressed();
    void onPageInputEditingFinished();
    void onPageInputTextChanged(const QString& text);

private:
    QLabel* fileNameLabel;
    QLabel* pageLabel;
    QLineEdit* pageInputEdit;
    QLabel* zoomLabel;
    QLabel* separatorLabel1;
    QLabel* separatorLabel2;
    QLabel* separatorLabel3;

    // 加载进度相关控件
    QProgressBar* loadingProgressBar;
    QLabel* loadingMessageLabel;
    QPropertyAnimation* progressAnimation;

    int currentTotalPages;

    void setupUI();
    void setupSeparators();
    void setupPageInput();
    void setupLoadingProgress();
    QString formatFileName(const QString& fullPath) const;
    bool validateAndJumpToPage(const QString& input);
};
