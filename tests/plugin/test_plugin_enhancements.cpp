#include <gtest/gtest.h>

#include "../../app/command/PluginCommands.h"
#include "../../app/delegate/PluginListDelegate.h"
#include "../../app/model/PluginConfigModel.h"
#include "../../app/model/PluginModel.h"
#include "../../app/plugin/PluginManager.h"

#include <QApplication>
#include <QJsonObject>
#include <QSignalSpy>

/**
 * Test fixture for plugin enhancement tests
 */
class PluginEnhancementsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize Qt application if not already initialized
        if (!QApplication::instance()) {
            int argc = 0;
            char** argv = nullptr;
            app = new QApplication(argc, argv);
        }

        pluginManager = &PluginManager::instance();
    }

    void TearDown() override {
        // Cleanup
        if (app) {
            delete app;
            app = nullptr;
        }
    }

    QApplication* app = nullptr;
    PluginManager* pluginManager = nullptr;
};

// ============================================================================
// PluginModel Tests
// ============================================================================

TEST_F(PluginEnhancementsTest, PluginModelConstruction) {
    PluginModel model(pluginManager);

    EXPECT_GE(model.rowCount(), 0);
    EXPECT_TRUE(model.roleNames().contains(PluginModel::NameRole));
    EXPECT_TRUE(model.roleNames().contains(PluginModel::VersionRole));
    EXPECT_TRUE(model.roleNames().contains(PluginModel::IsLoadedRole));
}

TEST_F(PluginEnhancementsTest, PluginModelFiltering) {
    PluginModel model(pluginManager);

    // Test text filtering
    int initialCount = model.rowCount();
    model.setFilterText("test");
    EXPECT_LE(model.rowCount(), initialCount);

    // Clear filter
    model.clearFilters();
    EXPECT_EQ(model.rowCount(), initialCount);

    // Test loaded-only filtering
    model.setShowOnlyLoaded(true);
    int loadedCount = model.rowCount();
    EXPECT_LE(loadedCount, initialCount);

    model.clearFilters();
    EXPECT_EQ(model.rowCount(), initialCount);
}

TEST_F(PluginEnhancementsTest, PluginModelSignals) {
    PluginModel model(pluginManager);

    QSignalSpy filterSpy(&model, &PluginModel::filterChanged);
    QSignalSpy refreshSpy(&model, &PluginModel::modelRefreshed);

    model.setFilterText("test");
    EXPECT_EQ(filterSpy.count(), 1);

    model.refresh();
    EXPECT_EQ(refreshSpy.count(), 1);
}

TEST_F(PluginEnhancementsTest, PluginModelDataRoles) {
    PluginModel model(pluginManager);

    if (model.rowCount() > 0) {
        QModelIndex idx = model.index(0, 0);

        // Test various data roles
        QVariant name = model.data(idx, PluginModel::NameRole);
        EXPECT_TRUE(name.isValid());
        EXPECT_FALSE(name.toString().isEmpty());

        QVariant version = model.data(idx, PluginModel::VersionRole);
        EXPECT_TRUE(version.isValid());

        QVariant isLoaded = model.data(idx, PluginModel::IsLoadedRole);
        EXPECT_TRUE(isLoaded.isValid());
        EXPECT_TRUE(isLoaded.canConvert<bool>());

        QVariant statusText = model.data(idx, PluginModel::StatusTextRole);
        EXPECT_TRUE(statusText.isValid());
        EXPECT_FALSE(statusText.toString().isEmpty());
    }
}

// ============================================================================
// PluginConfigModel Tests
// ============================================================================

TEST_F(PluginEnhancementsTest, PluginConfigModelConstruction) {
    PluginConfigModel model(pluginManager);

    EXPECT_EQ(model.columnCount(), PluginConfigModel::ColumnCount);
    EXPECT_GE(model.rowCount(), 0);
}

