#include "SearchSettingsWidget.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QSettings>
#include <QVBoxLayout>

#include "ElaComboBox.h"
#include "ElaScrollPageArea.h"
#include "ElaSpinBox.h"
#include "ElaText.h"
#include "ElaToggleSwitch.h"

SearchSettingsWidget::SearchSettingsWidget(QWidget* parent)
    : QWidget(parent), m_mainLayout(nullptr) {
    setupUi();
    loadSettings();
}

SearchSettingsWidget::~SearchSettingsWidget() = default;

void SearchSettingsWidget::setupUi() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(16);

    // Basic Options
    auto* basicArea = new ElaScrollPageArea(this);
    auto* basicLayout = new QVBoxLayout(basicArea);
    basicLayout->setContentsMargins(16, 12, 16, 12);

    auto* basicTitle = new ElaText(tr("Basic Options"), this);
    basicTitle->setTextPixelSize(14);
    basicLayout->addWidget(basicTitle);

    auto* caseRow = new QHBoxLayout();
    caseRow->addWidget(new ElaText(tr("Case sensitive by default"), this));
    caseRow->addStretch();
    m_caseSensitiveSwitch = new ElaToggleSwitch(this);
    caseRow->addWidget(m_caseSensitiveSwitch);
    basicLayout->addLayout(caseRow);

    auto* wholeRow = new QHBoxLayout();
    wholeRow->addWidget(new ElaText(tr("Whole words by default"), this));
    wholeRow->addStretch();
    m_wholeWordsSwitch = new ElaToggleSwitch(this);
    wholeRow->addWidget(m_wholeWordsSwitch);
    basicLayout->addLayout(wholeRow);

    auto* regexRow = new QHBoxLayout();
    regexRow->addWidget(new ElaText(tr("Enable regex search"), this));
    regexRow->addStretch();
    m_regexSwitch = new ElaToggleSwitch(this);
    regexRow->addWidget(m_regexSwitch);
    basicLayout->addLayout(regexRow);

    m_mainLayout->addWidget(basicArea);

    // Advanced Options
    auto* advArea = new ElaScrollPageArea(this);
    auto* advLayout = new QVBoxLayout(advArea);
    advLayout->setContentsMargins(16, 12, 16, 12);

    auto* advTitle = new ElaText(tr("Advanced Options"), this);
    advTitle->setTextPixelSize(14);
    advLayout->addWidget(advTitle);

    auto* fuzzyRow = new QHBoxLayout();
    fuzzyRow->addWidget(new ElaText(tr("Fuzzy search"), this));
    fuzzyRow->addStretch();
    m_fuzzySearchSwitch = new ElaToggleSwitch(this);
    fuzzyRow->addWidget(m_fuzzySearchSwitch);
    advLayout->addLayout(fuzzyRow);

    auto* threshRow = new QHBoxLayout();
    threshRow->addWidget(new ElaText(tr("Fuzzy threshold:"), this));
    m_fuzzyThresholdSpin = new ElaSpinBox(this);
    m_fuzzyThresholdSpin->setRange(1, 5);
    m_fuzzyThresholdSpin->setValue(2);
    threshRow->addWidget(m_fuzzyThresholdSpin);
    threshRow->addStretch();
    advLayout->addLayout(threshRow);

    auto* maxRow = new QHBoxLayout();
    maxRow->addWidget(new ElaText(tr("Max results:"), this));
    m_maxResultsSpin = new ElaSpinBox(this);
    m_maxResultsSpin->setRange(100, 10000);
    m_maxResultsSpin->setValue(1000);
    maxRow->addWidget(m_maxResultsSpin);
    maxRow->addStretch();
    advLayout->addLayout(maxRow);

    auto* ctxRow = new QHBoxLayout();
    ctxRow->addWidget(new ElaText(tr("Context length:"), this));
    m_contextLengthSpin = new ElaSpinBox(this);
    m_contextLengthSpin->setRange(20, 200);
    m_contextLengthSpin->setValue(50);
    ctxRow->addWidget(m_contextLengthSpin);
    ctxRow->addStretch();
    advLayout->addLayout(ctxRow);

    m_mainLayout->addWidget(advArea);

    // Performance
    auto* perfArea = new ElaScrollPageArea(this);
    auto* perfLayout = new QVBoxLayout(perfArea);
    perfLayout->setContentsMargins(16, 12, 16, 12);

    auto* perfTitle = new ElaText(tr("Performance"), this);
    perfTitle->setTextPixelSize(14);
    perfLayout->addWidget(perfTitle);

    auto* cacheRow = new QHBoxLayout();
    cacheRow->addWidget(new ElaText(tr("Enable search cache"), this));
    cacheRow->addStretch();
    m_cacheEnabledSwitch = new ElaToggleSwitch(this);
    m_cacheEnabledSwitch->setIsToggled(true);
    cacheRow->addWidget(m_cacheEnabledSwitch);
    perfLayout->addLayout(cacheRow);

    auto* memRow = new QHBoxLayout();
    memRow->addWidget(new ElaText(tr("Max cache (MB):"), this));
    m_maxCacheMemorySpin = new ElaSpinBox(this);
    m_maxCacheMemorySpin->setRange(50, 500);
    m_maxCacheMemorySpin->setValue(100);
    memRow->addWidget(m_maxCacheMemorySpin);
    memRow->addStretch();
    perfLayout->addLayout(memRow);

    auto* incRow = new QHBoxLayout();
    incRow->addWidget(new ElaText(tr("Incremental search"), this));
    incRow->addStretch();
    m_incrementalSearchSwitch = new ElaToggleSwitch(this);
    m_incrementalSearchSwitch->setIsToggled(true);
    incRow->addWidget(m_incrementalSearchSwitch);
    perfLayout->addLayout(incRow);

    auto* delayRow = new QHBoxLayout();
    delayRow->addWidget(new ElaText(tr("Search delay (ms):"), this));
    m_incrementalDelaySpin = new ElaSpinBox(this);
    m_incrementalDelaySpin->setRange(100, 1000);
    m_incrementalDelaySpin->setValue(300);
    delayRow->addWidget(m_incrementalDelaySpin);
    delayRow->addStretch();
    perfLayout->addLayout(delayRow);

    auto* timeoutRow = new QHBoxLayout();
    timeoutRow->addWidget(new ElaText(tr("Timeout (sec):"), this));
    m_searchTimeoutSpin = new ElaSpinBox(this);
    m_searchTimeoutSpin->setRange(5, 120);
    m_searchTimeoutSpin->setValue(30);
    timeoutRow->addWidget(m_searchTimeoutSpin);
    timeoutRow->addStretch();
    perfLayout->addLayout(timeoutRow);

    auto* bgRow = new QHBoxLayout();
    bgRow->addWidget(new ElaText(tr("Background processing"), this));
    bgRow->addStretch();
    m_backgroundProcessingSwitch = new ElaToggleSwitch(this);
    m_backgroundProcessingSwitch->setIsToggled(true);
    bgRow->addWidget(m_backgroundProcessingSwitch);
    perfLayout->addLayout(bgRow);

    auto* threadRow = new QHBoxLayout();
    threadRow->addWidget(new ElaText(tr("Max threads:"), this));
    m_maxThreadsSpin = new ElaSpinBox(this);
    m_maxThreadsSpin->setRange(1, 8);
    m_maxThreadsSpin->setValue(4);
    threadRow->addWidget(m_maxThreadsSpin);
    threadRow->addStretch();
    perfLayout->addLayout(threadRow);

    m_mainLayout->addWidget(perfArea);

    // Display
    auto* dispArea = new ElaScrollPageArea(this);
    auto* dispLayout = new QVBoxLayout(dispArea);
    dispLayout->setContentsMargins(16, 12, 16, 12);

    auto* dispTitle = new ElaText(tr("Display"), this);
    dispTitle->setTextPixelSize(14);
    dispLayout->addWidget(dispTitle);

    auto* colorRow = new QHBoxLayout();
    colorRow->addWidget(new ElaText(tr("Highlight color:"), this));
    m_highlightColorCombo = new ElaComboBox(this);
    m_highlightColorCombo->addItem(tr("Yellow"), "#FFFF00");
    m_highlightColorCombo->addItem(tr("Green"), "#00FF00");
    m_highlightColorCombo->addItem(tr("Cyan"), "#00FFFF");
    colorRow->addWidget(m_highlightColorCombo);
    colorRow->addStretch();
    dispLayout->addLayout(colorRow);

    auto* showCtxRow = new QHBoxLayout();
    showCtxRow->addWidget(new ElaText(tr("Show context"), this));
    showCtxRow->addStretch();
    m_showContextSwitch = new ElaToggleSwitch(this);
    m_showContextSwitch->setIsToggled(true);
    showCtxRow->addWidget(m_showContextSwitch);
    dispLayout->addLayout(showCtxRow);

    auto* hlAllRow = new QHBoxLayout();
    hlAllRow->addWidget(new ElaText(tr("Highlight all matches"), this));
    hlAllRow->addStretch();
    m_highlightAllMatchesSwitch = new ElaToggleSwitch(this);
    m_highlightAllMatchesSwitch->setIsToggled(true);
    hlAllRow->addWidget(m_highlightAllMatchesSwitch);
    dispLayout->addLayout(hlAllRow);

    m_mainLayout->addWidget(dispArea);
    m_mainLayout->addStretch();

    connect(m_fuzzySearchSwitch, &ElaToggleSwitch::toggled, this,
            &SearchSettingsWidget::onFuzzySearchToggled);
    connect(m_cacheEnabledSwitch, &ElaToggleSwitch::toggled, this,
            &SearchSettingsWidget::onCacheToggled);
    connect(m_incrementalSearchSwitch, &ElaToggleSwitch::toggled, this,
            &SearchSettingsWidget::onIncrementalSearchToggled);
}

