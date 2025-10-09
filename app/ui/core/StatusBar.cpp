#include "StatusBar.h"
#include <QEasingCurve>
#include <QFileInfo>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPropertyAnimation>

StatusBar::StatusBar(QWidget* parent)
    : QStatusBar(parent), currentTotalPages(0) {
    setupUI();
    setupLoadingProgress();
}

void StatusBar::setupUI() {
    // 创建文件名标签
    fileNameLabel = new QLabel("无文档", this);
    fileNameLabel->setMinimumWidth(150);
    fileNameLabel->setMaximumWidth(300);
    fileNameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    fileNameLabel->setStyleSheet("QLabel { padding: 2px 8px; }");

    // 创建页面信息标签和输入框
    pageLabel = new QLabel("页:", this);
    pageLabel->setAlignment(Qt::AlignCenter);
    pageLabel->setStyleSheet("QLabel { padding: 2px 4px; }");

    setupPageInput();

    // 创建缩放信息标签
    zoomLabel = new QLabel("缩放: 100%", this);
    zoomLabel->setMinimumWidth(80);
    zoomLabel->setAlignment(Qt::AlignCenter);
    zoomLabel->setStyleSheet("QLabel { padding: 2px 8px; }");

    setupSeparators();

    // 添加到状态栏（从右到左的顺序）
    addPermanentWidget(zoomLabel);
    addPermanentWidget(separatorLabel2);
    addPermanentWidget(pageInputEdit);
    addPermanentWidget(pageLabel);
    addPermanentWidget(separatorLabel1);
    addPermanentWidget(fileNameLabel);
}

void StatusBar::setupPageInput() {
    // 创建页码输入框
    pageInputEdit = new QLineEdit(this);
    pageInputEdit->setMaximumWidth(60);
    pageInputEdit->setMinimumWidth(60);
    pageInputEdit->setAlignment(Qt::AlignCenter);
    pageInputEdit->setPlaceholderText("页码");
    pageInputEdit->setStyleSheet(
        "QLineEdit { "
        "padding: 2px 4px; "
        "border: 1px solid gray; "
        "border-radius: 3px; "
        "background-color: white; "
        "} "
        "QLineEdit:focus { "
        "border: 2px solid #0078d4; "
        "background-color: #f0f8ff; "
        "} "
        "QLineEdit:disabled { "
        "background-color: #f0f0f0; "
        "color: #808080; "
        "}");
    pageInputEdit->setEnabled(false);  // 默认禁用，直到有文档加载

    // 添加输入验证器
    QIntValidator* validator = new QIntValidator(1, 9999, this);
    pageInputEdit->setValidator(validator);

    // 连接信号
    connect(pageInputEdit, &QLineEdit::returnPressed, this,
            &StatusBar::onPageInputReturnPressed);
    connect(pageInputEdit, &QLineEdit::editingFinished, this,
            &StatusBar::onPageInputEditingFinished);
    connect(pageInputEdit, &QLineEdit::textChanged, this,
            &StatusBar::onPageInputTextChanged);
}

void StatusBar::setupSeparators() {
    // 创建分隔符
    separatorLabel1 = new QLabel("|", this);
    separatorLabel1->setAlignment(Qt::AlignCenter);
    separatorLabel1->setStyleSheet("QLabel { color: gray; padding: 2px 4px; }");

    separatorLabel2 = new QLabel("|", this);
    separatorLabel2->setAlignment(Qt::AlignCenter);
    separatorLabel2->setStyleSheet("QLabel { color: gray; padding: 2px 4px; }");

    separatorLabel3 = new QLabel("|", this);
    separatorLabel3->setAlignment(Qt::AlignCenter);
    separatorLabel3->setStyleSheet("QLabel { color: gray; padding: 2px 4px; }");
}

void StatusBar::setDocumentInfo(const QString& fileName, int currentPage,
                                int totalPages, double zoomLevel) {
    setFileName(fileName);
    setPageInfo(currentPage, totalPages);
    setZoomLevel(zoomLevel);
}

