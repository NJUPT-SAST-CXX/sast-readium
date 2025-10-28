#include "SearchWidget.h"
#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDebug>
#include <QDir>
#include <QEvent>
#include <QGroupBox>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QShortcut>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStyle>
#include "../../logging/LoggingMacros.h"
#include "../../managers/StyleManager.h"
#include "../core/UIErrorHandler.h"

SearchWidget::SearchWidget(QWidget* parent)
    : QWidget(parent),
      m_mainLayout(nullptr),
      m_searchLayout(nullptr),
      m_navigationLayout(nullptr),
      m_infoLayout(nullptr),
      m_searchInput(nullptr),
      m_searchButton(nullptr),
      m_clearButton(nullptr),
      m_optionsButton(nullptr),
      m_closeButton(nullptr),
      m_previousButton(nullptr),
      m_nextButton(nullptr),
      m_resultInfoLabel(nullptr),
      m_optionsGroup(nullptr),
      m_caseSensitiveCheck(nullptr),
      m_wholeWordsCheck(nullptr),
      m_regexCheck(nullptr),
      m_searchBackwardCheck(nullptr),
      m_fuzzySearchCheck(nullptr),
      m_fuzzyThresholdSpin(nullptr),
      m_fuzzyThresholdLabel(nullptr),
      m_pageRangeGroup(nullptr),
      m_pageRangeCheck(nullptr),
      m_startPageSpin(nullptr),
      m_endPageSpin(nullptr),
      m_pageRangeLabel(nullptr),
      m_searchHistoryCombo(nullptr),
      m_clearHistoryButton(nullptr),
      m_resultsView(nullptr),
      m_statusLabel(nullptr),
      m_progressBar(nullptr),
      m_searchProgressLabel(nullptr),
      m_searchProgressBar(nullptr),
      m_highlightColorButton(nullptr),
      m_currentHighlightColorButton(nullptr),
      m_searchModel(new SearchModel(this)),
      m_document(nullptr),
      m_searchTimer(new QTimer(this)),
      m_optionsVisible(false),
      m_findShortcut(nullptr),
      m_findNextShortcut(nullptr),
      m_findPreviousShortcut(nullptr),
      m_escapeShortcut(nullptr) {
    // Initialize context menu manager
    contextMenuManager = new ContextMenuManager(this);

    // Initialize logging
    LOG_INFO("Initializing SearchWidget");

    setupUI();
    setupConnections();
    setupShortcuts();

    // Configure search timer for debounced search
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(300);  // 300ms delay

    setSearchInProgress(false);
    showSearchOptions(false);

    // Load persistent search history
    loadSearchHistoryFromSettings();

    LOG_DEBUG("SearchWidget initialization complete");
}

SearchWidget::~SearchWidget() {
    // Cancel any ongoing search
    cancelCurrentSearch();

    // Save settings before destruction
    try {
        saveSearchHistoryToSettings();
        LOG_DEBUG("SearchWidget destroyed and settings saved");
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to save settings during destruction: {}", e.what());
    }
}

