#include "TTSEnhancerPlugin.h"
#include <QAction>
#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>
#include "controller/EventBus.h"
#include "plugin/PluginHookRegistry.h"

TTSEnhancerPlugin::TTSEnhancerPlugin(QObject* parent)
    : PluginBase(parent),
      m_tts(nullptr),
      m_currentPage(1),
      m_autoAdvance(true),
      m_highlightSync(true),
      m_playPauseAction(nullptr),
      m_stopAction(nullptr),
      m_wordsSpoken(0) {
    m_metadata.name = "TTS Enhancer";
    m_metadata.version = "1.0.0";
    m_metadata.description =
        "Text-to-Speech with voice selection, speed control, and highlight "
        "sync";
    m_metadata.author = "SAST Readium Team";
    m_metadata.dependencies = QStringList();
    m_capabilities.provides = QStringList() << "tts.control" << "tts.voices"
                                            << "ui.toolbar" << "accessibility";

    m_voiceConfig.rate = 1.0;
    m_voiceConfig.pitch = 1.0;
    m_voiceConfig.volume = 1.0;
}

TTSEnhancerPlugin::~TTSEnhancerPlugin() {
    qDeleteAll(m_menuActions);
    qDeleteAll(m_toolbarActions);
    qDeleteAll(m_contextActions);
    delete m_tts;
}

bool TTSEnhancerPlugin::onInitialize() {
    m_logger.info("TTSEnhancerPlugin: Initializing...");

    // Initialize TTS engine
    m_tts = new QTextToSpeech(this);
    connect(m_tts, &QTextToSpeech::stateChanged, this,
            &TTSEnhancerPlugin::onStateChanged);

    // Load configuration
    m_voiceConfig.rate = m_configuration.value("rate").toDouble(1.0);
    m_voiceConfig.volume = m_configuration.value("volume").toDouble(1.0);
    m_autoAdvance = m_configuration.value("autoAdvance").toBool(true);
    m_highlightSync = m_configuration.value("highlightSync").toBool(true);

    m_tts->setRate(m_voiceConfig.rate - 1.0);  // Qt uses -1.0 to 1.0
    m_tts->setVolume(m_voiceConfig.volume);

    // Create actions
    m_playPauseAction = new QAction("Play", this);
    m_playPauseAction->setShortcut(QKeySequence("F5"));
    connect(m_playPauseAction, &QAction::triggered, this,
            &TTSEnhancerPlugin::onPlayPause);
    m_menuActions.append(m_playPauseAction);
    m_toolbarActions.append(m_playPauseAction);

    m_stopAction = new QAction("Stop", this);
    m_stopAction->setShortcut(QKeySequence("F6"));
    m_stopAction->setEnabled(false);
    connect(m_stopAction, &QAction::triggered, this,
            &TTSEnhancerPlugin::onStop);
    m_menuActions.append(m_stopAction);
    m_toolbarActions.append(m_stopAction);

    QAction* sep = new QAction(this);
    sep->setSeparator(true);
    m_menuActions.append(sep);

    QAction* speedUpAction = new QAction("Speed Up", this);
    speedUpAction->setShortcut(QKeySequence("Ctrl+]"));
    connect(speedUpAction, &QAction::triggered, this,
            &TTSEnhancerPlugin::onSpeedUp);
    m_menuActions.append(speedUpAction);

    QAction* slowDownAction = new QAction("Slow Down", this);
    slowDownAction->setShortcut(QKeySequence("Ctrl+["));
    connect(slowDownAction, &QAction::triggered, this,
            &TTSEnhancerPlugin::onSlowDown);
    m_menuActions.append(slowDownAction);

    // Context menu
    QAction* readSelectionAction = new QAction("Read Selection", this);
    connect(readSelectionAction, &QAction::triggered, this,
            [this]() { eventBus()->publish(new Event("tts.readSelection")); });
    m_contextActions.append(readSelectionAction);

    registerHooks();
    setupEventSubscriptions();

    m_logger.info(QString("TTSEnhancerPlugin: Available voices: %1")
                      .arg(availableVoices().size()));
    return true;
}

void TTSEnhancerPlugin::onShutdown() {
    m_logger.info("TTSEnhancerPlugin: Shutting down...");
    stop();
    PluginHookRegistry::instance().unregisterAllCallbacks(name());
    eventBus()->unsubscribeAll(this);
    m_logger.info(
        QString("TTSEnhancerPlugin: Words spoken: %1").arg(m_wordsSpoken));
}

