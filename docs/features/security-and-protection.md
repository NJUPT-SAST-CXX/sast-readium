# Security and Content Protection

## Password Protection

SAST Readium supports opening and viewing password-protected PDF files, ensuring that sensitive documents remain secure.

### Features

- **Detection**: Automatically detects encrypted PDF files upon loading.
- **Password Dialog**: Presents a secure, modal dialog to prompt the user for a password.
- **Retry Mechanism**: Allows users to retry entering the password if the previous attempt was incorrect.
- **Encryption Support**: Supports standard PDF encryption methods (RC4, AES) as supported by the underlying Poppler library.

### User Experience

1. When opening a protected file, a "Password Required" dialog appears.
2. Enter the document password.
3. If correct, the document opens normally.
4. If incorrect, an error message is displayed, and you are prompted to try again.
5. Canceling the dialog aborts the file load.

## Watermarking

For users who need to protect their documents or add status indicators (e.g., "DRAFT", "CONFIDENTIAL"), SAST Readium includes a customizable watermarking feature.

### Configuration

Watermarks can be configured in the Settings dialog under the "View" or "Watermark" tab.

- **Text**: Custom string (e.g., "CONFIDENTIAL").
- **Color**: Choose any color for the text.
- **Opacity**: Adjust transparency (0% to 100%) to blend with the document content.
- **Size**: Font size adjustment.
- **Rotation**: Rotate the watermark (e.g., 45 degrees).

### Rendering

- Watermarks are rendered as an overlay on top of the PDF content.
- They are consistent across all pages of the document.
- Optimized for performance to avoid lag during scrolling.

## Technical Implementation

### Password Handling

- **`PDFViewer::loadPDFDocument`**: Catches `PasswordException` or checks `document->isLocked()`.
- **`PasswordDialog`**: A QDialog subclass that securely handles password input (masking characters).
- **Poppler Integration**: Uses `Poppler::Document::load` with the provided password or `document->unlock(password)` methods.

### Watermark Rendering

- **`PDFWatermark` Component**: Stores the watermark state (text, style).
- **Rendering Pipeline**:
  - The watermark is drawn after the PDF page content in the `paintEvent` of the page widget.
  - Uses `QPainter` with high-quality antialiasing.
  - Caches the watermark layout if necessary for performance.