void SearchWidget::setupUI() {
    StyleManager* styleManager = &StyleManager::instance();

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(
        styleManager->spacingSM(), styleManager->spacingSM(),
        styleManager->spacingSM(), styleManager->spacingSM());
    m_mainLayout->setSpacing(styleManager->spacingXS());

    // Search input layout with history
    m_searchLayout = new QHBoxLayout();

    // Search history combo box
    m_searchHistoryCombo = new QComboBox();
    m_searchHistoryCombo->setEditable(true);
    m_searchHistoryCombo->setInsertPolicy(QComboBox::NoInsert);
    m_searchHistoryCombo->lineEdit()->setPlaceholderText(
        tr("Search document content..."));
    m_searchHistoryCombo->lineEdit()->setClearButtonEnabled(true);

    // Use the combo box line edit as our search input
    m_searchInput = m_searchHistoryCombo->lineEdit();

    m_searchButton = new QPushButton(tr("Search"));
    m_searchButton->setDefault(true);

    m_clearHistoryButton = new QPushButton(tr("Clear History"));
    m_clearHistoryButton->setToolTip(tr("Clear search history"));

    m_optionsButton = new QPushButton(tr("Options"));
    m_optionsButton->setCheckable(true);

    m_closeButton = new QPushButton("×");
    m_closeButton->setMaximumWidth(30);
    m_closeButton->setToolTip(tr("Close search"));

    m_searchLayout->addWidget(m_searchHistoryCombo);
    m_searchLayout->addWidget(m_searchButton);
    m_searchLayout->addWidget(m_clearHistoryButton);
    m_searchLayout->addWidget(m_optionsButton);
    m_searchLayout->addWidget(m_closeButton);

    // Navigation layout
    m_navigationLayout = new QHBoxLayout();

    m_previousButton = new QPushButton(tr("Previous"));
    m_nextButton = new QPushButton(tr("Next"));
    m_resultInfoLabel = new QLabel("0 / 0");

    m_navigationLayout->addWidget(m_previousButton);
    m_navigationLayout->addWidget(m_nextButton);
    m_navigationLayout->addStretch();
    m_navigationLayout->addWidget(m_resultInfoLabel);

    // Search options group
    m_optionsGroup = new QGroupBox(tr("Search Options"));
    auto* optionsLayout = new QVBoxLayout(m_optionsGroup);

    // Basic options
    m_caseSensitiveCheck = new QCheckBox(tr("Case Sensitive"));
    m_wholeWordsCheck = new QCheckBox(tr("Whole Words"));
    m_regexCheck = new QCheckBox(tr("Regular Expression"));
    m_searchBackwardCheck = new QCheckBox(tr("Search Backward"));

    optionsLayout->addWidget(m_caseSensitiveCheck);
    optionsLayout->addWidget(m_wholeWordsCheck);
    optionsLayout->addWidget(m_regexCheck);
    optionsLayout->addWidget(m_searchBackwardCheck);

    // Advanced options
    m_fuzzySearchCheck = new QCheckBox(tr("Fuzzy Search"));
    m_fuzzyThresholdLabel = new QLabel(tr("Fuzzy Threshold:"));
    m_fuzzyThresholdSpin = new QSpinBox();
    m_fuzzyThresholdSpin->setRange(1, 5);
    m_fuzzyThresholdSpin->setValue(2);
    m_fuzzyThresholdSpin->setEnabled(false);

    auto* fuzzyLayout = new QHBoxLayout();
    fuzzyLayout->addWidget(m_fuzzySearchCheck);
    fuzzyLayout->addWidget(m_fuzzyThresholdLabel);
    fuzzyLayout->addWidget(m_fuzzyThresholdSpin);
    fuzzyLayout->addStretch();

    optionsLayout->addLayout(fuzzyLayout);

    // Page range options
    m_pageRangeGroup = new QGroupBox(tr("Page Range"));
    auto* pageRangeLayout = new QVBoxLayout(m_pageRangeGroup);

    m_pageRangeCheck = new QCheckBox(tr("Limit Search Range"));
    m_pageRangeLabel = new QLabel(tr("From Page:"));
    m_startPageSpin = new QSpinBox();
    m_startPageSpin->setMinimum(1);
    m_startPageSpin->setEnabled(false);

    auto* toLabel = new QLabel(tr("To Page:"));
    m_endPageSpin = new QSpinBox();
    m_endPageSpin->setMinimum(1);
    m_endPageSpin->setEnabled(false);

    auto* rangeLayout = new QHBoxLayout();
    rangeLayout->addWidget(m_pageRangeLabel);
    rangeLayout->addWidget(m_startPageSpin);
    rangeLayout->addWidget(toLabel);
    rangeLayout->addWidget(m_endPageSpin);
    rangeLayout->addStretch();

    pageRangeLayout->addWidget(m_pageRangeCheck);
    pageRangeLayout->addLayout(rangeLayout);

    optionsLayout->addWidget(m_pageRangeGroup);

    // Results view
    m_resultsView = new QListView();
    m_resultsView->setModel(m_searchModel);
    m_resultsView->setAlternatingRowColors(true);
    m_resultsView->setSelectionMode(QAbstractItemView::SingleSelection);

    // Status and progress
    m_statusLabel = new QLabel(tr("Ready to search"));
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);

    // Enhanced UI elements
    m_searchProgressLabel = new QLabel(tr("Search Progress:"));
    m_searchProgressBar = new QProgressBar();
    m_searchProgressBar->setVisible(false);

    // Highlight color controls
    auto* colorLayout = new QHBoxLayout();
    m_highlightColorButton = new QPushButton(tr("Highlight Color"));
    m_highlightColorButton->setStyleSheet(
        "background-color: #FFFF00; color: black;");
    m_currentHighlightColorButton = new QPushButton(tr("Current Result Color"));
    m_currentHighlightColorButton->setStyleSheet(
        "background-color: #FF6600; color: white;");

    colorLayout->addWidget(new QLabel(tr("Highlight Colors:")));
    colorLayout->addWidget(m_highlightColorButton);
    colorLayout->addWidget(m_currentHighlightColorButton);
    colorLayout->addStretch();

    // Add to main layout
    m_mainLayout->addLayout(m_searchLayout);
    m_mainLayout->addLayout(m_navigationLayout);
    m_mainLayout->addWidget(m_optionsGroup);
    m_mainLayout->addLayout(colorLayout);
    m_mainLayout->addWidget(m_resultsView);
    m_mainLayout->addWidget(m_statusLabel);
    m_mainLayout->addWidget(m_progressBar);
    m_mainLayout->addWidget(m_searchProgressLabel);
    m_mainLayout->addWidget(m_searchProgressBar);
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

    // Search history
    connect(m_searchHistoryCombo,
            QOverload<const QString&>::of(&QComboBox::currentTextChanged), this,
            &SearchWidget::onSearchHistorySelected);
    connect(m_clearHistoryButton, &QPushButton::clicked, this,
            &SearchWidget::onClearHistoryClicked);

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

    // Advanced search options
    connect(m_fuzzySearchCheck, &QCheckBox::toggled, this,
            &SearchWidget::onFuzzySearchToggled);
    connect(m_pageRangeCheck, &QCheckBox::toggled, this,
            &SearchWidget::onPageRangeToggled);
    connect(m_startPageSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &SearchWidget::onPageRangeChanged);
    connect(m_endPageSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &SearchWidget::onPageRangeChanged);

    // Highlight color controls
    connect(m_highlightColorButton, &QPushButton::clicked, this,
            &SearchWidget::onHighlightColorClicked);
    connect(m_currentHighlightColorButton, &QPushButton::clicked, this,
            &SearchWidget::onCurrentHighlightColorClicked);

    // Search model signals
    connect(m_searchModel, &SearchModel::searchStarted, this,
            &SearchWidget::onSearchStarted);
    connect(m_searchModel, &SearchModel::searchFinished, this,
            &SearchWidget::onSearchFinished);
    connect(m_searchModel, &SearchModel::searchError, this,
            &SearchWidget::onSearchError);
    connect(m_searchModel, &SearchModel::currentResultChanged, this,
            &SearchWidget::onCurrentResultChanged);

    // Real-time search progress signals
    connect(m_searchModel, &SearchModel::realTimeSearchProgress, this,
            &SearchWidget::updateSearchProgress);

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
    // Cancel any ongoing search before changing document
    cancelCurrentSearch();

    m_document = document;

    // Update page range controls based on document
    if (document != nullptr) {
        int pageCount = document->numPages();
        LOG_INFO("Document loaded with {} pages", pageCount);

        m_startPageSpin->setMaximum(pageCount);
        m_endPageSpin->setMaximum(pageCount);
        m_startPageSpin->setValue(1);
        m_endPageSpin->setValue(pageCount);

        // Enable search functionality
        m_searchInput->setEnabled(true);
        m_searchButton->setEnabled(true);

        // Optimize search performance based on document size
        optimizeSearchPerformance();

        m_statusLabel->setText(tr("Ready to search %1 pages").arg(pageCount));

    } else {
        LOG_INFO("Document cleared");

        // Disable search functionality when no document
        m_searchInput->setEnabled(false);
        m_searchButton->setEnabled(false);

        m_statusLabel->setText(tr("No document loaded"));
    }

    clearSearch();
    loadSearchHistory();
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
    m_statusLabel->setText(tr("Ready to search"));
}

