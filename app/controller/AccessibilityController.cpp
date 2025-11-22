#include "AccessibilityController.h"
#include <QApplication>
#include <QFileInfo>
#include <QPalette>
#include <QStyle>
#include <algorithm>
#include "EventBus.h"

// Accessibility event types
namespace AccessibilityEvents {
const QString SCREEN_READER_TOGGLED = "accessibility.screen_reader_toggled";
const QString HIGH_CONTRAST_TOGGLED = "accessibility.high_contrast_toggled";
const QString TTS_ENABLED = "accessibility.tts_enabled";
const QString TTS_DISABLED = "accessibility.tts_disabled";
const QString TTS_STATE_CHANGED = "accessibility.tts_state_changed";
const QString TEXT_ANNOUNCED = "accessibility.text_announced";
const QString SETTINGS_CHANGED = "accessibility.settings_changed";
}  // namespace AccessibilityEvents

AccessibilityController::AccessibilityController(AccessibilityModel* model,
                                                 QObject* parent)
    : QObject(parent),
      m_model(model),
      m_initialized(false),
      m_isAnnouncing(false),
      m_maxQueueSize(50),
      m_currentPage(0),
      m_totalPages(0),
      m_currentZoom(1.0),
      m_lastTtsState(QTextToSpeech::State::Ready),
      m_logger("AccessibilityController") {
    if (!m_model) {
        SLOG_ERROR("AccessibilityController: Model is null");
        return;
    }

    m_announcementTimer = new QTimer(this);
    m_announcementTimer->setInterval(500);
    m_announcementTimer->setSingleShot(false);
    connect(m_announcementTimer, &QTimer::timeout, this,
            &AccessibilityController::processAnnouncementQueue);

    connectModelSignals();
}

AccessibilityController::~AccessibilityController() { shutdown(); }

void AccessibilityController::initialize() {
    if (m_initialized) {
        SLOG_WARN("AccessibilityController already initialized");
        return;
    }

    SLOG_INFO("Initializing AccessibilityController");

    initializeTextToSpeech();
    connectEventBusSignals();

    if (m_model->isScreenReaderEnabled()) {
        enableScreenReader(true);
    }

    if (m_model->isHighContrastMode()) {
        applyHighContrastColors();
    }

    m_initialized = true;
    emit initialized();
    SLOG_INFO("AccessibilityController initialized successfully");
}

void AccessibilityController::shutdown() {
    if (!m_initialized) {
        return;
    }

    SLOG_INFO("Shutting down AccessibilityController");

    m_announcementTimer->stop();
    clearAnnouncementQueue();
    shutdownTextToSpeech();

    m_initialized = false;
    emit shutdownComplete();
    SLOG_INFO("AccessibilityController shutdown complete");
}

void AccessibilityController::initializeTextToSpeech() {
    if (m_tts) {
        return;
    }

    try {
        QString engine = m_model->ttsEngine();
        if (engine.isEmpty()) {
            m_tts = std::make_unique<QTextToSpeech>(this);
        } else {
            m_tts = std::make_unique<QTextToSpeech>(engine, this);
        }

        if (!m_tts) {
            SLOG_ERROR("Failed to create QTextToSpeech instance");
            return;
        }

        connect(m_tts.get(), &QTextToSpeech::stateChanged, this,
                &AccessibilityController::onTtsStateChanged);
        connect(m_tts.get(), &QTextToSpeech::errorOccurred, this,
                &AccessibilityController::onTtsErrorOccurred);

        applyTtsSettings();

        SLOG_INFO_F("Text-to-speech initialized with engine: {}",
                    m_tts->engine().isEmpty() ? "default" : m_tts->engine());

    } catch (const std::exception& e) {
        QString error =
            QString("Failed to initialize text-to-speech: %1").arg(e.what());
        SLOG_ERROR(error);
        emit errorOccurred(error);
    }
}