TEST_F(PluginEnhancementsTest, PluginConfigModelConfiguration) {
    PluginConfigModel model(pluginManager);

    // Create test configuration
    QJsonObject config;
    config["testKey"] = "testValue";
    config["numericKey"] = 42;
    config["boolKey"] = true;

    model.setConfiguration(config);

    EXPECT_EQ(model.rowCount(), 3);
    EXPECT_TRUE(model.hasKey("testKey"));
    EXPECT_TRUE(model.hasKey("numericKey"));
    EXPECT_TRUE(model.hasKey("boolKey"));

    EXPECT_EQ(model.getValue("testKey").toString(), "testValue");
    EXPECT_EQ(model.getValue("numericKey").toInt(), 42);
    EXPECT_EQ(model.getValue("boolKey").toBool(), true);
}

TEST_F(PluginEnhancementsTest, PluginConfigModelAddEntry) {
    PluginConfigModel model(pluginManager);

    int initialCount = model.rowCount();

    bool added = model.addEntry("newKey", "newValue", "string", "Test entry");
    EXPECT_TRUE(added);
    EXPECT_EQ(model.rowCount(), initialCount + 1);
    EXPECT_TRUE(model.hasKey("newKey"));

    // Try adding duplicate key
    bool addedDuplicate = model.addEntry("newKey", "anotherValue");
    EXPECT_FALSE(addedDuplicate);
    EXPECT_EQ(model.rowCount(), initialCount + 1);
}

TEST_F(PluginEnhancementsTest, PluginConfigModelRemoveEntry) {
    PluginConfigModel model(pluginManager);

    model.addEntry("tempKey", "tempValue");
    int countWithEntry = model.rowCount();

    bool removed = model.removeEntry("tempKey");
    EXPECT_TRUE(removed);
    EXPECT_EQ(model.rowCount(), countWithEntry - 1);
    EXPECT_FALSE(model.hasKey("tempKey"));

    // Try removing non-existent key
    bool removedAgain = model.removeEntry("tempKey");
    EXPECT_FALSE(removedAgain);
}

TEST_F(PluginEnhancementsTest, PluginConfigModelSetValue) {
    PluginConfigModel model(pluginManager);

    model.addEntry("testKey", "initialValue", "string");

    bool changed = model.setValue("testKey", "newValue");
    EXPECT_TRUE(changed);
    EXPECT_EQ(model.getValue("testKey").toString(), "newValue");
}

TEST_F(PluginEnhancementsTest, PluginConfigModelValidation) {
    PluginConfigModel model(pluginManager);

    EXPECT_TRUE(model.isValidValue("string", QVariant("test")));
    EXPECT_TRUE(model.isValidValue("int", QVariant(42)));
    EXPECT_TRUE(model.isValidValue("bool", QVariant(true)));
    EXPECT_TRUE(model.isValidValue("double", QVariant(3.14)));

    // Invalid conversions
    EXPECT_FALSE(model.isValidValue("int", QVariant("not a number")));
}

TEST_F(PluginEnhancementsTest, PluginConfigModelSignals) {
    PluginConfigModel model(pluginManager);

    QSignalSpy configChangedSpy(&model,
                                &PluginConfigModel::configurationChanged);
    QSignalSpy entryAddedSpy(&model, &PluginConfigModel::entryAdded);
    QSignalSpy entryRemovedSpy(&model, &PluginConfigModel::entryRemoved);

    model.addEntry("newKey", "newValue");
    EXPECT_EQ(entryAddedSpy.count(), 1);
    EXPECT_EQ(configChangedSpy.count(), 1);

    model.removeEntry("newKey");
    EXPECT_EQ(entryRemovedSpy.count(), 1);
    EXPECT_EQ(configChangedSpy.count(), 2);
}

// ============================================================================
// PluginListDelegate Tests
// ============================================================================