void SearchWidget::showSearchOptions(bool show) {
    m_optionsVisible = show;
    m_optionsGroup->setVisible(show);
    m_optionsButton->setChecked(show);
}

bool SearchWidget::hasResults() const {
    return m_searchModel->rowCount(QModelIndex()) > 0;
}

int SearchWidget::getResultCount() const {
    return m_searchModel->rowCount(QModelIndex());
}

SearchResult SearchWidget::getCurrentResult() const {
    int currentIndex = m_searchModel->getCurrentResultIndex();
    if (currentIndex >= 0) {
        return m_searchModel->getResult(currentIndex);
    }
    return SearchResult();
}

void SearchWidget::performSearch() {
    QString query = m_searchInput->text().trimmed();

    // Validate input
    if (!validateSearchInput(query)) {
        if (query.isEmpty()) {
            showSearchError(tr("Please enter a search term"));
        } else if (query.length() > 1000) {
            showSearchError(
                tr("Search term is too long (maximum 1000 characters)"));
        } else if (m_regexCheck->isChecked()) {
            QRegularExpression regex(query);
            if (!regex.isValid()) {
                showSearchError(tr("Invalid regular expression: %1")
                                    .arg(regex.errorString()));
            }
        } else if (m_pageRangeCheck->isChecked() &&
                   m_startPageSpin->value() > m_endPageSpin->value()) {
            showSearchError(
                tr("Invalid page range: start page must be less than or equal "
                   "to end page"));
        }
        return;
    }

    if (m_document == nullptr) {
        showSearchError(tr("No document loaded"));
        return;
    }

    LOG_INFO("Starting search for query: '{}'", query.toStdString());

    // Cancel any existing search
    cancelCurrentSearch();

    SearchOptions options = getSearchOptions();

    // Optimize performance based on document size
    optimizeSearchPerformance();

    // Update search history
    updateSearchHistory();

    // Save current settings
    saveSearchHistoryToSettings();

    // Choose appropriate search method based on options
    try {
        if (options.fuzzySearch) {
            LOG_DEBUG("Starting fuzzy search with threshold: {}",
                      options.fuzzyThreshold);
            m_searchModel->startFuzzySearch(m_document, query, options);
        } else if (options.startPage >= 0 && options.endPage >= 0) {
            LOG_DEBUG("Starting page range search: pages {}-{}",
                      options.startPage + 1, options.endPage + 1);
            m_searchModel->startPageRangeSearch(
                m_document, query, options.startPage, options.endPage, options);
        } else {
            LOG_DEBUG("Starting standard search");
            m_searchModel->startSearch(m_document, query, options);
        }

        emit searchRequested(query, options);

    } catch (const std::exception& e) {
        LOG_ERROR("Search failed with exception: {}", e.what());
        showSearchError(tr("Search failed: %1").arg(e.what()));
    }
}

