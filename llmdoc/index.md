# SAST Readium Project Documentation Index

This is the master index for all technical documentation in the SAST Readium project. Each entry links to detailed documentation about specific features, systems, and processes.

## Feature Documentation

[Keyboard Shortcut System](feature/keyboard-shortcut-manager.md): Centralized keyboard shortcut management system with context-sensitivity, conflict detection, and accessibility support.

[Annotation System](feature/annotation-model.md): PDF annotation management system supporting multiple annotation types, serialization, and Poppler integration. Composed of layers: [Annotation Model](feature/annotation-model.md) for data management, [Annotation Controller](feature/annotation-controller.md) for business logic, [Annotation Commands](feature/annotation-commands.md) for undo/redo, [Annotation Rendering](feature/annotation-rendering.md) for visual display, [Annotation Selection Manager](feature/annotation-selection-manager.md) for interactive selection and resizing, and [Annotation Integration Helper](feature/annotation-integration-helper.md) for coordinating all components.

[Application Freeze Fixes](feature/application-freeze-fixes.md): Critical fixes addressing UI freezes through async thread cleanup in AsyncDocumentLoader, timeout-bounded waits in SearchEngine, deadlock prevention in EventBus via lock minimization, and timer callback overlap prevention in CacheManager using atomic flags.

[Plugin System](feature/plugin-system.md): Comprehensive plugin architecture supporting dynamic plugin loading, dependency resolution, event-driven communication, service integration, and extensible UI. Features Extension Point Pattern for UI integration (menus, toolbars, dock widgets, context menus, status bar), automatic UI element lifecycle management, PluginManager singleton, lifecycle management, command pattern for operations, enhanced MVC components (PluginModel, PluginConfigModel, PluginListDelegate), ConfigurePluginCommand with undo/redo support, and full internationalization (English/Chinese).

[Specialized Plugin Interfaces](feature/specialized-plugin-interfaces.md): Five domain-specific plugin interfaces for document processing, rendering, search enhancement, cache strategies, and annotations. Includes PluginHookRegistry system with 20+ predefined workflow hooks, standardized result types (DocumentProcessingResult, PluginSearchResult, AnnotationData), and thread-safe filter execution with priority ordering.

[Accessibility System](feature/accessibility-system.md): Comprehensive accessibility support implementing MVP architecture with screen reader mode, high contrast themes, text-to-speech engine integration, text enlargement, motion reduction, and enhanced keyboard navigation. Features full QTextToSpeech support, EventBus integration for decoupled communication, announcement queue system, and complete settings persistence via QSettings and JSON import/export. Composed of layers: [AccessibilityModel](feature/accessibility-model.md) for state management, [AccessibilityController](feature/accessibility-controller.md) for business logic coordination, and [AccessibilityCommands](feature/accessibility-commands.md) for undo/redo support with 15+ command classes.

[Cache System MVP Architecture](feature/cache-mvp-architecture.md): Cache component refactored into Model-View-Presenter architecture with four specialized models (CacheEntryModel for entry metadata, CacheDataModel for storage/retrieval, CacheConfigModel for settings, CacheStatsModel for metrics), view interfaces (ICacheView, ICacheStatsView, ICacheConfigView, ICacheMemoryView), and CachePresenter for orchestration. Includes performance fixes for timer callback overlap prevention and elapsed time tracking in periodic operations.

## Component Analysis & Investigations

[PDF Viewer Components Investigation Summary](agent/INVESTIGATION-SUMMARY.md): Executive summary of comprehensive analysis of 7 PDF viewer components including completeness assessment, performance optimizations, and integration verification.

[PDF Viewer Implementation Analysis](agent/pdf-viewer-components-implementation-analysis.md): Detailed component-by-component analysis of PDFViewer, QGraphicsPDFViewer, PDFPrerenderer, PDFOutlineWidget, PDFAnimations, PDFViewerComponents, and SplitViewManager with rendering pipeline verification and integration mapping.

[PDF Viewer Verification Checklist](agent/pdf-viewer-detailed-verification-checklist.md): Point-by-point verification of implementation completeness including rendering quality, performance optimizations, state management, and error handling across all viewer components.

[PDF Viewer Code Examples](agent/pdf-viewer-code-implementation-examples.md): Real code snippets and implementation details from the PDF viewer system demonstrating rendering pipelines, caching strategies, virtual scrolling, search optimization, gesture handling, and error recovery mechanisms.

[Application Freeze Issue: Timer and Threading Analysis](agent/timer-and-threading-freeze-analysis.md): Comprehensive investigation of 40+ QTimer instances, mutex patterns, thread synchronization, and blocking operations across the codebase. Documents 10 critical findings on freeze risk patterns including blocking waits, timer contention, deadlock scenarios, and excessive timer firing frequencies.

## Standard Operating Procedures (SOPs)

[Build System Compilation Fixes](sop/build-compilation-fixes.md): Documentation of critical build fixes applied to resolve compilation errors in action mappings, type casts, field naming, logging macros, and missing includes.
