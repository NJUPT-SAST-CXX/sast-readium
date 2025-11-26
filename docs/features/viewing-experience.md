# Viewing Experience

## Theme Support

SAST Readium offers multiple viewing themes to reduce eye strain and adapt to different lighting environments.

### Available Themes

- **Light Mode**: The standard high-contrast paper-like appearance. Best for bright environments.
- **Dark Mode**: Inverted colors for low-light environments. Reduces glare and power consumption on OLED screens.
- **Sepia Mode**: A warm, yellowish tint (like old paper) that reduces blue light exposure. Ideal for long reading sessions.

### Implementation

- Themes are applied globally to the application UI (toolbars, sidebars).
- For the PDF content:
  - **Light**: Standard rendering.
  - **Dark**: Inverts luminance while preserving hue for images (smart inversion) or simple inversion depending on settings.
  - **Sepia**: Applies a color filter overlay or modifies the rendering pipeline to tint the page background and text.

## Native Link Support

Modern PDFs often contain hyperlinks to other pages within the document (internal links) or to websites (external links).

### Features

- **Internal Navigation**: Clicking an item in the Table of Contents or a cross-reference instantly jumps to the target page.
- **External Links**: Clicking a web URL launches the user's default web browser.
- **Hover Effects**: The cursor changes to a pointing hand to indicate clickable areas.
- **Tooltips**: Hovering over a link may show the destination URL or page number.

### Technical Details

- **Annotation Parsing**: The viewer extracts `Link` annotations from the PDF page using `Poppler::Page::links()`.
- **Geometry Mapping**: Link coordinates are mapped from PDF point space to the view's coordinate system.
- **Event Handling**: Mouse clicks are hit-tested against the list of links on the current page.

## Spatial Indexing for Text Selection

To ensure smooth performance on pages with massive amounts of text, SAST Readium employs advanced spatial indexing.

- **Problem**: Searching linearly through thousands of text characters to find what is under the mouse cursor is slow (O(N)).
- **Solution**: We use a **QuadTree** or **Grid-based Spatial Hash** (QMultiHash) to index text positions.
- **Result**: Hit-testing for text selection becomes near-instantaneous (O(1) or O(log N)), ensuring the UI remains responsive even on complex technical documents.