void SearchWidget::performRealTimeSearch() {
    QString query = m_searchInput->text().trimmed();

    // Validate input for real-time search
    if (!validateSearchInput(query)) {
        if (!query.isEmpty()) {
            LOG_DEBUG("Real-time search skipped due to invalid input");
        }
        return;
    }

    if (m_document == nullptr) {
        LOG_DEBUG("Real-time search skipped - no document loaded");
        return;
    }

    LOG_DEBUG("Starting real-time search for query: '{}'", query.toStdString());

    // Cancel any existing search
    if (m_searchModel->isSearching()) {
        m_searchModel->cancelSearch();
    }

    SearchOptions options = getSearchOptions();

    // Limit results for real-time search performance
    options.maxResults = qMin(options.maxResults, 100);

    try {
        m_searchModel->startRealTimeSearch(m_document, query, options);
        emit searchRequested(query, options);

    } catch (const std::exception& e) {
        LOG_ERROR("Real-time search failed: {}", e.what());
        // Don't show error for real-time search to avoid UI spam
    }
}

void SearchWidget::nextResult() {
    if (!m_searchModel->hasNext()) {
        LOG_DEBUG("No next result available");
        m_statusLabel->setText(tr("No more results"));
        return;
    }

    SearchResult result = m_searchModel->nextResult();

    if (!result.isValid()) {
        LOG_WARNING("Invalid search result returned");
        showSearchError(tr("Invalid search result"));
        return;
    }

    LOG_DEBUG("Navigating to next result: page {}, position {}",
              result.pageNumber + 1, result.textPosition);

    // Update UI with current result info
    updateResultsInfo();
    updateNavigationButtons();

    // Update results view selection
    int currentIndex = m_searchModel->getCurrentResultIndex();
    if (currentIndex >= 0 && currentIndex < m_searchModel->rowCount()) {
        QModelIndex modelIndex = m_searchModel->index(currentIndex);
        m_resultsView->setCurrentIndex(modelIndex);
        m_resultsView->scrollTo(modelIndex, QAbstractItemView::EnsureVisible);
    }

    // Navigate to result with enhanced highlighting
    emit navigateToResult(result.pageNumber, result.boundingRect);
    emit resultSelected(result);

    // Update status with current position and context
    int totalResults = static_cast<int>(m_searchModel->getResults().size());
    m_statusLabel->setText(tr("Result %1 / %2: %3")
                               .arg(currentIndex + 1)
                               .arg(totalResults)
                               .arg(result.contextText.left(50)));
    m_statusLabel->setStyleSheet("");  // Clear any error styling
}

void SearchWidget::previousResult() {
    if (!m_searchModel->hasPrevious()) {
        LOG_DEBUG("No previous result available");
        m_statusLabel->setText(tr("No previous results"));
        return;
    }

    SearchResult result = m_searchModel->previousResult();

    if (!result.isValid()) {
        LOG_WARNING("Invalid search result returned");
        showSearchError(tr("Invalid search result"));
        return;
    }

    LOG_DEBUG("Navigating to previous result: page {}, position {}",
              result.pageNumber + 1, result.textPosition);

    // Update UI with current result info
    updateResultsInfo();
    updateNavigationButtons();

    // Update results view selection
    int currentIndex = m_searchModel->getCurrentResultIndex();
    if (currentIndex >= 0 && currentIndex < m_searchModel->rowCount()) {
        QModelIndex modelIndex = m_searchModel->index(currentIndex);
        m_resultsView->setCurrentIndex(modelIndex);
        m_resultsView->scrollTo(modelIndex, QAbstractItemView::EnsureVisible);
    }

    // Navigate to result with enhanced highlighting
    emit navigateToResult(result.pageNumber, result.boundingRect);
    emit resultSelected(result);

    // Update status with current position and context
    int totalResults = static_cast<int>(m_searchModel->getResults().size());
    m_statusLabel->setText(tr("Result %1 / %2: %3")
                               .arg(currentIndex + 1)
                               .arg(totalResults)
                               .arg(result.contextText.left(50)));
    m_statusLabel->setStyleSheet("");  // Clear any error styling
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
    if (!query.isEmpty() && m_document != nullptr) {
        m_searchTimer->start();
    } else {
        // Clear search results when input is empty
        clearSearch();
        emit searchCleared();
    }
}