void SearchSettingsWidget::loadSettings() {
    QSettings s("SAST", "Readium");
    s.beginGroup("Search");
    m_caseSensitiveSwitch->setIsToggled(
        s.value("case_sensitive", false).toBool());
    m_wholeWordsSwitch->setIsToggled(s.value("whole_words", false).toBool());
    m_regexSwitch->setIsToggled(s.value("regex", false).toBool());
    m_fuzzySearchSwitch->setIsToggled(s.value("fuzzy", false).toBool());
    m_fuzzyThresholdSpin->setValue(s.value("fuzzy_threshold", 2).toInt());
    m_maxResultsSpin->setValue(s.value("max_results", 1000).toInt());
    m_contextLengthSpin->setValue(s.value("context_length", 50).toInt());
    m_cacheEnabledSwitch->setIsToggled(s.value("cache", true).toBool());
    m_maxCacheMemorySpin->setValue(s.value("cache_memory", 100).toInt());
    m_incrementalSearchSwitch->setIsToggled(
        s.value("incremental", true).toBool());
    m_incrementalDelaySpin->setValue(s.value("delay", 300).toInt());
    m_searchTimeoutSpin->setValue(s.value("timeout", 30).toInt());
    m_backgroundProcessingSwitch->setIsToggled(
        s.value("background", true).toBool());
    m_maxThreadsSpin->setValue(s.value("threads", 4).toInt());
    int idx =
        m_highlightColorCombo->findData(s.value("highlight_color", "#FFFF00"));
    if (idx >= 0)
        m_highlightColorCombo->setCurrentIndex(idx);
    m_showContextSwitch->setIsToggled(s.value("show_context", true).toBool());
    m_highlightAllMatchesSwitch->setIsToggled(
        s.value("highlight_all", true).toBool());
    s.endGroup();
    updateControlsState();
}

