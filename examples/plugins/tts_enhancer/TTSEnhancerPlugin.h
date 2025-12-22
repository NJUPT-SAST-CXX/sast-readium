#pragma once

#include <QObject>
#include <QTextToSpeech>
#include "plugin/PluginInterface.h"

/**
 * @brief TTS voice configuration
 */
struct VoiceConfig {
    QString voiceName;
    QString language;
    double rate;    // 0.0 - 2.0, 1.0 = normal
    double pitch;   // 0.0 - 2.0, 1.0 = normal
    double volume;  // 0.0 - 1.0
};

/**
 * @brief TTSEnhancerPlugin - Text-to-Speech enhancement plugin
 *
 * This plugin demonstrates:
 * - **TTS Control**: Play, pause, stop, resume reading
 * - **Voice Selection**: Multiple voice options
 * - **Speed Control**: Adjustable reading speed
 * - **Highlight Sync**: Highlight text as it's read
 * - **Auto-advance**: Automatically go to next page
 */
class TTSEnhancerPlugin : public PluginBase, public IUIExtension {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.sast.readium.IPlugin/1.0" FILE
                          "tts_enhancer.json")
    Q_INTERFACES(IPluginInterface IUIExtension)

public:
    explicit TTSEnhancerPlugin(QObject* parent = nullptr);
    ~TTSEnhancerPlugin() override;

    void handleMessage(const QString& from, const QVariant& message) override;

    // IUIExtension
    QList<QAction*> menuActions() const override;
    QList<QAction*> toolbarActions() const override;
    QList<QAction*> contextMenuActions() const override;
    QString statusBarMessage() const override;
    QWidget* createDockWidget() override;
    QString menuPath() const override { return "Tools/Read Aloud"; }
    QString toolbarId() const override { return "tools_toolbar"; }

    // TTS API
    void speak(const QString& text);
    void speakPage(int pageNumber);
    void pause();
    void resume();
    void stop();
    void setRate(double rate);
    void setVolume(double volume);
    void setVoice(const QString& voiceName);
    QStringList availableVoices() const;
    bool isSpeaking() const;
    bool isPaused() const;

protected:
    bool onInitialize() override;
    void onShutdown() override;

private slots:
    void onPlayPause();
    void onStop();
    void onSpeedUp();
    void onSlowDown();
    void onStateChanged(QTextToSpeech::State state);
    void onWordBoundary(qint64 start, qint64 length);

private:
    void registerHooks();
    void setupEventSubscriptions();
    void updateActions();
    QString extractPageText(int pageNumber);

    QVariant onTTSRequested(const QVariantMap& context);

    // TTS engine
    QTextToSpeech* m_tts;
    VoiceConfig m_voiceConfig;

    // State
    QString m_currentText;
    int m_currentPage;
    bool m_autoAdvance;
    bool m_highlightSync;

    // UI
    QList<QAction*> m_menuActions;
    QList<QAction*> m_toolbarActions;
    QList<QAction*> m_contextActions;
    QAction* m_playPauseAction;
    QAction* m_stopAction;

    // Stats
    int m_wordsSpoken;
};