void StatusBar::setPageInfo(int current, int total) {
    currentTotalPages = total;

    if (total > 0) {
        // 更新页码输入框的占位符文本
        pageInputEdit->setPlaceholderText(
            QString("%1/%2").arg(current + 1).arg(total));
        pageInputEdit->setEnabled(true);
        pageInputEdit->setToolTip(
            QString("输入页码 (1-%1) 并按回车跳转").arg(total));
    } else {
        pageInputEdit->setPlaceholderText("0/0");
        pageInputEdit->setEnabled(false);
        pageInputEdit->setToolTip("");
    }
}

void StatusBar::setZoomLevel(int percent) {
    zoomLabel->setText(QString("缩放: %1%").arg(percent));
}

void StatusBar::setZoomLevel(double percent) {
    int roundedPercent = static_cast<int>(percent * 100 + 0.5);  // 四舍五入
    setZoomLevel(roundedPercent);
}

void StatusBar::setFileName(const QString& fileName) {
    if (fileName.isEmpty()) {
        fileNameLabel->setText("无文档");
    } else {
        QString displayName = formatFileName(fileName);
        fileNameLabel->setText(displayName);
        fileNameLabel->setToolTip(fileName);  // 完整路径作为工具提示
    }
}

void StatusBar::setMessage(const QString& message) {
    showMessage(message, 3000);
}

void StatusBar::clearDocumentInfo() {
    fileNameLabel->setText("无文档");
    fileNameLabel->setToolTip("");
    pageInputEdit->setPlaceholderText("0/0");
    pageInputEdit->setEnabled(false);
    pageInputEdit->setToolTip("");
    pageInputEdit->clear();
    zoomLabel->setText("缩放: 100%");
    currentTotalPages = 0;
}

QString StatusBar::formatFileName(const QString& fullPath) const {
    if (fullPath.isEmpty()) {
        return "无文档";
    }

    QFileInfo fileInfo(fullPath);
    QString baseName = fileInfo.baseName();

    // 如果文件名太长，进行截断
    QFontMetrics metrics(fileNameLabel->font());
    int maxWidth = fileNameLabel->maximumWidth() - 16;  // 留出padding空间

    if (metrics.horizontalAdvance(baseName) > maxWidth) {
        baseName = metrics.elidedText(baseName, Qt::ElideMiddle, maxWidth);
    }

    return baseName;
}

void StatusBar::onPageInputReturnPressed() {
    QString input = pageInputEdit->text().trimmed();
    if (validateAndJumpToPage(input)) {
        pageInputEdit->clear();
        pageInputEdit->clearFocus();
        // 恢复正常样式
        pageInputEdit->setStyleSheet(
            pageInputEdit->styleSheet().replace("border: 2px solid red;", ""));
    }
}

void StatusBar::onPageInputEditingFinished() {
    // 当输入框失去焦点时，清空内容并恢复样式
    pageInputEdit->clear();
    pageInputEdit->setStyleSheet(
        pageInputEdit->styleSheet().replace("border: 2px solid red;", ""));
}

void StatusBar::onPageInputTextChanged(const QString& text) {
    // 实时验证输入
    if (text.isEmpty()) {
        // 恢复正常样式
        pageInputEdit->setStyleSheet(
            pageInputEdit->styleSheet().replace("border: 2px solid red;", ""));
        return;
    }

    bool ok;
    int pageNumber = text.toInt(&ok);

    if (!ok || pageNumber < 1 || pageNumber > currentTotalPages) {
        // 显示错误样式
        QString currentStyle = pageInputEdit->styleSheet();
        if (!currentStyle.contains("border: 2px solid red;")) {
            pageInputEdit->setStyleSheet(
                currentStyle + " QLineEdit:focus { border: 2px solid red; }");
        }
    } else {
        // 恢复正常样式
        pageInputEdit->setStyleSheet(
            pageInputEdit->styleSheet().replace("border: 2px solid red;", ""));
    }
}