void AccessibilityController::shutdownTextToSpeech() {
    if (m_tts) {
        m_tts->stop(QTextToSpeech::BoundaryHint::Immediate);
        disconnect(m_tts.get(), nullptr, this, nullptr);
        m_tts.reset();
        SLOG_DEBUG("Text-to-speech shutdown");
    }
}

void AccessibilityController::recreateTextToSpeech() {
    shutdownTextToSpeech();
    initializeTextToSpeech();
}

void AccessibilityController::applyTtsSettings() {
    if (!m_tts) {
        return;
    }

    QLocale locale = m_model->ttsLocale();
    QVoice voice = m_model->ttsVoice();

    if (locale.language() != QLocale::C && m_tts->locale() != locale) {
        m_tts->setLocale(locale);
        SLOG_DEBUG_F("TTS locale set to: {}", locale.name());
    }

    if (!voice.name().isEmpty()) {
        QList<QVoice> voices = m_tts->availableVoices();
        auto it = std::find_if(
            voices.begin(), voices.end(),
            [&voice](const QVoice& v) { return v.name() == voice.name(); });
        if (it != voices.end()) {
            m_tts->setVoice(*it);
            SLOG_DEBUG_F("TTS voice set to: {}", voice.name());
        }
    }

    m_tts->setRate(m_model->ttsRate());
    m_tts->setPitch(m_model->ttsPitch());
    m_tts->setVolume(m_model->ttsVolume());

    SLOG_DEBUG_F("TTS settings applied: rate={}, pitch={}, volume={}",
                 m_model->ttsRate(), m_model->ttsPitch(), m_model->ttsVolume());
}

void AccessibilityController::connectModelSignals() {
    if (!m_model) {
        return;
    }

    connect(m_model, &AccessibilityModel::settingsChanged, this,
            &AccessibilityController::onModelSettingsChanged);
    connect(m_model, &AccessibilityModel::screenReaderEnabledChanged, this,
            &AccessibilityController::onScreenReaderEnabledChanged);
    connect(m_model, &AccessibilityModel::highContrastModeChanged, this,
            &AccessibilityController::onHighContrastModeChanged);
    connect(m_model, &AccessibilityModel::ttsEnabledChanged, this,
            &AccessibilityController::onTtsEnabledChanged);
    connect(m_model, &AccessibilityModel::ttsVoiceChanged, this,
            &AccessibilityController::onTtsVoiceChanged);
    connect(m_model, &AccessibilityModel::ttsLocaleChanged, this,
            &AccessibilityController::onTtsLocaleChanged);
    connect(m_model, &AccessibilityModel::ttsEngineChanged, this,
            &AccessibilityController::onTtsEngineChanged);
}

void AccessibilityController::connectEventBusSignals() {
    EventBus& bus = EventBus::instance();

    bus.subscribe(AppEvents::PAGE_CHANGED(), this, [this](Event* event) {
        QVariantMap data = event->data().toMap();
        int page = data.value("page", 0).toInt();
        int total = data.value("total", 0).toInt();
        onPageChanged(page, total);
    });

    bus.subscribe(AppEvents::ZOOM_CHANGED(), this, [this](Event* event) {
        double zoom = event->data().toDouble();
        onZoomChanged(zoom);
    });

    bus.subscribe(AppEvents::DOCUMENT_OPENED(), this, [this](Event* event) {
        QString path = event->data().toString();
        onDocumentOpened(path);
    });

    bus.subscribe(AppEvents::DOCUMENT_CLOSED(), this,
                  [this](Event* /*event*/) { onDocumentClosed(); });

    bus.subscribe(AppEvents::THEME_CHANGED(), this,
                  [this](Event* /*event*/) { onThemeChanged(); });

    SLOG_DEBUG("EventBus signals connected");
}

