#include "ReadingModeManager.h"
#include "../../managers/StyleManager.h"

ReadingModeManager::ReadingModeManager(QObject* parent)
    : QObject(parent),
      m_currentMode(Normal),
      m_backgroundColor(Qt::white),
      m_foregroundColor(Qt::black),
      m_brightness(1.0) {
    // Connect to theme changes to update Normal mode colors
    connect(&STYLE, &StyleManager::themeChanged, this,
            &ReadingModeManager::onThemeChanged);

    // Initialize with current theme colors for Normal mode
    if (m_currentMode == Normal) {
        m_backgroundColor = STYLE.backgroundColor();
        m_foregroundColor = STYLE.textColor();
    }
}

void ReadingModeManager::setReadingMode(ReadingMode mode) {
    if (m_currentMode != mode) {
        m_currentMode = mode;
        applyMode(mode);
        emit readingModeChanged(mode);
    }
}

void ReadingModeManager::applyMode(ReadingMode mode) {
    switch (mode) {
        case Normal:
            // Normal mode respects current theme
            m_backgroundColor = STYLE.backgroundColor();
            m_foregroundColor = STYLE.textColor();
            break;

        case Night:
            // Night mode: fixed dark colors for reading
            m_backgroundColor = QColor(30, 30, 30);
            m_foregroundColor = QColor(220, 220, 220);
            break;

        case Sepia:
            // Sepia mode: fixed warm colors for reading
            m_backgroundColor = QColor(244, 241, 222);
            m_foregroundColor = QColor(75, 60, 40);
            break;

        case Custom:
            // Use custom colors set via setCustomColors
            break;
    }

    emit colorsChanged();
}

void ReadingModeManager::setCustomColors(const QColor& bg, const QColor& fg) {
    m_backgroundColor = bg;
    m_foregroundColor = fg;
    if (m_currentMode == Custom) {
        emit colorsChanged();
    }
}

void ReadingModeManager::setBrightness(qreal brightness) {
    qreal newBrightness = qBound(0.0, brightness, 1.0);
    if (qAbs(m_brightness - newBrightness) > 0.01) {
        m_brightness = newBrightness;
        emit brightnessChanged(m_brightness);
    }
}

void ReadingModeManager::onThemeChanged() {
    // Only update colors if in Normal mode (other modes are fixed)
    if (m_currentMode == Normal) {
        m_backgroundColor = STYLE.backgroundColor();
        m_foregroundColor = STYLE.textColor();
        emit colorsChanged();
    }
}

QPalette ReadingModeManager::getPalette() const {
    QPalette palette;

    // Apply brightness adjustment
    QColor adjustedBg = m_backgroundColor;
    QColor adjustedFg = m_foregroundColor;

    if (m_brightness < 1.0) {
        int factor = static_cast<int>(m_brightness * 255);
        adjustedBg = adjustedBg.darker(100 + (100 - factor));
    }

    palette.setColor(QPalette::Window, adjustedBg);
    palette.setColor(QPalette::WindowText, adjustedFg);
    palette.setColor(QPalette::Base, adjustedBg);
    palette.setColor(QPalette::Text, adjustedFg);

    return palette;
}