bool StatusBar::validateAndJumpToPage(const QString& input) {
    if (input.isEmpty()) {
        showMessage("请输入页码", 1500);
        return false;
    }

    // 检查是否有有效文档
    if (currentTotalPages <= 0) {
        showMessage("没有可跳转的文档", 2000);
        return false;
    }

    bool ok;
    int pageNumber = input.toInt(&ok);

    if (!ok) {
        showMessage("请输入有效的页码数字", 2000);
        // 添加错误样式
        QString currentStyle = pageInputEdit->styleSheet();
        if (!currentStyle.contains("border: 2px solid red;")) {
            pageInputEdit->setStyleSheet(
                currentStyle + " QLineEdit { border: 2px solid red; }");
        }
        return false;
    }

    if (pageNumber < 1 || pageNumber > currentTotalPages) {
        showMessage(QString("页码超出范围 (1-%1)").arg(currentTotalPages),
                    2000);
        // 添加错误样式
        QString currentStyle = pageInputEdit->styleSheet();
        if (!currentStyle.contains("border: 2px solid red;")) {
            pageInputEdit->setStyleSheet(
                currentStyle + " QLineEdit { border: 2px solid red; }");
        }
        return false;
    }

    // 发出页码跳转信号
    emit pageJumpRequested(pageNumber - 1);  // 转换为0-based
    showMessage(QString("跳转到第 %1 页").arg(pageNumber), 1000);
    return true;
}

void StatusBar::enablePageInput(bool enabled) {
    pageInputEdit->setEnabled(enabled);
}

void StatusBar::setPageInputRange(int min, int max) {
    currentTotalPages = max;
    if (max > 0) {
        pageInputEdit->setToolTip(
            QString("输入页码 (%1-%2) 并按回车跳转").arg(min).arg(max));
    }
}

StatusBar::StatusBar(WidgetFactory* factory, QWidget* parent)
    : StatusBar(parent) {
    QPushButton* prevButton = factory->createButton(actionID::prev, "Prev");
    QPushButton* nextButton = factory->createButton(actionID::next, "Next");

    addWidget(prevButton);
    addWidget(nextButton);
}

void StatusBar::setupLoadingProgress() {
    // 创建加载进度条
    loadingProgressBar = new QProgressBar(this);
    loadingProgressBar->setMinimumWidth(200);
    loadingProgressBar->setMaximumWidth(300);
    loadingProgressBar->setMinimum(0);
    loadingProgressBar->setMaximum(100);
    loadingProgressBar->setValue(0);
    loadingProgressBar->setVisible(false);
    loadingProgressBar->setStyleSheet(
        "QProgressBar {"
        "    border: 1px solid #ccc;"
        "    border-radius: 3px;"
        "    text-align: center;"
        "    font-size: 11px;"
        "}"
        "QProgressBar::chunk {"
        "    background-color: #4CAF50;"
        "    border-radius: 2px;"
        "}");

    // 创建加载消息标签
    loadingMessageLabel = new QLabel(this);
    loadingMessageLabel->setVisible(false);
    loadingMessageLabel->setStyleSheet(
        "QLabel { padding: 2px 8px; color: #666; }");

    // 创建进度动画
    progressAnimation =
        new QPropertyAnimation(loadingProgressBar, "value", this);
    progressAnimation->setDuration(300);
    progressAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // 添加到状态栏（在最左侧）
    insertPermanentWidget(0, loadingMessageLabel);
    insertPermanentWidget(1, loadingProgressBar);
}

void StatusBar::showLoadingProgress(const QString& message) {
    loadingMessageLabel->setText(message);
    loadingMessageLabel->setVisible(true);
    loadingProgressBar->setValue(0);
    loadingProgressBar->setVisible(true);

    // 隐藏其他控件以节省空间
    fileNameLabel->setVisible(false);
    separatorLabel1->setVisible(false);
}

void StatusBar::updateLoadingProgress(int progress) {
    progress = qBound(0, progress, 100);

    // 使用动画更新进度
    progressAnimation->stop();
    progressAnimation->setStartValue(loadingProgressBar->value());
    progressAnimation->setEndValue(progress);
    progressAnimation->start();
}

void StatusBar::setLoadingMessage(const QString& message) {
    if (loadingMessageLabel->isVisible()) {
        loadingMessageLabel->setText(message);
    }
}

void StatusBar::hideLoadingProgress() {
    loadingProgressBar->setVisible(false);
    loadingMessageLabel->setVisible(false);

    // 恢复其他控件显示
    fileNameLabel->setVisible(true);
    separatorLabel1->setVisible(true);
}