void AccessibilityController::enableScreenReader(bool enable) {
    if (m_model->isScreenReaderEnabled() == enable) {
        return;
    }

    m_model->setScreenReaderEnabled(enable);

    if (enable) {
        m_announcementTimer->start();
        announceText(localizeMessage("screen_reader_enabled"));
        SLOG_INFO("Screen reader enabled");
    } else {
        m_announcementTimer->stop();
        clearAnnouncementQueue();
        SLOG_INFO("Screen reader disabled");
    }

    emit screenReaderStateChanged(enable);
    publishAccessibilityEvent(AccessibilityEvents::SCREEN_READER_TOGGLED,
                              enable);
}

bool AccessibilityController::isScreenReaderEnabled() const {
    return m_model && m_model->isScreenReaderEnabled();
}

void AccessibilityController::announceText(const QString& text) {
    if (!m_model->isScreenReaderEnabled() || text.isEmpty()) {
        return;
    }

    queueAnnouncement(text, 1);
    emit textAnnounced(text);
    publishAccessibilityEvent(AccessibilityEvents::TEXT_ANNOUNCED, text);
}

void AccessibilityController::announcePageChange(int pageNumber,
                                                 int totalPages) {
    if (!m_model->isScreenReaderEnabled() ||
        !m_model->shouldAnnouncePageChanges()) {
        return;
    }

    QString announcement = formatPageAnnouncement(pageNumber, totalPages);
    queueAnnouncement(announcement, 2);
}

void AccessibilityController::announceZoomChange(double zoomLevel) {
    if (!m_model->isScreenReaderEnabled() ||
        !m_model->shouldAnnounceZoomChanges()) {
        return;
    }

    QString announcement = formatZoomAnnouncement(zoomLevel);
    queueAnnouncement(announcement, 1);
}

void AccessibilityController::announceSelectionChange(
    const QString& selectedText) {
    if (!m_model->isScreenReaderEnabled() || selectedText.isEmpty()) {
        return;
    }

    QString announcement =
        localizeMessage("selection_changed", QStringList() << selectedText);
    queueAnnouncement(announcement, 0);
}

void AccessibilityController::announceError(const QString& error) {
    if (!m_model->isScreenReaderEnabled()) {
        return;
    }

    QString announcement = localizeMessage("error_prefix") + ": " + error;
    queueAnnouncement(announcement, 3);
}

void AccessibilityController::announceWarning(const QString& warning) {
    if (!m_model->isScreenReaderEnabled()) {
        return;
    }

    QString announcement = localizeMessage("warning_prefix") + ": " + warning;
    queueAnnouncement(announcement, 2);
}

void AccessibilityController::announceSuccess(const QString& message) {
    if (!m_model->isScreenReaderEnabled()) {
        return;
    }

    QString announcement = localizeMessage("success_prefix") + ": " + message;
    queueAnnouncement(announcement, 1);
}

void AccessibilityController::setHighContrastMode(bool enable) {
    if (m_model->isHighContrastMode() == enable) {
        return;
    }

    m_model->setHighContrastMode(enable);

    if (enable) {
        applyHighContrastColors();
    } else {
        restoreNormalColors();
    }

    emit highContrastStateChanged(enable);
    publishAccessibilityEvent(AccessibilityEvents::HIGH_CONTRAST_TOGGLED,
                              enable);
}

bool AccessibilityController::isHighContrastMode() const {
    return m_model && m_model->isHighContrastMode();
}

void AccessibilityController::applyHighContrastColors() {
    SLOG_DEBUG("Applying high contrast colors");

    if (qApp && qApp->style()) {
        QPalette palette = qApp->palette();
        palette.setColor(QPalette::Window, m_model->backgroundColor());
        palette.setColor(QPalette::WindowText, m_model->foregroundColor());
        palette.setColor(QPalette::Base, m_model->backgroundColor());
        palette.setColor(QPalette::AlternateBase,
                         m_model->backgroundColor().lighter(110));
        palette.setColor(QPalette::Text, m_model->foregroundColor());
        palette.setColor(QPalette::Button,
                         m_model->backgroundColor().lighter(105));
        palette.setColor(QPalette::ButtonText, m_model->foregroundColor());
        palette.setColor(QPalette::Highlight, m_model->highlightColor());
        palette.setColor(QPalette::HighlightedText, m_model->foregroundColor());
        qApp->setPalette(palette);
    }
}

