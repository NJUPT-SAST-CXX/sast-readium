#include "SearchWidget.h"
#include <QApplication>
#include <QDebug>
#include <QKeySequence>
#include <QMessageBox>
#include <QShortcut>
#include <QStyle>

SearchWidget::SearchWidget(QWidget* parent)
    : QWidget(parent),
      m_searchModel(new SearchModel(this)),
      m_document(nullptr),
      m_searchTimer(new QTimer(this)),
      m_optionsVisible(false) {
    setupUI();
    setupConnections();
    setupShortcuts();

    // Configure search timer for debounced search
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(300);  // 300ms delay

    setSearchInProgress(false);
    showSearchOptions(false);
}

void SearchWidget::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(6, 6, 6, 6);
    m_mainLayout->setSpacing(4);

    // Search input layout
    m_searchLayout = new QHBoxLayout();

    m_searchInput = new QLineEdit();
    m_searchInput->setPlaceholderText("搜索文档内容...");
    m_searchInput->setClearButtonEnabled(true);

    m_searchButton = new QPushButton("搜索");
    m_searchButton->setDefault(true);

    m_optionsButton = new QPushButton("选项");
    m_optionsButton->setCheckable(true);

    m_closeButton = new QPushButton("×");
    m_closeButton->setMaximumWidth(30);
    m_closeButton->setToolTip("关闭搜索");

    m_searchLayout->addWidget(m_searchInput);
    m_searchLayout->addWidget(m_searchButton);
    m_searchLayout->addWidget(m_optionsButton);
    m_searchLayout->addWidget(m_closeButton);

    // Navigation layout
    m_navigationLayout = new QHBoxLayout();

    m_previousButton = new QPushButton("上一个");
    m_nextButton = new QPushButton("下一个");
    m_resultInfoLabel = new QLabel("0 / 0");

    m_navigationLayout->addWidget(m_previousButton);
    m_navigationLayout->addWidget(m_nextButton);
    m_navigationLayout->addStretch();
    m_navigationLayout->addWidget(m_resultInfoLabel);

    // Search options group
    m_optionsGroup = new QGroupBox("搜索选项");
    QVBoxLayout* optionsLayout = new QVBoxLayout(m_optionsGroup);

    m_caseSensitiveCheck = new QCheckBox("区分大小写");
    m_wholeWordsCheck = new QCheckBox("全词匹配");
    m_regexCheck = new QCheckBox("正则表达式");
    m_searchBackwardCheck = new QCheckBox("向后搜索");

    optionsLayout->addWidget(m_caseSensitiveCheck);
    optionsLayout->addWidget(m_wholeWordsCheck);
    optionsLayout->addWidget(m_regexCheck);
    optionsLayout->addWidget(m_searchBackwardCheck);

    // Results view
    m_resultsView = new QListView();
    m_resultsView->setModel(m_searchModel);
    m_resultsView->setAlternatingRowColors(true);
    m_resultsView->setSelectionMode(QAbstractItemView::SingleSelection);

    // Status and progress
    m_statusLabel = new QLabel("准备搜索");
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);

    // Add to main layout
    m_mainLayout->addLayout(m_searchLayout);
    m_mainLayout->addLayout(m_navigationLayout);
    m_mainLayout->addWidget(m_optionsGroup);
    m_mainLayout->addWidget(m_resultsView);
    m_mainLayout->addWidget(m_statusLabel);
    m_mainLayout->addWidget(m_progressBar);
}

void SearchWidget::setupConnections() {
    // Search input and controls
    connect(m_searchInput, &QLineEdit::textChanged, this,
            &SearchWidget::onSearchTextChanged);
    connect(m_searchInput, &QLineEdit::returnPressed, this,
            &SearchWidget::performSearch);
    connect(m_searchButton, &QPushButton::clicked, this,
            &SearchWidget::performSearch);
    connect(m_optionsButton, &QPushButton::toggled, this,
            &SearchWidget::toggleSearchOptions);
    connect(m_closeButton, &QPushButton::clicked, this,
            &SearchWidget::searchClosed);

    // Navigation
    connect(m_previousButton, &QPushButton::clicked, this,
            &SearchWidget::previousResult);
    connect(m_nextButton, &QPushButton::clicked, this,
            &SearchWidget::nextResult);

    // Results view
    connect(m_resultsView, &QListView::clicked, this,
            &SearchWidget::onResultClicked);
    connect(m_resultsView, &QListView::doubleClicked, this,
            &SearchWidget::onResultClicked);

    // Search model signals
    connect(m_searchModel, &SearchModel::searchStarted, this,
            &SearchWidget::onSearchStarted);
    connect(m_searchModel, &SearchModel::searchFinished, this,
            &SearchWidget::onSearchFinished);
    connect(m_searchModel, &SearchModel::searchError, this,
            &SearchWidget::onSearchError);
    connect(m_searchModel, &SearchModel::currentResultChanged, this,
            &SearchWidget::onCurrentResultChanged);

    // Search timer for real-time search
    connect(m_searchTimer, &QTimer::timeout, this,
            &SearchWidget::performRealTimeSearch);

    // Real-time search signals
    connect(m_searchModel, &SearchModel::realTimeSearchStarted, this,
            &SearchWidget::onRealTimeSearchStarted);
    connect(m_searchModel, &SearchModel::realTimeResultsUpdated, this,
            &SearchWidget::onRealTimeResultsUpdated);
    connect(m_searchModel, &SearchModel::realTimeSearchProgress, this,
            &SearchWidget::onRealTimeSearchProgress);
}