void SearchWidget::onSearchStarted() {
    setSearchInProgress(true);
    showSearchProgress(true);
    m_statusLabel->setText(tr("Searching..."));
}

void SearchWidget::onSearchFinished(int resultCount) {
    setSearchInProgress(false);
    showSearchProgress(false);
    updateNavigationButtons();
    updateResultsInfo();

    LOG_INFO("Search completed with {} results", resultCount);

    if (resultCount > 0) {
        m_statusLabel->setText(tr("Found %1 results").arg(resultCount));
        m_statusLabel->setStyleSheet("");  // Clear any error styling

        // Auto-navigate to first result with enhanced feedback
        if (m_searchModel->getCurrentResultIndex() >= 0) {
            SearchResult result = m_searchModel->getResult(0);
            if (result.isValid()) {
                LOG_DEBUG("Auto-navigating to first result on page {}",
                          result.pageNumber + 1);

                // Update results view selection
                QModelIndex firstIndex = m_searchModel->index(0);
                m_resultsView->setCurrentIndex(firstIndex);
                m_resultsView->scrollTo(firstIndex,
                                        QAbstractItemView::EnsureVisible);

                emit navigateToResult(result.pageNumber, result.boundingRect);
                emit resultSelected(result);

                // Show context in status
                m_statusLabel->setText(tr("Found %1 results - showing: %2")
                                           .arg(resultCount)
                                           .arg(result.contextText.left(50)));
            }
        }

        // Expand results view if it was collapsed
        if (!m_resultsView->isVisible()) {
            m_resultsView->setVisible(true);
        }

    } else {
        m_statusLabel->setText(tr("No matching results found"));
        m_statusLabel->setStyleSheet(
            "color: #888888;");  // Gray text for no results

        // Collapse results view when no results
        m_resultsView->setVisible(false);

        LOG_DEBUG("No search results found");
    }

    // Save successful search to history
    if (resultCount > 0) {
        QString query = m_searchInput->text().trimmed();
        if (!query.isEmpty()) {
            m_searchModel->addToSearchHistory(query);
            updateSearchHistory();
            saveSearchHistoryToSettings();
        }
    }
}

void SearchWidget::onSearchError(const QString& error) {
    setSearchInProgress(false);
    m_statusLabel->setText(tr("Search error: %1").arg(error));
    QMessageBox::warning(this, tr("Search Error"), error);
}

void SearchWidget::onCurrentResultChanged(int index) {
    updateNavigationButtons();
    updateResultsInfo();

    // Update selection in results view
    if (index >= 0 && index < m_searchModel->rowCount(QModelIndex())) {
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
    int total = m_searchModel->rowCount(QModelIndex());

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

    // Advanced options
    options.fuzzySearch = m_fuzzySearchCheck->isChecked();
    options.fuzzyThreshold = m_fuzzyThresholdSpin->value();

    // Page range options
    if (m_pageRangeCheck->isChecked()) {
        options.startPage = m_startPageSpin->value() - 1;  // Convert to 0-based
        options.endPage = m_endPageSpin->value() - 1;      // Convert to 0-based
    } else {
        options.startPage = -1;  // Search all pages
        options.endPage = -1;
    }

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
    m_statusLabel->setText(tr("Real-time searching..."));
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
        tr("Search progress: %1/%2 pages").arg(currentPage).arg(totalPages));
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

void SearchWidget::retranslateUi() {
    // Update all UI text with new translations
    m_searchHistoryCombo->lineEdit()->setPlaceholderText(
        tr("Search document content..."));
    m_searchButton->setText(tr("Search"));
    m_clearHistoryButton->setText(tr("Clear History"));
    m_clearHistoryButton->setToolTip(tr("Clear search history"));
    m_optionsButton->setText(tr("Options"));
    m_closeButton->setToolTip(tr("Close search"));

    m_previousButton->setText(tr("Previous"));
    m_nextButton->setText(tr("Next"));

    m_optionsGroup->setTitle(tr("Search Options"));
    m_caseSensitiveCheck->setText(tr("Case Sensitive"));
    m_wholeWordsCheck->setText(tr("Whole Words"));
    m_regexCheck->setText(tr("Regular Expression"));
    m_searchBackwardCheck->setText(tr("Search Backward"));

    m_fuzzySearchCheck->setText(tr("Fuzzy Search"));
    m_fuzzyThresholdLabel->setText(tr("Fuzzy Threshold:"));

    m_pageRangeGroup->setTitle(tr("Page Range"));
    m_pageRangeCheck->setText(tr("Limit Search Range"));
    m_pageRangeLabel->setText(tr("From Page:"));
    // Find "To Page:" label and update it
    auto* toLabel = m_pageRangeGroup->findChild<QLabel*>("toPageLabel");
    if (toLabel != nullptr) {
        toLabel->setText(tr("To Page:"));
    }

    m_statusLabel->setText(tr("Ready to search"));
    m_searchProgressLabel->setText(tr("Search Progress:"));

    m_highlightColorButton->setText(tr("Highlight Color"));
    m_currentHighlightColorButton->setText(tr("Current Result Color"));

    // Update highlight color label if it exists
    auto* colorLabel = findChild<QLabel*>("highlightColorsLabel");
    if (colorLabel != nullptr) {
        colorLabel->setText(tr("Highlight Colors:"));
    }

    updateResultsInfo();
}

void SearchWidget::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}