void AccessibilityController::restoreNormalColors() {
    SLOG_DEBUG("Restoring normal colors");

    if (qApp && qApp->style()) {
        qApp->setPalette(qApp->style()->standardPalette());
    }
}

void AccessibilityController::enableTextToSpeech(bool enable) {
    if (m_model->isTtsEnabled() == enable) {
        return;
    }

    m_model->setTtsEnabled(enable);

    if (enable) {
        if (!m_tts) {
            initializeTextToSpeech();
        }
        emit featureEnabled("TextToSpeech");
        publishAccessibilityEvent(AccessibilityEvents::TTS_ENABLED, true);
        SLOG_INFO("Text-to-speech enabled");
    } else {
        if (m_tts) {
            m_tts->stop(QTextToSpeech::BoundaryHint::Immediate);
        }
        emit featureDisabled("TextToSpeech");
        publishAccessibilityEvent(AccessibilityEvents::TTS_DISABLED, true);
        SLOG_INFO("Text-to-speech disabled");
    }
}

bool AccessibilityController::isTextToSpeechEnabled() const {
    return m_model && m_model->isTtsEnabled();
}

void AccessibilityController::speak(const QString& text) {
    if (!m_tts || !m_model->isTtsEnabled() || text.isEmpty()) {
        return;
    }

    m_currentSpeechText = text;
    m_tts->say(text);
    emit speechStarted(text);
    SLOG_DEBUG_F("Speaking: {}", text.left(50));
}

void AccessibilityController::pause() {
    if (!m_tts) {
        return;
    }

    m_tts->pause(QTextToSpeech::BoundaryHint::Default);
    emit speechPaused();
    SLOG_DEBUG("Speech paused");
}

void AccessibilityController::resume() {
    if (!m_tts) {
        return;
    }

    m_tts->resume();
    emit speechResumed();
    SLOG_DEBUG("Speech resumed");
}

void AccessibilityController::stop() {
    if (!m_tts) {
        return;
    }

    m_tts->stop(QTextToSpeech::BoundaryHint::Immediate);
    m_currentSpeechText.clear();
    SLOG_DEBUG("Speech stopped");
}

QTextToSpeech::State AccessibilityController::textToSpeechState() const {
    if (!m_tts) {
        return QTextToSpeech::State::Error;
    }
    return m_tts->state();
}

bool AccessibilityController::isTextToSpeechAvailable() const {
    return m_tts && m_tts->state() != QTextToSpeech::State::Error;
}

QStringList AccessibilityController::availableEngines() const {
    return QTextToSpeech::availableEngines();
}

QString AccessibilityController::currentEngine() const {
    if (!m_tts) {
        return QString();
    }
    return m_tts->engine();
}

void AccessibilityController::setEngine(const QString& engine) {
    if (m_model->ttsEngine() == engine) {
        return;
    }

    m_model->setTtsEngine(engine);
    recreateTextToSpeech();
}

QList<QLocale> AccessibilityController::availableLocales() const {
    if (!m_tts) {
        return QList<QLocale>();
    }
    return m_tts->availableLocales();
}

QLocale AccessibilityController::currentLocale() const {
    if (!m_tts) {
        return QLocale::system();
    }
    return m_tts->locale();
}

void AccessibilityController::setLocale(const QLocale& locale) {
    if (!m_tts) {
        return;
    }

    m_model->setTtsLocale(locale);
    m_tts->setLocale(locale);
}

QList<QVoice> AccessibilityController::availableVoices() const {
    if (!m_tts) {
        return QList<QVoice>();
    }
    return m_tts->availableVoices();
}

QVoice AccessibilityController::currentVoice() const {
    if (!m_tts) {
        return QVoice();
    }
    return m_tts->voice();
}

