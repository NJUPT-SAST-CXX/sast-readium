# Plugin System

The SAST Readium Plugin System allows developers to extend the application's functionality without modifying the core codebase. This section provides comprehensive documentation for developing, integrating, and managing plugins.

## Documentation

### [Development Guide](development-guide.md)

A step-by-step guide to creating your first plugin:

- **Project Structure**: How to set up your plugin project.
- **Lifecycle**: Understanding plugin loading, initialization, and shutdown.
- **API Basics**: accessing core services and handling events.
- **Metadata**: Configuring plugin properties and dependencies.

### [Architecture](architecture.md)

Deep dive into the enhanced plugin system architecture:

- **PluginModel**: Qt model for plugin management UI.
- **Configuration**: Managing plugin settings with `PluginConfigModel`.
- **Commands**: Undo/Redo support for plugin operations.
- **I18n**: Internationalization support for plugins.

### [UI Integration](ui-integration.md)

Learn how to integrate your plugin with the application UI:

- **Menus & Toolbars**: Adding custom actions to the main window.
- **Context Menus**: Extending right-click menus.
- **Status Bar**: Displaying status information.
- **Dock Widgets**: Adding custom panels (future support).

### [Specialized Plugins](specialized-plugins.md)

Develop specialized plugins for deep integration:

- **Document Processors**: Transform and analyze documents.
- **Rendering Plugins**: Apply filters, watermarks, and overlays.
- **Search Enhancements**: Custom search algorithms.
- **Cache Strategies**: Optimize performance with custom caching.
- **Annotations**: Custom annotation types.

## Quick Links

- [Example Plugins](../../examples/plugins/)
- [API Reference](../api-reference.md)