void SearchWidget::contextMenuEvent(QContextMenuEvent* event) {
    if (!contextMenuManager) {
        QWidget::contextMenuEvent(event);
        return;
    }

    // Create UI element context
    ContextMenuManager::UIElementContext context;
    context.targetWidget = this;
    context.elementIndex = -1;
    context.isEnabled = isEnabled();
    context.isVisible = isVisible();
    context.elementId = "searchWidget";

    // Add search-specific properties
    context.properties["hasResults"] = hasResults();
    context.properties["resultCount"] = getResultCount();
    context.properties["searchText"] =
        m_searchInput ? m_searchInput->text() : QString();

    // Show search widget context menu
    contextMenuManager->showSearchMenu(event->globalPos(), context, this);

    event->accept();
}

// Advanced search feature implementations
void SearchWidget::onFuzzySearchToggled(bool enabled) {
    m_fuzzyThresholdSpin->setEnabled(enabled);
    m_fuzzyThresholdLabel->setEnabled(enabled);
}

void SearchWidget::onPageRangeToggled(bool enabled) {
    m_startPageSpin->setEnabled(enabled);
    m_endPageSpin->setEnabled(enabled);
    m_pageRangeLabel->setEnabled(enabled);

    if (enabled && m_document != nullptr) {
        int pageCount = m_document->numPages();
        m_startPageSpin->setMaximum(pageCount);
        m_endPageSpin->setMaximum(pageCount);
        m_endPageSpin->setValue(pageCount);
    }
}

void SearchWidget::onPageRangeChanged() {
    // Ensure start page is not greater than end page
    if (m_startPageSpin->value() > m_endPageSpin->value()) {
        m_endPageSpin->setValue(m_startPageSpin->value());
    }
}

void SearchWidget::onSearchHistorySelected(const QString& query) {
    if (!query.isEmpty() && query != m_searchInput->text()) {
        m_searchInput->setText(query);
        // Optionally trigger search immediately
        performSearch();
    }
}

void SearchWidget::onClearHistoryClicked() {
    m_searchModel->clearSearchHistory();
    updateSearchHistory();
}

// Advanced search feature methods
void SearchWidget::setFuzzySearchEnabled(bool enabled) {
    m_fuzzySearchCheck->setChecked(enabled);
    onFuzzySearchToggled(enabled);
}

void SearchWidget::setPageRangeEnabled(bool enabled) {
    m_pageRangeCheck->setChecked(enabled);
    onPageRangeToggled(enabled);
}

void SearchWidget::setPageRange(int startPage, int endPage) {
    m_startPageSpin->setValue(startPage +
                              1);          // Convert from 0-based to 1-based
    m_endPageSpin->setValue(endPage + 1);  // Convert from 0-based to 1-based
}

void SearchWidget::updateSearchHistory() {
    m_searchHistoryCombo->clear();
    QStringList history = m_searchModel->getSearchHistory();
    m_searchHistoryCombo->addItems(history);
}

void SearchWidget::loadSearchHistory() { updateSearchHistory(); }

// Enhanced UI feature implementations
void SearchWidget::setHighlightColors(const QColor& normalColor,
                                      const QColor& currentColor) {
    // Update button styles to show current colors
    m_highlightColorButton->setStyleSheet(
        QString("background-color: %1; color: %2;")
            .arg(normalColor.name())
            .arg(normalColor.lightness() > 128 ? "black" : "white"));

    m_currentHighlightColorButton->setStyleSheet(
        QString("background-color: %1; color: %2;")
            .arg(currentColor.name())
            .arg(currentColor.lightness() > 128 ? "black" : "white"));
}