void AccessibilityController::setVoice(const QVoice& voice) {
    if (!m_tts) {
        return;
    }

    m_model->setTtsVoice(voice);
    m_tts->setVoice(voice);
}

qreal AccessibilityController::speechRate() const {
    return m_model ? m_model->ttsRate() : 0.0;
}

void AccessibilityController::setSpeechRate(qreal rate) {
    if (!m_model) {
        return;
    }

    m_model->setTtsRate(rate);
    if (m_tts) {
        m_tts->setRate(rate);
    }
}

qreal AccessibilityController::speechPitch() const {
    return m_model ? m_model->ttsPitch() : 0.0;
}

void AccessibilityController::setSpeechPitch(qreal pitch) {
    if (!m_model) {
        return;
    }

    m_model->setTtsPitch(pitch);
    if (m_tts) {
        m_tts->setPitch(pitch);
    }
}

qreal AccessibilityController::speechVolume() const {
    return m_model ? m_model->ttsVolume() : 1.0;
}

void AccessibilityController::setSpeechVolume(qreal volume) {
    if (!m_model) {
        return;
    }

    m_model->setTtsVolume(volume);
    if (m_tts) {
        m_tts->setVolume(volume);
    }
}

void AccessibilityController::setTextEnlargement(bool enable) {
    if (m_model) {
        m_model->setTextEnlargementEnabled(enable);
    }
}

bool AccessibilityController::isTextEnlargementEnabled() const {
    return m_model && m_model->isTextEnlargementEnabled();
}

void AccessibilityController::setTextScaleFactor(qreal factor) {
    if (m_model) {
        m_model->setTextScaleFactor(factor);
    }
}

qreal AccessibilityController::textScaleFactor() const {
    return m_model ? m_model->textScaleFactor() : 1.0;
}

void AccessibilityController::setReduceMotion(bool reduce) {
    if (m_model) {
        m_model->setReduceMotion(reduce);
    }
}

bool AccessibilityController::shouldReduceMotion() const {
    return m_model && m_model->shouldReduceMotion();
}

void AccessibilityController::setReduceTransparency(bool reduce) {
    if (m_model) {
        m_model->setReduceTransparency(reduce);
    }
}

bool AccessibilityController::shouldReduceTransparency() const {
    return m_model && m_model->shouldReduceTransparency();
}

void AccessibilityController::setEnhancedKeyboardNavigation(bool enable) {
    if (m_model) {
        m_model->setEnhancedKeyboardNavigationEnabled(enable);
    }
}

bool AccessibilityController::isEnhancedKeyboardNavigationEnabled() const {
    return m_model && m_model->isEnhancedKeyboardNavigationEnabled();
}

void AccessibilityController::readCurrentPage() {
    if (!m_model->isTtsEnabled() || m_currentDocument.isEmpty()) {
        return;
    }

    QString announcement = formatPageAnnouncement(m_currentPage, m_totalPages);
    speak(announcement);
}

void AccessibilityController::readSelectedText() {
    // This would be connected to document selection events
    SLOG_DEBUG("Read selected text requested");
}

void AccessibilityController::readDocumentTitle() {
    if (!m_model->isTtsEnabled() || m_currentDocument.isEmpty()) {
        return;
    }

    QFileInfo fileInfo(m_currentDocument);
    QString title = localizeMessage("document_title_prefix") + ": " +
                    fileInfo.completeBaseName();
    speak(title);
}

void AccessibilityController::readStatusMessage(const QString& message) {
    if (!m_model->isTtsEnabled() || message.isEmpty()) {
        return;
    }

    speak(message);
}

bool AccessibilityController::testTextToSpeech() {
    if (!m_tts) {
        initializeTextToSpeech();
    }

    if (!m_tts || m_tts->state() == QTextToSpeech::State::Error) {
        SLOG_ERROR("Text-to-speech test failed: TTS not available");
        return false;
    }

    QString testMessage = localizeMessage("tts_test_message");
    speak(testMessage);
    return true;
}