void SearchSettingsWidget::saveSettings() {
    QSettings s("SAST", "Readium");
    s.beginGroup("Search");
    s.setValue("case_sensitive", m_caseSensitiveSwitch->getIsToggled());
    s.setValue("whole_words", m_wholeWordsSwitch->getIsToggled());
    s.setValue("regex", m_regexSwitch->getIsToggled());
    s.setValue("fuzzy", m_fuzzySearchSwitch->getIsToggled());
    s.setValue("fuzzy_threshold", m_fuzzyThresholdSpin->value());
    s.setValue("max_results", m_maxResultsSpin->value());
    s.setValue("context_length", m_contextLengthSpin->value());
    s.setValue("cache", m_cacheEnabledSwitch->getIsToggled());
    s.setValue("cache_memory", m_maxCacheMemorySpin->value());
    s.setValue("incremental", m_incrementalSearchSwitch->getIsToggled());
    s.setValue("delay", m_incrementalDelaySpin->value());
    s.setValue("timeout", m_searchTimeoutSpin->value());
    s.setValue("background", m_backgroundProcessingSwitch->getIsToggled());
    s.setValue("threads", m_maxThreadsSpin->value());
    s.setValue("highlight_color",
               m_highlightColorCombo->currentData().toString());
    s.setValue("show_context", m_showContextSwitch->getIsToggled());
    s.setValue("highlight_all", m_highlightAllMatchesSwitch->getIsToggled());
    s.endGroup();
    emit settingsChanged();
}