void TTSEnhancerPlugin::handleMessage(const QString& from,
                                      const QVariant& message) {
    QVariantMap msgMap = message.toMap();
    QString action = msgMap.value("action").toString();

    if (action == "speak") {
        QString text = msgMap.value("text").toString();
        speak(text);
    } else if (action == "speak_page") {
        int page = msgMap.value("pageNumber").toInt(m_currentPage);
        speakPage(page);
    } else if (action == "pause") {
        pause();
    } else if (action == "resume") {
        resume();
    } else if (action == "stop") {
        stop();
    } else if (action == "set_rate") {
        setRate(msgMap.value("rate").toDouble());
    } else if (action == "set_volume") {
        setVolume(msgMap.value("volume").toDouble());
    } else if (action == "get_voices") {
        Event* resp = new Event("plugin.response");
        QVariantMap data;
        data["from"] = name();
        data["to"] = from;
        data["voices"] = availableVoices();
        data["currentRate"] = m_voiceConfig.rate;
        data["currentVolume"] = m_voiceConfig.volume;
        resp->setData(QVariant::fromValue(data));
        eventBus()->publish(resp);
    } else if (action == "get_status") {
        Event* resp = new Event("plugin.response");
        QVariantMap data;
        data["from"] = name();
        data["to"] = from;
        data["isSpeaking"] = isSpeaking();
        data["isPaused"] = isPaused();
        data["wordsSpoken"] = m_wordsSpoken;
        resp->setData(QVariant::fromValue(data));
        eventBus()->publish(resp);
    }
}

// ============================================================================
// IUIExtension
// ============================================================================

QList<QAction*> TTSEnhancerPlugin::menuActions() const { return m_menuActions; }

QList<QAction*> TTSEnhancerPlugin::toolbarActions() const {
    return m_toolbarActions;
}

QList<QAction*> TTSEnhancerPlugin::contextMenuActions() const {
    return m_contextActions;
}

QString TTSEnhancerPlugin::statusBarMessage() const {
    if (isSpeaking()) {
        return QString("Reading... (%.1fx)").arg(m_voiceConfig.rate);
    } else if (isPaused()) {
        return "Paused";
    }
    return QString();
}

QWidget* TTSEnhancerPlugin::createDockWidget() {
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);

    QLabel* titleLabel = new QLabel("<b>Text-to-Speech</b>");
    layout->addWidget(titleLabel);

    QLabel* statusLabel = new QLabel("Ready");
    statusLabel->setObjectName("ttsStatusLabel");
    layout->addWidget(statusLabel);

    QLabel* rateLabel = new QLabel("Speed: 1.0x");
    rateLabel->setObjectName("rateLabel");
    layout->addWidget(rateLabel);

    QSlider* rateSlider = new QSlider(Qt::Horizontal);
    rateSlider->setObjectName("rateSlider");
    rateSlider->setMinimum(50);
    rateSlider->setMaximum(200);
    rateSlider->setValue(100);
    layout->addWidget(rateSlider);

    QLabel* volumeLabel = new QLabel("Volume: 100%");
    volumeLabel->setObjectName("volumeLabel");
    layout->addWidget(volumeLabel);

    QSlider* volumeSlider = new QSlider(Qt::Horizontal);
    volumeSlider->setObjectName("volumeSlider");
    volumeSlider->setMinimum(0);
    volumeSlider->setMaximum(100);
    volumeSlider->setValue(100);
    layout->addWidget(volumeSlider);

    layout->addStretch();
    return widget;
}

// ============================================================================
// TTS API
// ============================================================================

void TTSEnhancerPlugin::speak(const QString& text) {
    if (text.isEmpty())
        return;

    m_currentText = text;
    m_tts->say(text);
    m_wordsSpoken += text.split(QRegularExpression("\\s+")).count();
    updateActions();

    Event* event = new Event("tts.started");
    QVariantMap data;
    data["textLength"] = text.length();
    event->setData(QVariant::fromValue(data));
    eventBus()->publish(event);
}

void TTSEnhancerPlugin::speakPage(int pageNumber) {
    QString text = extractPageText(pageNumber);
    if (!text.isEmpty()) {
        m_currentPage = pageNumber;
        speak(text);
    }
}

void TTSEnhancerPlugin::pause() {
    m_tts->pause();
    updateActions();
    eventBus()->publish(new Event("tts.paused"));
}