void SearchWidget::setupShortcuts() {
    m_findShortcut = new QShortcut(QKeySequence::Find, this);
    connect(m_findShortcut, &QShortcut::activated, this,
            &SearchWidget::focusSearchInput);

    m_findNextShortcut = new QShortcut(QKeySequence::FindNext, this);
    connect(m_findNextShortcut, &QShortcut::activated, this,
            &SearchWidget::nextResult);

    m_findPreviousShortcut = new QShortcut(QKeySequence::FindPrevious, this);
    connect(m_findPreviousShortcut, &QShortcut::activated, this,
            &SearchWidget::previousResult);

    m_escapeShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(m_escapeShortcut, &QShortcut::activated, this,
            &SearchWidget::searchClosed);
}

void SearchWidget::setDocument(Poppler::Document* document) {
    m_document = document;
    clearSearch();
}

void SearchWidget::focusSearchInput() {
    m_searchInput->setFocus();
    m_searchInput->selectAll();
}

void SearchWidget::clearSearch() {
    m_searchInput->clear();
    m_searchModel->clearResults();
    updateNavigationButtons();
    updateResultsInfo();
    m_statusLabel->setText("准备搜索");
}

void SearchWidget::showSearchOptions(bool show) {
    m_optionsVisible = show;
    m_optionsGroup->setVisible(show);
    m_optionsButton->setChecked(show);
}

bool SearchWidget::hasResults() const { return m_searchModel->rowCount() > 0; }

int SearchWidget::getResultCount() const { return m_searchModel->rowCount(); }

SearchResult SearchWidget::getCurrentResult() const {
    int currentIndex = m_searchModel->getCurrentResultIndex();
    if (currentIndex >= 0) {
        return m_searchModel->getResult(currentIndex);
    }
    return SearchResult();
}

void SearchWidget::performSearch() {
    QString query = m_searchInput->text().trimmed();
    if (query.isEmpty() || !m_document) {
        return;
    }

    SearchOptions options = getSearchOptions();
    m_searchModel->startSearch(m_document, query, options);
    emit searchRequested(query, options);
}

void SearchWidget::performRealTimeSearch() {
    QString query = m_searchInput->text().trimmed();
    if (query.isEmpty() || !m_document) {
        return;
    }

    SearchOptions options = getSearchOptions();
    m_searchModel->startRealTimeSearch(m_document, query, options);
    emit searchRequested(query, options);
}

void SearchWidget::nextResult() {
    if (m_searchModel->hasNext()) {
        SearchResult result = m_searchModel->nextResult();

        // Update UI with current result info
        updateResultsInfo();
        updateNavigationButtons();

        // Navigate to result with highlighting
        emit navigateToResult(result.pageNumber, result.boundingRect);
        emit resultSelected(result);

        // Update status with current position
        int currentIndex = m_searchModel->getCurrentResultIndex();
        int totalResults = m_searchModel->getResults().size();
        m_statusLabel->setText(
            QString("结果 %1 / %2").arg(currentIndex + 1).arg(totalResults));
    }
}

void SearchWidget::previousResult() {
    if (m_searchModel->hasPrevious()) {
        SearchResult result = m_searchModel->previousResult();

        // Update UI with current result info
        updateResultsInfo();
        updateNavigationButtons();

        // Navigate to result with highlighting
        emit navigateToResult(result.pageNumber, result.boundingRect);
        emit resultSelected(result);

        // Update status with current position
        int currentIndex = m_searchModel->getCurrentResultIndex();
        int totalResults = m_searchModel->getResults().size();
        m_statusLabel->setText(
            QString("结果 %1 / %2").arg(currentIndex + 1).arg(totalResults));
    }
}