QString AccessibilityController::getAccessibilityStatus() const {
    QStringList status;
    status << QString("Initialized: %1").arg(m_initialized ? "Yes" : "No");
    status << QString("Screen Reader: %1")
                  .arg(isScreenReaderEnabled() ? "Enabled" : "Disabled");
    status << QString("High Contrast: %1")
                  .arg(isHighContrastMode() ? "Enabled" : "Disabled");
    status << QString("Text-to-Speech: %1")
                  .arg(isTextToSpeechEnabled() ? "Enabled" : "Disabled");
    status << QString("TTS Available: %1")
                  .arg(isTextToSpeechAvailable() ? "Yes" : "No");
    status
        << QString("TTS State: %1").arg(static_cast<int>(textToSpeechState()));
    status << QString("Announcement Queue Size: %1")
                  .arg(m_announcementQueue.size());
    return status.join("\n");
}

QStringList AccessibilityController::getEnabledFeatures() const {
    QStringList features;
    if (isScreenReaderEnabled())
        features << "Screen Reader";
    if (isHighContrastMode())
        features << "High Contrast";
    if (isTextToSpeechEnabled())
        features << "Text-to-Speech";
    if (isEnhancedKeyboardNavigationEnabled())
        features << "Enhanced Keyboard";
    if (isTextEnlargementEnabled())
        features << "Text Enlargement";
    if (shouldReduceMotion())
        features << "Reduce Motion";
    return features;
}

void AccessibilityController::onPageChanged(int pageNumber, int totalPages) {
    m_currentPage = pageNumber;
    m_totalPages = totalPages;
    announcePageChange(pageNumber, totalPages);
}

void AccessibilityController::onZoomChanged(double zoomLevel) {
    m_currentZoom = zoomLevel;
    announceZoomChange(zoomLevel);
}

void AccessibilityController::onDocumentOpened(const QString& filePath) {
    m_currentDocument = filePath;
    m_currentPage = 1;
    m_totalPages = 0;

    if (m_model->isScreenReaderEnabled()) {
        QFileInfo fileInfo(filePath);
        QString announcement = localizeMessage(
            "document_opened", QStringList() << fileInfo.fileName());
        announceText(announcement);
    }
}

void AccessibilityController::onDocumentClosed() {
    m_currentDocument.clear();
    m_currentPage = 0;
    m_totalPages = 0;

    if (m_model->isScreenReaderEnabled()) {
        announceText(localizeMessage("document_closed"));
    }
}

void AccessibilityController::onSelectionChanged(const QString& selectedText) {
    announceSelectionChange(selectedText);
}

void AccessibilityController::onThemeChanged() {
    if (m_model->isHighContrastMode()) {
        applyHighContrastColors();
    }
}

void AccessibilityController::onLanguageChanged(
    const QString& /*languageCode*/) {
    // Update TTS locale if needed based on application language
    SLOG_DEBUG("Language changed, TTS locale may need update");
}

void AccessibilityController::onModelSettingsChanged(
    const AccessibilitySettings& /*settings*/) {
    publishAccessibilityEvent(AccessibilityEvents::SETTINGS_CHANGED);
}

void AccessibilityController::onScreenReaderEnabledChanged(bool enabled) {
    enableScreenReader(enabled);
}

void AccessibilityController::onHighContrastModeChanged(bool enabled) {
    setHighContrastMode(enabled);
}

void AccessibilityController::onTtsEnabledChanged(bool enabled) {
    enableTextToSpeech(enabled);
}

void AccessibilityController::onTtsVoiceChanged(const QVoice& voice) {
    if (m_tts) {
        m_tts->setVoice(voice);
    }
}

void AccessibilityController::onTtsLocaleChanged(const QLocale& locale) {
    if (m_tts) {
        m_tts->setLocale(locale);
    }
}

void AccessibilityController::onTtsEngineChanged(const QString& /*engine*/) {
    recreateTextToSpeech();
}

