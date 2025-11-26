#pragma once

#include <QWidget>

class QVBoxLayout;
class QStandardItemModel;
class ElaTableView;
class ElaToggleSwitch;
class ElaLineEdit;
class ElaPushButton;
class ElaText;

class PluginSettingsWidget : public QWidget {
    Q_OBJECT

public:
    explicit PluginSettingsWidget(QWidget* parent = nullptr);
    ~PluginSettingsWidget() override;

    void loadSettings();
    void saveSettings();
    void resetToDefaults();
    void refreshPluginList();

signals:
    void settingsChanged();

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void onAddDirectory();
    void onRemoveDirectory();
    void onHotReloadToggled(bool enabled);
    void onPluginToggled(int row, int column);
    void onPluginSelected(int row, int column);

private:
    void setupUi();
    void retranslateUi();
    void populatePluginTable();

    QVBoxLayout* m_mainLayout;
    ElaLineEdit* m_directoryEdit;
    ElaPushButton* m_addDirBtn;
    ElaPushButton* m_removeDirBtn;
    ElaTableView* m_directoriesTable;
    QStandardItemModel* m_directoriesModel;
    ElaToggleSwitch* m_hotReloadSwitch;
    ElaTableView* m_pluginsTable;
    QStandardItemModel* m_pluginsModel;
    ElaText* m_pluginInfoText;
    ElaPushButton* m_refreshBtn;
};