void SearchWidget::onResultClicked(const QModelIndex& index) {
    if (index.isValid()) {
        SearchResult result = m_searchModel->getResult(index.row());
        m_searchModel->setCurrentResultIndex(index.row());
        emit navigateToResult(result.pageNumber, result.boundingRect);
        emit resultSelected(result);
    }
}

void SearchWidget::onSearchTextChanged() {
    // Restart timer for debounced real-time search
    m_searchTimer->stop();

    QString query = m_searchInput->text().trimmed();
    if (!query.isEmpty() && m_document) {
        m_searchTimer->start();
    } else {
        // Clear search results when input is empty
        clearSearch();
        emit searchCleared();
    }
}

void SearchWidget::onSearchStarted() {
    setSearchInProgress(true);
    m_statusLabel->setText("正在搜索...");
}

void SearchWidget::onSearchFinished(int resultCount) {
    setSearchInProgress(false);
    updateNavigationButtons();
    updateResultsInfo();

    if (resultCount > 0) {
        m_statusLabel->setText(QString("找到 %1 个结果").arg(resultCount));
        // Auto-navigate to first result
        if (m_searchModel->getCurrentResultIndex() >= 0) {
            SearchResult result = m_searchModel->getResult(0);
            emit navigateToResult(result.pageNumber, result.boundingRect);
            emit resultSelected(result);
        }
    } else {
        m_statusLabel->setText("未找到匹配结果");
    }
}

void SearchWidget::onSearchError(const QString& error) {
    setSearchInProgress(false);
    m_statusLabel->setText(QString("搜索错误: %1").arg(error));
    QMessageBox::warning(this, "搜索错误", error);
}

void SearchWidget::onCurrentResultChanged(int index) {
    updateNavigationButtons();
    updateResultsInfo();

    // Update selection in results view
    if (index >= 0 && index < m_searchModel->rowCount()) {
        QModelIndex modelIndex = m_searchModel->index(index);
        m_resultsView->setCurrentIndex(modelIndex);
    }
}

void SearchWidget::toggleSearchOptions() {
    showSearchOptions(m_optionsButton->isChecked());
}

void SearchWidget::updateNavigationButtons() {
    m_previousButton->setEnabled(m_searchModel->hasPrevious());
    m_nextButton->setEnabled(m_searchModel->hasNext());
}

void SearchWidget::updateResultsInfo() {
    int current = m_searchModel->getCurrentResultIndex() + 1;
    int total = m_searchModel->rowCount();

    if (total > 0) {
        m_resultInfoLabel->setText(QString("%1 / %2").arg(current).arg(total));
    } else {
        m_resultInfoLabel->setText("0 / 0");
    }
}

SearchOptions SearchWidget::getSearchOptions() const {
    SearchOptions options;
    options.caseSensitive = m_caseSensitiveCheck->isChecked();
    options.wholeWords = m_wholeWordsCheck->isChecked();
    options.useRegex = m_regexCheck->isChecked();
    options.searchBackward = m_searchBackwardCheck->isChecked();
    return options;
}

void SearchWidget::setSearchInProgress(bool inProgress) {
    m_searchButton->setEnabled(!inProgress);
    m_progressBar->setVisible(inProgress);

    if (inProgress) {
        m_progressBar->setRange(0, 0);  // Indeterminate progress
    }
}

// Real-time search slot implementations
void SearchWidget::onRealTimeSearchStarted() {
    setSearchInProgress(true);
    m_statusLabel->setText("实时搜索中...");
}

void SearchWidget::onRealTimeResultsUpdated(
    const QList<SearchResult>& results) {
    // Update navigation buttons and result info
    updateNavigationButtons();
    updateResultsInfo();

    // Emit signal to update highlights in the viewer
    if (!results.isEmpty()) {
        emit resultSelected(results.first());  // Select first result by default
    }
}

void SearchWidget::onRealTimeSearchProgress(int currentPage, int totalPages) {
    m_statusLabel->setText(
        QString("搜索进度: %1/%2 页").arg(currentPage).arg(totalPages));
}

// Navigation helper methods
void SearchWidget::navigateToCurrentResult() {
    if (m_searchModel->getResults().isEmpty()) {
        return;
    }

    int currentIndex = m_searchModel->getCurrentResultIndex();
    if (currentIndex >= 0 &&
        currentIndex < m_searchModel->getResults().size()) {
        SearchResult result = m_searchModel->getResult(currentIndex);
        emit navigateToResult(result.pageNumber, result.boundingRect);
        emit resultSelected(result);
    }
}
