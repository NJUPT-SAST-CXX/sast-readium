#pragma once

#include <QObject>
#include <QString>
#include "../../../app/plugin/PluginInterface.h"

/**
 * @brief HelloPlugin - Example plugin demonstrating plugin system usage
 *
 * This plugin demonstrates:
 * - Basic plugin structure and lifecycle
 * - Plugin metadata
 * - Event subscription
 * - Service access
 * - Configuration management
 */
class HelloPlugin : public PluginBase {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.sast.readium.IPlugin/1.0" FILE
                          "hello_plugin.json")
    Q_INTERFACES(IPluginInterface)

public:
    explicit HelloPlugin(QObject* parent = nullptr);
    ~HelloPlugin() override;

protected:
    // PluginBase overrides
    bool onInitialize() override;
    void onShutdown() override;

private slots:
    // Event handlers
    void onDocumentOpened(const QVariant& data);
    void onDocumentClosed(const QVariant& data);

private:
    void setupEventSubscriptions();
    void removeEventSubscriptions();
    void logMessage(const QString& message);

    // Plugin state
    int m_documentOpenCount;
};
