#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include "model/PluginConfigModel.h"
#include "plugin/PluginManager.h"

/**
 * @brief Test suite for Plugin Configuration System
 *
 * Tests the enhanced PluginConfigModel with schema support,
 * including groups, constraints, required fields, and validation.
 */
class TestPluginConfig : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // ConfigEntry tests
    void testConfigEntryDefaultConstruction();
    void testConfigEntryValueConstruction();

    // Schema parsing tests
    void testSetConfigSchema();
    void testParseGroupsFromSchema();
    void testBuildConfigEntriesFromSchema();

    // Group management tests
    void testGetEntriesForGroup();
    void testGetGroupIds();
    void testAddGroup();

    // Required configuration tests
    void testGetRequiredEntries();
    void testHasRequiredUnset();
    void testGetRequiredUnsetKeys();

    // Validation tests
    void testValidateEntryType();
    void testValidateEntryRange();
    void testValidateEntryEnum();
    void testValidateEntryRequired();
    void testValidateAllEntries();

    // Type detection tests
    void testDetectTypeBool();
    void testDetectTypeInt();
    void testDetectTypeDouble();
    void testDetectTypeString();

private:
    QJsonObject createTestSchema();
    PluginConfigModel* m_model{nullptr};
};

void TestPluginConfig::initTestCase() {
    // Create model without plugin manager for isolated testing
    m_model = new PluginConfigModel(nullptr, "TestPlugin");
}

void TestPluginConfig::cleanupTestCase() {
    delete m_model;
    m_model = nullptr;
}

QJsonObject TestPluginConfig::createTestSchema() {
    QJsonObject schema;

    // Groups
    QJsonObject groups;

    QJsonObject generalGroup;
    generalGroup["displayName"] = "General";
    generalGroup["description"] = "General settings";
    generalGroup["order"] = 0;
    groups["general"] = generalGroup;

    QJsonObject advancedGroup;
    advancedGroup["displayName"] = "Advanced";
    advancedGroup["description"] = "Advanced settings";
    advancedGroup["order"] = 1;
    advancedGroup["advanced"] = true;
    groups["advanced"] = advancedGroup;

    schema["groups"] = groups;

    // Properties
    QJsonObject properties;

    QJsonObject nameProp;
    nameProp["type"] = "string";
    nameProp["displayName"] = "Name";
    nameProp["description"] = "Plugin name";
    nameProp["default"] = "Default Name";
    nameProp["group"] = "general";
    nameProp["required"] = true;
    nameProp["order"] = 0;
    properties["name"] = nameProp;

    QJsonObject enabledProp;
    enabledProp["type"] = "bool";
    enabledProp["displayName"] = "Enabled";
    enabledProp["default"] = true;
    enabledProp["group"] = "general";
    enabledProp["order"] = 1;
    properties["enabled"] = enabledProp;

    QJsonObject maxItemsProp;
    maxItemsProp["type"] = "int";
    maxItemsProp["displayName"] = "Max Items";
    maxItemsProp["default"] = 100;
    maxItemsProp["minimum"] = 1;
    maxItemsProp["maximum"] = 1000;
    maxItemsProp["group"] = "advanced";
    maxItemsProp["order"] = 0;
    properties["maxItems"] = maxItemsProp;

    QJsonObject modeProp;
    modeProp["type"] = "string";
    modeProp["displayName"] = "Mode";
    modeProp["default"] = "normal";
    QJsonArray enumValues;
    enumValues.append("normal");
    enumValues.append("compact");
    enumValues.append("expanded");
    modeProp["enum"] = enumValues;
    modeProp["group"] = "general";
    modeProp["order"] = 2;
    properties["mode"] = modeProp;

    schema["properties"] = properties;

    return schema;
}

// ============================================================================
// ConfigEntry Tests
// ============================================================================

void TestPluginConfig::testConfigEntryDefaultConstruction() {
    PluginConfigModel::ConfigEntry entry;

    QCOMPARE(entry.key, QString());
    QVERIFY(entry.value.isNull());
    QCOMPARE(entry.isRequired, false);
    QCOMPARE(entry.isReadOnly, false);
    QCOMPARE(entry.order, 0);
}

void TestPluginConfig::testConfigEntryValueConstruction() {
    PluginConfigModel::ConfigEntry entry("testKey", "testValue", "string",
                                         "Test description", false);

    QCOMPARE(entry.key, QString("testKey"));
    QCOMPARE(entry.value.toString(), QString("testValue"));
    QCOMPARE(entry.type, QString("string"));
    QCOMPARE(entry.description, QString("Test description"));
    QCOMPARE(entry.isReadOnly, false);
    QCOMPARE(entry.group, QString("general"));
}