void AccessibilityController::onTtsStateChanged(QTextToSpeech::State state) {
    if (m_lastTtsState != state) {
        m_lastTtsState = state;
        emit textToSpeechStateChanged(state);
        publishAccessibilityEvent(AccessibilityEvents::TTS_STATE_CHANGED,
                                  static_cast<int>(state));

        if (state == QTextToSpeech::State::Ready) {
            m_isAnnouncing = false;
            emit speechFinished();
            m_currentSpeechText.clear();
            processAnnouncementQueue();
        }
    }
}

void AccessibilityController::onTtsErrorOccurred(
    QTextToSpeech::ErrorReason reason, const QString& errorString) {
    QString error = QString("Text-to-speech error (%1): %2")
                        .arg(static_cast<int>(reason))
                        .arg(errorString);
    SLOG_ERROR(error);
    emit textToSpeechError(error);
    emit errorOccurred(error);
}

void AccessibilityController::processAnnouncementQueue() {
    if (m_announcementQueue.isEmpty() || m_isAnnouncing) {
        return;
    }

    if (!m_tts || !m_model->isTtsEnabled()) {
        clearAnnouncementQueue();
        return;
    }

    if (m_tts->state() != QTextToSpeech::State::Ready) {
        return;
    }

    std::sort(m_announcementQueue.begin(), m_announcementQueue.end());

    if (!m_announcementQueue.isEmpty()) {
        Announcement announcement = m_announcementQueue.takeFirst();
        m_isAnnouncing = true;
        speak(announcement.text);
    }
}

void AccessibilityController::publishAccessibilityEvent(
    const QString& eventType, const QVariant& data) {
    EventBus::instance().publish(eventType, data);
}

QString AccessibilityController::formatPageAnnouncement(int pageNumber,
                                                        int totalPages) const {
    if (totalPages > 0) {
        return localizeMessage("page_announcement",
                               QStringList() << QString::number(pageNumber)
                                             << QString::number(totalPages));
    } else {
        return localizeMessage("page_announcement_single",
                               QStringList() << QString::number(pageNumber));
    }
}

QString AccessibilityController::formatZoomAnnouncement(
    double zoomLevel) const {
    int percentage = static_cast<int>(zoomLevel * 100);
    return localizeMessage("zoom_announcement",
                           QStringList() << QString::number(percentage));
}

QString AccessibilityController::localizeMessage(
    const QString& key, const QStringList& args) const {
    static QHash<QString, QString> fallbackMessages = {
        {"screen_reader_enabled", "Screen reader enabled"},
        {"screen_reader_disabled", "Screen reader disabled"},
        {"page_announcement", "Page %1 of %2"},
        {"page_announcement_single", "Page %1"},
        {"zoom_announcement", "Zoom level %1 percent"},
        {"document_opened", "Document opened: %1"},
        {"document_closed", "Document closed"},
        {"selection_changed", "Selected: %1"},
        {"document_title_prefix", "Document title"},
        {"error_prefix", "Error"},
        {"warning_prefix", "Warning"},
        {"success_prefix", "Success"},
        {"tts_test_message", "Text to speech is working correctly"}};

    QString message = fallbackMessages.value(key, key);

    for (int i = 0; i < args.size(); ++i) {
        message = message.arg(args[i]);
    }

    return message;
}

void AccessibilityController::queueAnnouncement(const QString& text,
                                                int priority) {
    if (text.isEmpty()) {
        return;
    }

    if (m_announcementQueue.size() >= m_maxQueueSize) {
        m_announcementQueue.removeLast();
        SLOG_WARN("Announcement queue full, removing oldest item");
    }

    Announcement announcement;
    announcement.text = text;
    announcement.priority = priority;
    announcement.timestamp = QDateTime::currentMSecsSinceEpoch();

    m_announcementQueue.append(announcement);
}

void AccessibilityController::clearAnnouncementQueue() {
    m_announcementQueue.clear();
    SLOG_DEBUG("Announcement queue cleared");
}