void SearchWidget::showSearchProgress(bool show) {
    m_searchProgressLabel->setVisible(show);
    m_searchProgressBar->setVisible(show);
}

void SearchWidget::updateSearchProgress(int current, int total) {
    if (total > 0) {
        m_searchProgressBar->setRange(0, total);
        m_searchProgressBar->setValue(current);
        m_searchProgressLabel->setText(
            QString("搜索进度: %1/%2 页").arg(current).arg(total));
    } else {
        m_searchProgressBar->setRange(0, 0);  // Indeterminate
        m_searchProgressLabel->setText("正在搜索...");
    }
}

void SearchWidget::setSearchResultInfo(int currentResult, int totalResults) {
    if (totalResults > 0) {
        m_resultInfoLabel->setText(
            QString("%1 / %2").arg(currentResult + 1).arg(totalResults));
    } else {
        m_resultInfoLabel->setText("0 / 0");
    }
}

void SearchWidget::onHighlightColorClicked() {
    QColor currentColor = getNormalHighlightColor();

    QColor newColor = QColorDialog::getColor(
        currentColor, this, tr("Select Normal Highlight Color"));
    if (newColor.isValid()) {
        LOG_DEBUG("Normal highlight color changed to: {}",
                  newColor.name().toStdString());

        m_highlightColorButton->setStyleSheet(
            QString("background-color: %1; color: %2;")
                .arg(newColor.name())
                .arg(newColor.lightness() > 128 ? "black" : "white"));

        // Emit signal to update highlighting
        emit highlightColorsChanged(newColor, getCurrentHighlightColor());

        // Save the new color setting
        saveSearchHistoryToSettings();

        // If there are current search results, refresh highlighting
        if (m_searchModel->rowCount() > 0) {
            SearchResult currentResult = getCurrentResult();
            if (currentResult.isValid()) {
                emit resultSelected(currentResult);
            }
        }
    }
}

void SearchWidget::onCurrentHighlightColorClicked() {
    QColor currentColor = getCurrentHighlightColor();

    QColor newColor = QColorDialog::getColor(
        currentColor, this, tr("Select Current Result Highlight Color"));
    if (newColor.isValid()) {
        LOG_DEBUG("Current highlight color changed to: {}",
                  newColor.name().toStdString());

        m_currentHighlightColorButton->setStyleSheet(
            QString("background-color: %1; color: %2;")
                .arg(newColor.name())
                .arg(newColor.lightness() > 128 ? "black" : "white"));

        // Emit signal to update highlighting
        emit highlightColorsChanged(getNormalHighlightColor(), newColor);

        // Save the new color setting
        saveSearchHistoryToSettings();

        // If there are current search results, refresh highlighting
        if (m_searchModel->rowCount() > 0) {
            SearchResult currentResult = getCurrentResult();
            if (currentResult.isValid()) {
                emit resultSelected(currentResult);
            }
        }
    }
}

QColor SearchWidget::getNormalHighlightColor() const {
    QString style = m_highlightColorButton->styleSheet();
    QRegularExpression colorRegex("background-color:\\s*([^;]+);");
    QRegularExpressionMatch match = colorRegex.match(style);
    if (match.hasMatch()) {
        return QColor(match.captured(1));
    }
    return QColor("#FFFF00");  // Default yellow
}

QColor SearchWidget::getCurrentHighlightColor() const {
    QString style = m_currentHighlightColorButton->styleSheet();
    QRegularExpression colorRegex("background-color:\\s*([^;]+);");
    QRegularExpressionMatch match = colorRegex.match(style);
    if (match.hasMatch()) {
        return QColor(match.captured(1));
    }
    return QColor("#FF6600");  // Default orange
}

// Search history persistence implementation
void SearchWidget::saveSearchHistoryToSettings() {
    try {
        QSettings settings;
        settings.beginGroup("SearchWidget");

        QStringList history = m_searchModel->getSearchHistory();
        settings.setValue("searchHistory", history);

        // Save highlight colors
        settings.setValue("normalHighlightColor",
                          getNormalHighlightColor().name());
        settings.setValue("currentHighlightColor",
                          getCurrentHighlightColor().name());

        // Save search options
        settings.setValue("caseSensitive", m_caseSensitiveCheck->isChecked());
        settings.setValue("wholeWords", m_wholeWordsCheck->isChecked());
        settings.setValue("useRegex", m_regexCheck->isChecked());
        settings.setValue("searchBackward", m_searchBackwardCheck->isChecked());
        settings.setValue("fuzzySearch", m_fuzzySearchCheck->isChecked());
        settings.setValue("fuzzyThreshold", m_fuzzyThresholdSpin->value());

        settings.endGroup();
        settings.sync();

        LOG_DEBUG("Search history and settings saved successfully");
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to save search history: {}", e.what());
        showSearchError(tr("Failed to save search history: %1").arg(e.what()));
    }
}

