# Text-to-Speech (TTS)

## Overview

The Text-to-Speech (TTS) feature provides accessibility and convenience by allowing users to listen to the content of PDF documents. It utilizes the system's native speech synthesis engine via `QtTextToSpeech` to provide a seamless and efficient reading experience.

## Key Features

### 🔊 Playback Control

- **Play/Pause/Stop**: Full control over the reading process.
- **Auto-Advance**: Automatically turns the page when the current page finishes reading.
- **Visual Feedback**: Highlights the current page being read (implementation dependent).

### 🎛️ Customization

- **Voice Selection**: Choose from available system voices (dependent on OS).
- **Reading Rate**: Adjust the speed of speech to suit your preference.
- **Volume Control**: Independent volume control for the TTS engine.
- **Pitch Control**: (Optional/If implemented) Adjust the pitch of the voice.

## Usage

1. **Activate TTS**: Click the "Read" or "TTS" icon in the toolbar.
2. **Controls**: A floating or docked control panel will appear with Play, Pause, and Stop buttons.
3. **Settings**:
   - Use the slider to adjust the reading speed (Rate).
   - Use the volume slider to adjust the output volume.
4. **Navigation**: The TTS engine follows the current page. Changing pages manually will stop the current reading or prepare to read the new page.

## Technical Implementation

The TTS system is built on top of the `QtTextToSpeech` module.

### Architecture

- **`PDFTTSReader`**: The main class managing the TTS engine state and playback logic.
- **`useTTS` Hook**: A React/State-management style hook (if using such a pattern) or a controller class that exposes state (isReading, rate, volume) to the UI.
- **Integration**: The `PDFViewer` component integrates the TTS controls and responds to page change events to trigger auto-advance.

### Auto-Advance Logic

When the TTS engine signals that it has finished reading the text of the current page, the `PDFViewer` automatically triggers a "Next Page" command. This ensures a continuous reading experience for long documents.

## Platform Support

- **Windows**: Uses SAPI (Microsoft Speech API) or Windows Runtime Text-to-Speech.
- **macOS**: Uses NSSpeechSynthesizer.
- **Linux**: Uses Speech Dispatcher.

## Troubleshooting

- **No Audio**: Check system volume and ensure a valid voice is selected.
- **Missing Voices**: Install additional language packs in your operating system settings.
- **Linux**: Ensure `speech-dispatcher` and `libqt6texttospeech6` (and plugins) are installed.