// ============================================================================
// Schema Parsing Tests
// ============================================================================

void TestPluginConfig::testSetConfigSchema() {
    QJsonObject schema = createTestSchema();
    m_model->setConfigSchema(schema);

    QVERIFY(m_model->hasSchema());
    QCOMPARE(m_model->getConfigSchema(), schema);
}

void TestPluginConfig::testParseGroupsFromSchema() {
    QJsonObject schema = createTestSchema();
    m_model->setConfigSchema(schema);

    QList<PluginConfigModel::ConfigGroup> groups = m_model->getGroups();
    QCOMPARE(groups.size(), 2);

    // Groups should be sorted by order
    QCOMPARE(groups[0].id, QString("general"));
    QCOMPARE(groups[0].displayName, QString("General"));
    QCOMPARE(groups[0].order, 0);

    QCOMPARE(groups[1].id, QString("advanced"));
    QCOMPARE(groups[1].displayName, QString("Advanced"));
    QCOMPARE(groups[1].isAdvanced, true);
}

void TestPluginConfig::testBuildConfigEntriesFromSchema() {
    QJsonObject schema = createTestSchema();
    m_model->setConfigSchema(schema);

    QCOMPARE(m_model->rowCount(), 4);

    // Check that entries exist
    QVERIFY(m_model->hasKey("name"));
    QVERIFY(m_model->hasKey("enabled"));
    QVERIFY(m_model->hasKey("maxItems"));
    QVERIFY(m_model->hasKey("mode"));

    // Check default values
    QCOMPARE(m_model->getValue("name").toString(), QString("Default Name"));
    QCOMPARE(m_model->getValue("enabled").toBool(), true);
    QCOMPARE(m_model->getValue("maxItems").toInt(), 100);
    QCOMPARE(m_model->getValue("mode").toString(), QString("normal"));
}

// ============================================================================
// Group Management Tests
// ============================================================================

void TestPluginConfig::testGetEntriesForGroup() {
    QJsonObject schema = createTestSchema();
    m_model->setConfigSchema(schema);

    QVector<PluginConfigModel::ConfigEntry> generalEntries =
        m_model->getEntriesForGroup("general");
    QCOMPARE(generalEntries.size(), 3);  // name, enabled, mode

    QVector<PluginConfigModel::ConfigEntry> advancedEntries =
        m_model->getEntriesForGroup("advanced");
    QCOMPARE(advancedEntries.size(), 1);  // maxItems
}

void TestPluginConfig::testGetGroupIds() {
    QJsonObject schema = createTestSchema();
    m_model->setConfigSchema(schema);

    QStringList groupIds = m_model->getGroupIds();
    QCOMPARE(groupIds.size(), 2);
    QVERIFY(groupIds.contains("general"));
    QVERIFY(groupIds.contains("advanced"));
}

void TestPluginConfig::testAddGroup() {
    QJsonObject schema = createTestSchema();
    m_model->setConfigSchema(schema);

    PluginConfigModel::ConfigGroup newGroup("custom", "Custom Group",
                                            "Custom description", 50);
    m_model->addGroup(newGroup);

    QList<PluginConfigModel::ConfigGroup> groups = m_model->getGroups();
    QCOMPARE(groups.size(), 3);

    // Find the new group (should be between general and advanced based on
    // order)
    bool found = false;
    for (const auto& group : groups) {
        if (group.id == "custom") {
            found = true;
            QCOMPARE(group.displayName, QString("Custom Group"));
            QCOMPARE(group.order, 50);
        }
    }
    QVERIFY(found);
}

// ============================================================================
// Required Configuration Tests
// ============================================================================

void TestPluginConfig::testGetRequiredEntries() {
    QJsonObject schema = createTestSchema();
    m_model->setConfigSchema(schema);

    QVector<PluginConfigModel::ConfigEntry> requiredEntries =
        m_model->getRequiredEntries();

    QCOMPARE(requiredEntries.size(), 1);
    QCOMPARE(requiredEntries[0].key, QString("name"));
}

void TestPluginConfig::testHasRequiredUnset() {
    QJsonObject schema = createTestSchema();
    m_model->setConfigSchema(schema);

    // With default values, required fields should be set
    QCOMPARE(m_model->hasRequiredUnset(), false);

    // Clear the required field
    m_model->setValue("name", QString());
    QCOMPARE(m_model->hasRequiredUnset(), true);
}