void SearchWidget::loadSearchHistoryFromSettings() {
    try {
        QSettings settings;
        settings.beginGroup("SearchWidget");

        // Load search history
        QStringList history =
            settings.value("searchHistory", QStringList()).toStringList();
        if (!history.isEmpty()) {
            m_searchHistoryCombo->clear();
            m_searchHistoryCombo->addItems(history);

            // Update the search model's history
            for (const QString& query : history) {
                m_searchModel->addToSearchHistory(query);
            }
        }

        // Load highlight colors
        QString normalColorName =
            settings.value("normalHighlightColor", "#FFFF00").toString();
        QString currentColorName =
            settings.value("currentHighlightColor", "#FF6600").toString();
        setHighlightColors(QColor(normalColorName), QColor(currentColorName));

        // Load search options
        m_caseSensitiveCheck->setChecked(
            settings.value("caseSensitive", false).toBool());
        m_wholeWordsCheck->setChecked(
            settings.value("wholeWords", false).toBool());
        m_regexCheck->setChecked(settings.value("useRegex", false).toBool());
        m_searchBackwardCheck->setChecked(
            settings.value("searchBackward", false).toBool());
        m_fuzzySearchCheck->setChecked(
            settings.value("fuzzySearch", false).toBool());
        m_fuzzyThresholdSpin->setValue(
            settings.value("fuzzyThreshold", 2).toInt());

        settings.endGroup();

        LOG_DEBUG("Search history and settings loaded successfully");
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load search history: {}", e.what());
        showSearchError(tr("Failed to load search history: %1").arg(e.what()));
    }
}

// Search validation and error handling
bool SearchWidget::validateSearchInput(const QString& query) const {
    // Use UIErrorHandler for comprehensive validation
    auto validation = InputValidator::validateSearchQuery(
        query, false, m_regexCheck->isChecked());

    if (validation.result != UIErrorHandler::ValidationResult::Valid) {
        // Don't show feedback here as this is called during typing
        LOG_DEBUG("Search validation failed: {}",
                  validation.message.toStdString());
        return false;
    }

    // Validate page range if enabled
    if (m_pageRangeCheck->isChecked()) {
        int startPage = m_startPageSpin->value();
        int endPage = m_endPageSpin->value();
        int totalPages = m_document ? m_document->numPages() : 0;

        auto pageValidation =
            InputValidator::validatePageRange(startPage, endPage, totalPages);
        if (pageValidation.result != UIErrorHandler::ValidationResult::Valid) {
            LOG_DEBUG("Page range validation failed: {}",
                      pageValidation.message.toStdString());
            return false;
        }
    }

    LOG_DEBUG("Search input validation passed for query: {}",
              query.toStdString());
    return true;
}

void SearchWidget::showSearchError(const QString& error) {
    LOG_ERROR("Search error: {}", error.toStdString());

    // Use UIErrorHandler for consistent error display
    ErrorHandling::ErrorInfo errorInfo =
        ErrorHandling::createSearchError("Search", error);
    UIErrorHandler::instance().handleSystemError(this, errorInfo);

    // Also update status label for immediate feedback
    m_statusLabel->setText(tr("Error: %1").arg(error));
    m_statusLabel->setStyleSheet("color: " + STYLE.errorColor().name() + ";");

    // Reset style after 5 seconds
    QTimer::singleShot(5000, this,
                       [this]() { m_statusLabel->setStyleSheet(""); });
}

// Performance optimization
void SearchWidget::cancelCurrentSearch() {
    if (m_searchModel->isSearching()) {
        LOG_INFO("Cancelling current search operation");
        m_searchModel->cancelSearch();
        setSearchInProgress(false);
        m_statusLabel->setText(tr("Search cancelled"));
    }

    // Stop any pending real-time search
    if (m_searchTimer->isActive()) {
        m_searchTimer->stop();
        LOG_DEBUG("Stopped pending real-time search");
    }
}

void SearchWidget::optimizeSearchPerformance() {
    // Adjust search parameters based on document size and system performance
    if (m_document != nullptr) {
        int pageCount = m_document->numPages();

        // For large documents, increase debounce delay and limit results
        if (pageCount > 1000) {
            m_searchTimer->setInterval(500);  // Longer delay for large docs
            LOG_DEBUG("Optimized search for large document: {} pages",
                      pageCount);
        } else if (pageCount > 100) {
            m_searchTimer->setInterval(400);  // Medium delay
            LOG_DEBUG("Optimized search for medium document: {} pages",
                      pageCount);
        } else {
            m_searchTimer->setInterval(300);  // Default delay
            LOG_DEBUG(
                "Using default search timing for small document: {} pages",
                pageCount);
        }
    }
}
