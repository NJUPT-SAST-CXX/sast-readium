#include "AccessibilityManager.h"
#include <QTextToSpeech>
#include "../controller/AccessibilityController.h"
#include "../controller/ServiceLocator.h"
#include "../logging/SimpleLogging.h"
#include "../model/AccessibilityModel.h"

AccessibilityManager::AccessibilityManager(QObject* parent)
    : QObject(parent), m_initialized(false) {
    m_model = new AccessibilityModel(this);
    m_controller = new AccessibilityController(m_model, this);
}

AccessibilityManager::~AccessibilityManager() {
    if (m_controller) {
        m_controller->shutdown();
    }
}

void AccessibilityManager::initialize() {
    if (m_initialized) {
        SLOG_WARN("AccessibilityManager already initialized");
        return;
    }

    if (!m_controller) {
        SLOG_ERROR("AccessibilityManager: Controller is null");
        return;
    }

    m_controller->initialize();
    setupConnections();

    m_initialized = true;
    emit initialized();
    SLOG_INFO("AccessibilityManager initialized (using new MVP architecture)");
}

bool AccessibilityManager::isInitialized() const { return m_initialized; }

void AccessibilityManager::setupConnections() {
    if (!m_controller) {
        return;
    }

    connect(m_controller, &AccessibilityController::screenReaderStateChanged,
            this, &AccessibilityManager::screenReaderModeChanged);

    connect(m_controller, &AccessibilityController::highContrastStateChanged,
            this, &AccessibilityManager::highContrastModeChanged);

    connect(m_controller, &AccessibilityController::textToSpeechStateChanged,
            this, [this](QTextToSpeech::State state) {
                bool active = (state == QTextToSpeech::State::Speaking);
                emit textToSpeechStateChanged(active);
            });

    connect(m_controller, &AccessibilityController::speechFinished, this,
            &AccessibilityManager::textToSpeechFinished);
}

void AccessibilityManager::enableScreenReaderMode(bool enable) {
    if (m_controller) {
        m_controller->enableScreenReader(enable);
    }
}

bool AccessibilityManager::isScreenReaderEnabled() const {
    return m_controller ? m_controller->isScreenReaderEnabled() : false;
}

void AccessibilityManager::announceText(const QString& text) {
    if (m_controller) {
        m_controller->announceText(text);
    }
}

void AccessibilityManager::announcePageChange(int pageNumber, int totalPages) {
    if (m_controller) {
        m_controller->announcePageChange(pageNumber, totalPages);
    }
}

void AccessibilityManager::setHighContrastMode(bool enable) {
    if (m_controller) {
        m_controller->setHighContrastMode(enable);
    }
}

bool AccessibilityManager::isHighContrastMode() const {
    return m_controller ? m_controller->isHighContrastMode() : false;
}

QColor AccessibilityManager::getBackgroundColor() const {
    if (m_model) {
        return m_model->backgroundColor();
    }
    return QColor(Qt::white);
}

QColor AccessibilityManager::getForegroundColor() const {
    if (m_model) {
        return m_model->foregroundColor();
    }
    return QColor(Qt::black);
}

QColor AccessibilityManager::getHighlightColor() const {
    if (m_model) {
        return m_model->highlightColor();
    }
    return QColor(255, 255, 0, 128);
}

void AccessibilityManager::startTextToSpeech(const QString& text) {
    if (m_controller) {
        if (!m_controller->isTextToSpeechEnabled()) {
            m_controller->enableTextToSpeech(true);
        }
        m_controller->speak(text);
    }
}

void AccessibilityManager::stopTextToSpeech() {
    if (m_controller) {
        m_controller->stop();
    }
}

void AccessibilityManager::pauseTextToSpeech() const {
    if (m_controller) {
        const_cast<AccessibilityController*>(m_controller.data())->pause();
    }
}

void AccessibilityManager::resumeTextToSpeech() const {
    if (m_controller) {
        const_cast<AccessibilityController*>(m_controller.data())->resume();
    }
}

bool AccessibilityManager::isTextToSpeechActive() const {
    if (m_controller) {
        return m_controller->textToSpeechState() ==
               QTextToSpeech::State::Speaking;
    }
    return false;
}

void AccessibilityManager::setTextToSpeechRate(qreal rate) {
    if (m_controller) {
        m_controller->setSpeechRate(rate);
    }
}

void AccessibilityManager::setTextToSpeechVolume(qreal volume) {
    if (m_controller) {
        m_controller->setSpeechVolume(volume);
    }
}
