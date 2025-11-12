#include "SearchPanel.h"

// ElaWidgetTools
#include "ElaCheckBox.h"
#include "ElaLineEdit.h"
#include "ElaListView.h"
#include "ElaPushButton.h"

// Qt
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

// Logging
#include "logging/SimpleLogging.h"

// Business logic
#include "search/SearchEngine.h"

SearchPanel::SearchPanel(QWidget* parent)
    : ElaWidget(parent), m_searchEngine(nullptr), m_currentResultIndex(-1) {
    SLOG_INFO("SearchPanel: Constructor started");

    setupUi();
    connectSignals();

    SLOG_INFO("SearchPanel: Constructor completed");
}

SearchPanel::~SearchPanel() { SLOG_INFO("SearchPanel: Destructor called"); }

void SearchPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // 搜索输入区域
    auto* searchLayout = new QHBoxLayout();

    m_searchInput = new ElaLineEdit(this);
    m_searchInput->setPlaceholderText(tr("Search..."));
    searchLayout->addWidget(m_searchInput);

    m_searchBtn = new ElaPushButton(tr("Search"), this);
    searchLayout->addWidget(m_searchBtn);

    m_clearBtn = new ElaPushButton(tr("Clear"), this);
    searchLayout->addWidget(m_clearBtn);

    mainLayout->addLayout(searchLayout);

    // 搜索选项
    auto* optionsLayout = new QHBoxLayout();

    m_caseSensitiveCheck = new ElaCheckBox(tr("Case sensitive"), this);
    optionsLayout->addWidget(m_caseSensitiveCheck);

    m_wholeWordsCheck = new ElaCheckBox(tr("Whole words"), this);
    optionsLayout->addWidget(m_wholeWordsCheck);

    m_regexCheck = new ElaCheckBox(tr("Regular expression"), this);
    optionsLayout->addWidget(m_regexCheck);

    optionsLayout->addStretch();

    mainLayout->addLayout(optionsLayout);

    // 结果导航
    auto* navigationLayout = new QHBoxLayout();

    m_prevBtn = new ElaPushButton(tr("Previous"), this);
    m_prevBtn->setEnabled(false);
    navigationLayout->addWidget(m_prevBtn);

    m_nextBtn = new ElaPushButton(tr("Next"), this);
    m_nextBtn->setEnabled(false);
    navigationLayout->addWidget(m_nextBtn);

    navigationLayout->addStretch();

    mainLayout->addLayout(navigationLayout);

    // 结果列表
    m_resultsList = new ElaListView(this);
    mainLayout->addWidget(m_resultsList);
}

void SearchPanel::connectSignals() {
    connect(m_searchBtn, &ElaPushButton::clicked, this, [this]() {
        QString query = m_searchInput->text();
        if (!query.isEmpty()) {
            emit searchRequested(query, m_caseSensitiveCheck->isChecked(),
                                 m_wholeWordsCheck->isChecked(),
                                 m_regexCheck->isChecked());
        }
    });

    connect(m_searchInput, &ElaLineEdit::returnPressed, this, [this]() {
        QString query = m_searchInput->text();
        if (!query.isEmpty()) {
            emit searchRequested(query, m_caseSensitiveCheck->isChecked(),
                                 m_wholeWordsCheck->isChecked(),
                                 m_regexCheck->isChecked());
        }
    });

    connect(m_clearBtn, &ElaPushButton::clicked, this,
            &SearchPanel::clearResults);

    connect(m_prevBtn, &ElaPushButton::clicked, this,
            &SearchPanel::previousResultRequested);
    connect(m_nextBtn, &ElaPushButton::clicked, this,
            &SearchPanel::nextResultRequested);

    connect(m_resultsList, &ElaListView::clicked, this,
            [this](const QModelIndex& index) {
                if (index.isValid()) {
                    int pageNumber = index.data(Qt::UserRole).toInt();
                    int resultIndex = index.row();
                    emit resultSelected(pageNumber, resultIndex);
                }
            });
}

void SearchPanel::startSearch(const QString& query) {
    SLOG_INFO_F("SearchPanel: Starting search: {}", query.toStdString());
    clearResults();
}

void SearchPanel::stopSearch() { SLOG_INFO("SearchPanel: Stopping search"); }

void SearchPanel::clearResults() {
    SLOG_INFO("SearchPanel: Clearing results");
    if (m_resultsList->model()) {
        m_resultsList->model()->removeRows(0,
                                           m_resultsList->model()->rowCount());
    }
    m_results.clear();
    m_currentResultIndex = -1;
    updateResultsList();
}

void SearchPanel::setCaseSensitive(bool enabled) {
    m_caseSensitiveCheck->setChecked(enabled);
}

void SearchPanel::setWholeWords(bool enabled) {
    m_wholeWordsCheck->setChecked(enabled);
}

void SearchPanel::setRegexEnabled(bool enabled) {
    m_regexCheck->setChecked(enabled);
}

void SearchPanel::setSearchEngine(SearchEngine* engine) {
    m_searchEngine = engine;
}

void SearchPanel::updateResultsList() {
    if (m_resultsList->model()) {
        m_resultsList->model()->removeRows(0,
                                           m_resultsList->model()->rowCount());
    }

    // Note: For ElaListView, you would typically set a proper model here
    // For now, we're just clearing the list. A full implementation would
    // require creating a QStandardItemModel or custom model and populating it
    // with the search results.

    bool hasResults = !m_results.isEmpty();
    m_prevBtn->setEnabled(hasResults && m_currentResultIndex > 0);
    m_nextBtn->setEnabled(hasResults &&
                          m_currentResultIndex < m_results.size() - 1);
}

void SearchPanel::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    ElaWidget::changeEvent(event);
}

void SearchPanel::retranslateUi() {
    SLOG_INFO("SearchPanel: Retranslating UI");

    m_searchInput->setPlaceholderText(tr("Search..."));
    m_searchBtn->setText(tr("Search"));
    m_clearBtn->setText(tr("Clear"));
    m_caseSensitiveCheck->setText(tr("Case sensitive"));
    m_wholeWordsCheck->setText(tr("Whole words"));
    m_regexCheck->setText(tr("Regular expression"));
    m_prevBtn->setText(tr("Previous"));
    m_nextBtn->setText(tr("Next"));
}
