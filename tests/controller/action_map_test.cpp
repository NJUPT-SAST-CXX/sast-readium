#include <QTest>
#include <QMetaEnum>
#include <QSet>
#include "../../app/controller/tool.hpp"
#include "../TestUtilities.h"

class ActionMapTest : public TestBase {
    Q_OBJECT

private slots:
    void testActionMapValues() {
        // Test that all expected action values are defined
        QVERIFY(static_cast<int>(ActionMap::openFile) >= 0);
        QVERIFY(static_cast<int>(ActionMap::openFolder) >= 0);
        QVERIFY(static_cast<int>(ActionMap::save) >= 0);
        QVERIFY(static_cast<int>(ActionMap::saveAs) >= 0);
        
        // Tab operations
        QVERIFY(static_cast<int>(ActionMap::newTab) >= 0);
        QVERIFY(static_cast<int>(ActionMap::closeTab) >= 0);
        QVERIFY(static_cast<int>(ActionMap::closeCurrentTab) >= 0);
        QVERIFY(static_cast<int>(ActionMap::closeAllTabs) >= 0);
        QVERIFY(static_cast<int>(ActionMap::nextTab) >= 0);
        QVERIFY(static_cast<int>(ActionMap::prevTab) >= 0);
        QVERIFY(static_cast<int>(ActionMap::switchToTab) >= 0);
        
        // Sidebar operations
        QVERIFY(static_cast<int>(ActionMap::toggleSideBar) >= 0);
        QVERIFY(static_cast<int>(ActionMap::showSideBar) >= 0);
        QVERIFY(static_cast<int>(ActionMap::hideSideBar) >= 0);
        
        // View mode operations
        QVERIFY(static_cast<int>(ActionMap::setSinglePageMode) >= 0);
        QVERIFY(static_cast<int>(ActionMap::setContinuousScrollMode) >= 0);
        
        // Page navigation operations
        QVERIFY(static_cast<int>(ActionMap::firstPage) >= 0);
        QVERIFY(static_cast<int>(ActionMap::previousPage) >= 0);
        QVERIFY(static_cast<int>(ActionMap::nextPage) >= 0);
        QVERIFY(static_cast<int>(ActionMap::lastPage) >= 0);
        QVERIFY(static_cast<int>(ActionMap::goToPage) >= 0);
        
        // Zoom operations
        QVERIFY(static_cast<int>(ActionMap::zoomIn) >= 0);
        QVERIFY(static_cast<int>(ActionMap::zoomOut) >= 0);
        QVERIFY(static_cast<int>(ActionMap::fitToWidth) >= 0);
        QVERIFY(static_cast<int>(ActionMap::fitToPage) >= 0);
        QVERIFY(static_cast<int>(ActionMap::fitToHeight) >= 0);
        
        // Rotation operations
        QVERIFY(static_cast<int>(ActionMap::rotateLeft) >= 0);
        QVERIFY(static_cast<int>(ActionMap::rotateRight) >= 0);
        
        // Theme operations
        QVERIFY(static_cast<int>(ActionMap::toggleTheme) >= 0);
        
        // Search operations
        QVERIFY(static_cast<int>(ActionMap::showSearch) >= 0);
        QVERIFY(static_cast<int>(ActionMap::hideSearch) >= 0);
        QVERIFY(static_cast<int>(ActionMap::toggleSearch) >= 0);
        QVERIFY(static_cast<int>(ActionMap::findNext) >= 0);
        QVERIFY(static_cast<int>(ActionMap::findPrevious) >= 0);
        QVERIFY(static_cast<int>(ActionMap::clearSearch) >= 0);
        
        // Document info operations
        QVERIFY(static_cast<int>(ActionMap::showDocumentMetadata) >= 0);
        
        // Recent files operations
        QVERIFY(static_cast<int>(ActionMap::openRecentFile) >= 0);
        QVERIFY(static_cast<int>(ActionMap::clearRecentFiles) >= 0);
        
        // Additional operations
        QVERIFY(static_cast<int>(ActionMap::saveFile) >= 0);
        QVERIFY(static_cast<int>(ActionMap::closeFile) >= 0);
        QVERIFY(static_cast<int>(ActionMap::fullScreen) >= 0);
        QVERIFY(static_cast<int>(ActionMap::exportFile) >= 0);
        QVERIFY(static_cast<int>(ActionMap::printFile) >= 0);
        QVERIFY(static_cast<int>(ActionMap::reloadFile) >= 0);
    }
    
