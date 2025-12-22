# TTS Enhancer Plugin

This plugin demonstrates Text-to-Speech functionality using Qt's QTextToSpeech.

## Features

- **TTS Control**: Play, pause, stop, resume reading
- **Voice Selection**: Multiple system voice options
- **Speed Control**: Adjustable reading speed (0.5x - 2.0x)
- **Volume Control**: Adjustable volume level
- **Auto-advance**: Automatically proceed to next page
- **Highlight Sync**: Highlight words as they're read

## Keyboard Shortcuts

| Action | Shortcut |
|--------|----------|
| Play/Pause | F5 |
| Stop | F6 |
| Speed Up | Ctrl+] |
| Slow Down | Ctrl+[ |

## VoiceConfig Structure

```cpp
struct VoiceConfig {
    QString voiceName;
    QString language;
    double rate;      // 0.5 - 2.0, 1.0 = normal
    double pitch;     // 0.0 - 2.0, 1.0 = normal
    double volume;    // 0.0 - 1.0
};
```

## Inter-plugin Communication

### Speak Text

```cpp
QVariantMap msg;
msg["action"] = "speak";
msg["text"] = "Hello, world!";
pluginManager->sendMessage("TTS Enhancer", msg);
```

### Speak Page

```cpp
QVariantMap msg;
msg["action"] = "speak_page";
msg["pageNumber"] = 5;
pluginManager->sendMessage("TTS Enhancer", msg);
```

### Control Playback

```cpp
QVariantMap msg;
msg["action"] = "pause";  // or "resume", "stop"
pluginManager->sendMessage("TTS Enhancer", msg);
```

### Set Rate

```cpp
QVariantMap msg;
msg["action"] = "set_rate";
msg["rate"] = 1.5;  // 1.5x speed
pluginManager->sendMessage("TTS Enhancer", msg);
```

### Get Available Voices

```cpp
QVariantMap msg;
msg["action"] = "get_voices";
pluginManager->sendMessage("TTS Enhancer", msg);
// Response: { voices: [...], currentRate, currentVolume }
```

## Events Published

| Event | Data |
|-------|------|
| `tts.started` | textLength |
| `tts.paused` | - |
| `tts.resumed` | - |
| `tts.stopped` | - |
| `tts.pageComplete` | pageNumber, autoAdvance |
| `tts.wordBoundary` | start, length |

## Configuration

```json
{
    "rate": 1.0,
    "volume": 1.0,
    "autoAdvance": true,
    "highlightSync": true,
    "defaultVoice": ""
}
```

## Requirements

- Qt6 TextToSpeech module
- System TTS voices installed

## Building

```bash
mkdir build && cd build
cmake .. && cmake --build .
```