void TTSEnhancerPlugin::resume() {
    m_tts->resume();
    updateActions();
    eventBus()->publish(new Event("tts.resumed"));
}

void TTSEnhancerPlugin::stop() {
    m_tts->stop();
    m_currentText.clear();
    updateActions();
    eventBus()->publish(new Event("tts.stopped"));
}

void TTSEnhancerPlugin::setRate(double rate) {
    m_voiceConfig.rate = qBound(0.5, rate, 2.0);
    m_tts->setRate(m_voiceConfig.rate - 1.0);
    m_logger.info(
        QString("TTSEnhancerPlugin: Rate set to %1x").arg(m_voiceConfig.rate));
}

void TTSEnhancerPlugin::setVolume(double volume) {
    m_voiceConfig.volume = qBound(0.0, volume, 1.0);
    m_tts->setVolume(m_voiceConfig.volume);
}

void TTSEnhancerPlugin::setVoice(const QString& voiceName) {
    for (const QVoice& voice : m_tts->availableVoices()) {
        if (voice.name() == voiceName) {
            m_tts->setVoice(voice);
            m_voiceConfig.voiceName = voiceName;
            break;
        }
    }
}

QStringList TTSEnhancerPlugin::availableVoices() const {
    QStringList voices;
    for (const QVoice& voice : m_tts->availableVoices()) {
        voices.append(voice.name());
    }
    return voices;
}

bool TTSEnhancerPlugin::isSpeaking() const {
    return m_tts->state() == QTextToSpeech::Speaking;
}

bool TTSEnhancerPlugin::isPaused() const {
    return m_tts->state() == QTextToSpeech::Paused;
}

QString TTSEnhancerPlugin::extractPageText(int pageNumber) {
    Q_UNUSED(pageNumber)
    // Simulated - in real implementation, extract from PDF via Poppler
    return "This is simulated page text for demonstration purposes. "
           "In a real implementation, this would extract actual text from the "
           "PDF page.";
}

void TTSEnhancerPlugin::updateActions() {
    bool speaking = isSpeaking();
    bool paused = isPaused();

    m_playPauseAction->setText(speaking ? "Pause" : "Play");
    m_stopAction->setEnabled(speaking || paused);
}

// ============================================================================
// Slots
// ============================================================================

void TTSEnhancerPlugin::onPlayPause() {
    if (isSpeaking()) {
        pause();
    } else if (isPaused()) {
        resume();
    } else {
        speakPage(m_currentPage);
    }
}

void TTSEnhancerPlugin::onStop() { stop(); }

void TTSEnhancerPlugin::onSpeedUp() { setRate(m_voiceConfig.rate + 0.1); }

void TTSEnhancerPlugin::onSlowDown() { setRate(m_voiceConfig.rate - 0.1); }

void TTSEnhancerPlugin::onStateChanged(QTextToSpeech::State state) {
    updateActions();

    if (state == QTextToSpeech::Ready && m_autoAdvance &&
        !m_currentText.isEmpty()) {
        // Auto-advance to next page
        Event* event = new Event("tts.pageComplete");
        QVariantMap data;
        data["pageNumber"] = m_currentPage;
        data["autoAdvance"] = m_autoAdvance;
        event->setData(QVariant::fromValue(data));
        eventBus()->publish(event);
    }
}

void TTSEnhancerPlugin::onWordBoundary(qint64 start, qint64 length) {
    if (!m_highlightSync)
        return;

    Event* event = new Event("tts.wordBoundary");
    QVariantMap data;
    data["start"] = start;
    data["length"] = length;
    event->setData(QVariant::fromValue(data));
    eventBus()->publish(event);
}

// ============================================================================
// Hooks & Events
// ============================================================================

void TTSEnhancerPlugin::registerHooks() {
    auto& registry = PluginHookRegistry::instance();
    registry.registerCallback(
        "tts.requested", name(),
        [this](const QVariantMap& ctx) { return onTTSRequested(ctx); });
}

void TTSEnhancerPlugin::setupEventSubscriptions() {
    eventBus()->subscribe("page.changed", this, [this](Event* event) {
        if (!isSpeaking()) {
            m_currentPage = event->data().toInt();
        }
    });

    eventBus()->subscribe("document.closed", this, [this](Event*) { stop(); });
}

QVariant TTSEnhancerPlugin::onTTSRequested(const QVariantMap& context) {
    QString text = context.value("text").toString();
    if (!text.isEmpty()) {
        speak(text);
    }

    QVariantMap result;
    result["handled"] = true;
    return result;
}