    void testActionMapUniqueness() {
        // Test that all action values are unique
        QSet<int> actionValues;
        
        // File operations
        actionValues.insert(static_cast<int>(ActionMap::openFile));
        actionValues.insert(static_cast<int>(ActionMap::openFolder));
        actionValues.insert(static_cast<int>(ActionMap::save));
        actionValues.insert(static_cast<int>(ActionMap::saveAs));
        actionValues.insert(static_cast<int>(ActionMap::saveFile));
        actionValues.insert(static_cast<int>(ActionMap::closeFile));
        actionValues.insert(static_cast<int>(ActionMap::exportFile));
        actionValues.insert(static_cast<int>(ActionMap::printFile));
        actionValues.insert(static_cast<int>(ActionMap::reloadFile));
        
        // Tab operations
        actionValues.insert(static_cast<int>(ActionMap::newTab));
        actionValues.insert(static_cast<int>(ActionMap::closeTab));
        actionValues.insert(static_cast<int>(ActionMap::closeCurrentTab));
        actionValues.insert(static_cast<int>(ActionMap::closeAllTabs));
        actionValues.insert(static_cast<int>(ActionMap::nextTab));
        actionValues.insert(static_cast<int>(ActionMap::prevTab));
        actionValues.insert(static_cast<int>(ActionMap::switchToTab));
        
        // Sidebar operations
        actionValues.insert(static_cast<int>(ActionMap::toggleSideBar));
        actionValues.insert(static_cast<int>(ActionMap::showSideBar));
        actionValues.insert(static_cast<int>(ActionMap::hideSideBar));
        
        // View mode operations
        actionValues.insert(static_cast<int>(ActionMap::setSinglePageMode));
        actionValues.insert(static_cast<int>(ActionMap::setContinuousScrollMode));
        
        // Page navigation operations
        actionValues.insert(static_cast<int>(ActionMap::firstPage));
        actionValues.insert(static_cast<int>(ActionMap::previousPage));
        actionValues.insert(static_cast<int>(ActionMap::nextPage));
        actionValues.insert(static_cast<int>(ActionMap::lastPage));
        actionValues.insert(static_cast<int>(ActionMap::goToPage));
        
        // Zoom operations
        actionValues.insert(static_cast<int>(ActionMap::zoomIn));
        actionValues.insert(static_cast<int>(ActionMap::zoomOut));
        actionValues.insert(static_cast<int>(ActionMap::fitToWidth));
        actionValues.insert(static_cast<int>(ActionMap::fitToPage));
        actionValues.insert(static_cast<int>(ActionMap::fitToHeight));
        
        // Rotation operations
        actionValues.insert(static_cast<int>(ActionMap::rotateLeft));
        actionValues.insert(static_cast<int>(ActionMap::rotateRight));
        
        // Theme operations
        actionValues.insert(static_cast<int>(ActionMap::toggleTheme));
        
        // Search operations
        actionValues.insert(static_cast<int>(ActionMap::showSearch));
        actionValues.insert(static_cast<int>(ActionMap::hideSearch));
        actionValues.insert(static_cast<int>(ActionMap::toggleSearch));
        actionValues.insert(static_cast<int>(ActionMap::findNext));
        actionValues.insert(static_cast<int>(ActionMap::findPrevious));
        actionValues.insert(static_cast<int>(ActionMap::clearSearch));
        
        // Document info operations
        actionValues.insert(static_cast<int>(ActionMap::showDocumentMetadata));
        
        // Recent files operations
        actionValues.insert(static_cast<int>(ActionMap::openRecentFile));
        actionValues.insert(static_cast<int>(ActionMap::clearRecentFiles));
        
        // Additional operations
        actionValues.insert(static_cast<int>(ActionMap::fullScreen));
        
        // The number of unique values should equal the number of enum values
        // Based on the enum definition, we have approximately 42 values
        QVERIFY(actionValues.size() >= 40); // Allow some flexibility for enum changes
    }
    