void SearchSettingsWidget::resetToDefaults() {
    m_caseSensitiveSwitch->setIsToggled(false);
    m_wholeWordsSwitch->setIsToggled(false);
    m_regexSwitch->setIsToggled(false);
    m_fuzzySearchSwitch->setIsToggled(false);
    m_fuzzyThresholdSpin->setValue(2);
    m_maxResultsSpin->setValue(1000);
    m_contextLengthSpin->setValue(50);
    m_cacheEnabledSwitch->setIsToggled(true);
    m_maxCacheMemorySpin->setValue(100);
    m_incrementalSearchSwitch->setIsToggled(true);
    m_incrementalDelaySpin->setValue(300);
    m_searchTimeoutSpin->setValue(30);
    m_backgroundProcessingSwitch->setIsToggled(true);
    m_maxThreadsSpin->setValue(4);
    m_highlightColorCombo->setCurrentIndex(0);
    m_showContextSwitch->setIsToggled(true);
    m_highlightAllMatchesSwitch->setIsToggled(true);
    updateControlsState();
    emit settingsChanged();
}

void SearchSettingsWidget::onFuzzySearchToggled(bool enabled) {
    m_fuzzyThresholdSpin->setEnabled(enabled);
    emit settingsChanged();
}

void SearchSettingsWidget::onCacheToggled(bool enabled) {
    m_maxCacheMemorySpin->setEnabled(enabled);
    emit settingsChanged();
}

void SearchSettingsWidget::onIncrementalSearchToggled(bool enabled) {
    m_incrementalDelaySpin->setEnabled(enabled);
    emit settingsChanged();
}

void SearchSettingsWidget::updateControlsState() {
    m_fuzzyThresholdSpin->setEnabled(m_fuzzySearchSwitch->getIsToggled());
    m_maxCacheMemorySpin->setEnabled(m_cacheEnabledSwitch->getIsToggled());
    m_incrementalDelaySpin->setEnabled(
        m_incrementalSearchSwitch->getIsToggled());
    m_maxThreadsSpin->setEnabled(m_backgroundProcessingSwitch->getIsToggled());
}

void SearchSettingsWidget::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange)
        retranslateUi();
    QWidget::changeEvent(event);
}

void SearchSettingsWidget::retranslateUi() {}