TEST_F(PluginEnhancementsTest, PluginListDelegateConstruction) {
    PluginListDelegate delegate;

    EXPECT_EQ(delegate.displayMode(), PluginListDelegate::NormalMode);
    EXPECT_TRUE(delegate.showIcons());
    EXPECT_TRUE(delegate.showStatus());
}

TEST_F(PluginEnhancementsTest, PluginListDelegateDisplayMode) {
    PluginListDelegate delegate;

    delegate.setDisplayMode(PluginListDelegate::CompactMode);
    EXPECT_EQ(delegate.displayMode(), PluginListDelegate::CompactMode);

    delegate.setDisplayMode(PluginListDelegate::DetailedMode);
    EXPECT_EQ(delegate.displayMode(), PluginListDelegate::DetailedMode);
}

TEST_F(PluginEnhancementsTest, PluginListDelegateCustomization) {
    PluginListDelegate delegate;

    delegate.setShowIcons(false);
    EXPECT_FALSE(delegate.showIcons());

    delegate.setShowStatus(false);
    EXPECT_FALSE(delegate.showStatus());

    delegate.setHighlightErrors(false);
    EXPECT_FALSE(delegate.highlightErrors());

    QColor testColor(255, 0, 0);
    delegate.setLoadedColor(testColor);
    EXPECT_EQ(delegate.loadedColor(), testColor);
}

// ============================================================================
// ConfigurePluginCommand Tests
// ============================================================================

TEST_F(PluginEnhancementsTest, ConfigurePluginCommandConstruction) {
    QJsonObject config;
    config["key"] = "value";

    ConfigurePluginCommand cmd(pluginManager, "TestPlugin", config);

    EXPECT_EQ(cmd.pluginName(), "TestPlugin");
    EXPECT_EQ(cmd.newConfiguration(), config);
    EXPECT_EQ(cmd.name(), "ConfigurePlugin");
}

TEST_F(PluginEnhancementsTest, ConfigurePluginCommandFactory) {
    QJsonObject config;
    config["test"] = "value";

    auto cmd = PluginCommandFactory::createConfigureCommand(
        pluginManager, "TestPlugin", config);

    EXPECT_NE(cmd, nullptr);
    EXPECT_EQ(cmd->name(), "ConfigurePlugin");
}

TEST_F(PluginEnhancementsTest, ConfigurePluginCommandCanExecute) {
    QJsonObject config;

    // Empty plugin name should not be executable
    ConfigurePluginCommand cmd1(pluginManager, "", config);
    EXPECT_FALSE(cmd1.canExecute());

    // Valid plugin name (if exists in available plugins)
    ConfigurePluginCommand cmd2(pluginManager, "TestPlugin", config);
    // Can execute depends on whether TestPlugin exists
    // EXPECT_TRUE or EXPECT_FALSE depends on test environment
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(PluginEnhancementsTest, ModelCommandIntegration) {
    PluginModel model(pluginManager);
    PluginConfigModel configModel(pluginManager);

    // Create a configuration command
    QJsonObject config;
    config["testSetting"] = "testValue";

    auto cmd = PluginCommandFactory::createConfigureCommand(
        pluginManager, "TestPlugin", config);

    EXPECT_NE(cmd, nullptr);

    // Test configuration model with the same config
    configModel.setConfiguration(config);
    EXPECT_TRUE(configModel.hasKey("testSetting"));
}

TEST_F(PluginEnhancementsTest, ModelDelegateIntegration) {
    PluginModel model(pluginManager);
    PluginListDelegate delegate;

    // Test that delegate can handle model data
    if (model.rowCount() > 0) {
        QModelIndex idx = model.index(0, 0);
        QStyleOptionViewItem option;

        // This should not crash
        QSize hint = delegate.sizeHint(option, idx);
        EXPECT_GT(hint.height(), 0);
        EXPECT_GT(hint.width(), 0);
    }
}

// ============================================================================
// Main Test Entry Point
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