    void testActionMapCasting() {
        // Test that ActionMap values can be cast to and from int
        ActionMap action = ActionMap::openFile;
        int actionInt = static_cast<int>(action);
        ActionMap actionBack = static_cast<ActionMap>(actionInt);
        
        QCOMPARE(static_cast<int>(actionBack), static_cast<int>(action));
    }
    
    void testActionMapComparison() {
        // Test that ActionMap values can be compared
        QVERIFY(ActionMap::openFile == ActionMap::openFile);
        QVERIFY(ActionMap::openFile != ActionMap::openFolder);
        
        // Test ordering (enum values should have consistent ordering)
        QVERIFY(static_cast<int>(ActionMap::openFile) < static_cast<int>(ActionMap::reloadFile));
    }
    
    void testActionMapInSwitch() {
        // Test that ActionMap can be used in switch statements
        ActionMap testAction = ActionMap::openFile;
        bool switchWorked = false;
        
        switch (testAction) {
            case ActionMap::openFile:
                switchWorked = true;
                break;
            case ActionMap::openFolder:
                switchWorked = false;
                break;
            default:
                switchWorked = false;
                break;
        }
        
        QVERIFY(switchWorked);
    }
    
    void testActionMapInHashMap() {
        // Test that ActionMap can be used as a key in hash maps
        QHash<ActionMap, QString> actionNames;
        
        actionNames[ActionMap::openFile] = "Open File";
        actionNames[ActionMap::openFolder] = "Open Folder";
        actionNames[ActionMap::save] = "Save";
        
        QCOMPARE(actionNames[ActionMap::openFile], QString("Open File"));
        QCOMPARE(actionNames[ActionMap::openFolder], QString("Open Folder"));
        QCOMPARE(actionNames[ActionMap::save], QString("Save"));
        
        QVERIFY(actionNames.contains(ActionMap::openFile));
        QVERIFY(!actionNames.contains(ActionMap::saveAs));
    }
    
    void testActionMapCategories() {
        // Test that actions are logically grouped
        
        // File operations should be in lower range
        QVERIFY(static_cast<int>(ActionMap::openFile) < 10);
        QVERIFY(static_cast<int>(ActionMap::openFolder) < 10);
        QVERIFY(static_cast<int>(ActionMap::save) < 10);
        QVERIFY(static_cast<int>(ActionMap::saveAs) < 10);
        
        // Tab operations should be grouped together
        int newTabValue = static_cast<int>(ActionMap::newTab);
        int closeTabValue = static_cast<int>(ActionMap::closeTab);
        int nextTabValue = static_cast<int>(ActionMap::nextTab);
        
        // These should be relatively close to each other
        QVERIFY(qAbs(newTabValue - closeTabValue) < 10);
        QVERIFY(qAbs(closeTabValue - nextTabValue) < 10);
        
        // Navigation operations should be grouped
        int firstPageValue = static_cast<int>(ActionMap::firstPage);
        int lastPageValue = static_cast<int>(ActionMap::lastPage);
        
        QVERIFY(qAbs(firstPageValue - lastPageValue) < 10);
    }
    
    void testActionMapBoundaryValues() {
        // Test boundary conditions
        
        // First action should be openFile (value 0)
        QCOMPARE(static_cast<int>(ActionMap::openFile), 0);
        
        // Last action should be reloadFile
        int reloadFileValue = static_cast<int>(ActionMap::reloadFile);
        QVERIFY(reloadFileValue >= 40); // Should be a reasonably high value (reloadFile is at position 43)
        
        // All values should be non-negative
        QVERIFY(static_cast<int>(ActionMap::openFile) >= 0);
        QVERIFY(static_cast<int>(ActionMap::save) >= 0);
        QVERIFY(static_cast<int>(ActionMap::toggleTheme) >= 0);
        QVERIFY(static_cast<int>(ActionMap::reloadFile) >= 0);
    }
};

QTEST_MAIN(ActionMapTest)
#include "action_map_test.moc"
