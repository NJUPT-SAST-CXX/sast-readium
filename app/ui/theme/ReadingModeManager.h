#pragma once

#include <QColor>
#include <QObject>
#include <QPalette>

/**
 * @brief Manages night/reading modes (Feature 15)
 */
class ReadingModeManager : public QObject {
    Q_OBJECT

public:
    static ReadingModeManager& instance() {
        static ReadingModeManager instance;
        return instance;
    }

    enum ReadingMode { Normal, Night, Sepia, Custom };

    explicit ReadingModeManager(QObject* parent = nullptr);
    ~ReadingModeManager() = default;

    void setReadingMode(ReadingMode mode);
    ReadingMode getReadingMode() const { return m_currentMode; }

    void setCustomColors(const QColor& bg, const QColor& fg);
    QColor getBackgroundColor() const { return m_backgroundColor; }
    QColor getForegroundColor() const { return m_foregroundColor; }

    void setBrightness(qreal brightness);  // 0.0 - 1.0
    qreal getBrightness() const { return m_brightness; }

    QPalette getPalette() const;

signals:
    void readingModeChanged(ReadingMode mode);
    void brightnessChanged(qreal brightness);
    void colorsChanged();

private:
    void applyMode(ReadingMode mode);
    void onThemeChanged();

    ReadingMode m_currentMode;
    QColor m_backgroundColor;
    QColor m_foregroundColor;
    qreal m_brightness;
};