void TestPluginConfig::testGetRequiredUnsetKeys() {
    QJsonObject schema = createTestSchema();
    m_model->setConfigSchema(schema);

    // Clear the required field
    m_model->setValue("name", QString());

    QStringList unsetKeys = m_model->getRequiredUnsetKeys();
    QCOMPARE(unsetKeys.size(), 1);
    QVERIFY(unsetKeys.contains("name"));
}

// ============================================================================
// Validation Tests
// ============================================================================

void TestPluginConfig::testValidateEntryType() {
    QVERIFY(m_model->isValidValue("bool", true));
    QVERIFY(m_model->isValidValue("bool", false));
    QVERIFY(m_model->isValidValue("int", 42));
    QVERIFY(m_model->isValidValue("double", 3.14));
    QVERIFY(m_model->isValidValue("string", QString("test")));
}

void TestPluginConfig::testValidateEntryRange() {
    QJsonObject schema = createTestSchema();
    m_model->setConfigSchema(schema);

    // Valid value within range
    m_model->setValue("maxItems", 500);
    QStringList errors = m_model->validateAllEntries();
    QVERIFY(!errors.contains("Max Items"));

    // Value below minimum
    m_model->setValue("maxItems", 0);
    errors = m_model->validateAllEntries();
    QVERIFY(errors.size() > 0);

    // Value above maximum
    m_model->setValue("maxItems", 2000);
    errors = m_model->validateAllEntries();
    QVERIFY(errors.size() > 0);

    // Reset to valid value
    m_model->setValue("maxItems", 100);
}

void TestPluginConfig::testValidateEntryEnum() {
    QJsonObject schema = createTestSchema();
    m_model->setConfigSchema(schema);

    // Valid enum value
    m_model->setValue("mode", QString("compact"));
    QStringList errors = m_model->validateAllEntries();
    bool hasEnumError = false;
    for (const QString& error : errors) {
        if (error.contains("mode") || error.contains("Mode")) {
            hasEnumError = true;
        }
    }
    QVERIFY(!hasEnumError);

    // Invalid enum value
    m_model->setValue("mode", QString("invalid"));
    errors = m_model->validateAllEntries();
    hasEnumError = false;
    for (const QString& error : errors) {
        if (error.contains("must be one of")) {
            hasEnumError = true;
        }
    }
    QVERIFY(hasEnumError);

    // Reset to valid value
    m_model->setValue("mode", QString("normal"));
}

void TestPluginConfig::testValidateEntryRequired() {
    QJsonObject schema = createTestSchema();
    m_model->setConfigSchema(schema);

    // Clear required field
    m_model->setValue("name", QString());

    QStringList errors = m_model->validateAllEntries();
    QVERIFY(errors.size() > 0);

    bool hasRequiredError = false;
    for (const QString& error : errors) {
        if (error.contains("Required") || error.contains("not set")) {
            hasRequiredError = true;
        }
    }
    QVERIFY(hasRequiredError);
}

void TestPluginConfig::testValidateAllEntries() {
    QJsonObject schema = createTestSchema();
    m_model->setConfigSchema(schema);

    // All valid values
    QStringList errors = m_model->validateAllEntries();
    QCOMPARE(errors.size(), 0);

    // Introduce errors
    m_model->setValue("name", QString());       // Required field empty
    m_model->setValue("maxItems", 0);           // Below minimum
    m_model->setValue("mode", QString("bad"));  // Invalid enum

    errors = m_model->validateAllEntries();
    QVERIFY(errors.size() >= 3);
}

// ============================================================================
// Type Detection Tests
// ============================================================================

void TestPluginConfig::testDetectTypeBool() {
    // This test indirectly tests detectType through addEntry
    m_model->addEntry("testBool", true);
    QCOMPARE(m_model->getType("testBool"), QString("bool"));
    m_model->removeEntry("testBool");
}

void TestPluginConfig::testDetectTypeInt() {
    m_model->addEntry("testInt", 42);
    QCOMPARE(m_model->getType("testInt"), QString("int"));
    m_model->removeEntry("testInt");
}

void TestPluginConfig::testDetectTypeDouble() {
    m_model->addEntry("testDouble", 3.14);
    QCOMPARE(m_model->getType("testDouble"), QString("double"));
    m_model->removeEntry("testDouble");
}

void TestPluginConfig::testDetectTypeString() {
    m_model->addEntry("testString", QString("hello"));
    QCOMPARE(m_model->getType("testString"), QString("string"));
    m_model->removeEntry("testString");
}

QTEST_MAIN(TestPluginConfig)
#include "test_plugin_config.moc"
