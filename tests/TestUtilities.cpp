#include "TestUtilities.h"
#include <QApplication>
#include <QDebug>
#include <QtGlobal>

// Ensure Qt resources from app.qrc and ela_ui.qrc are registered in every
// test process, even for tests that do not inherit from TestBase.
// We rely on TestUtilities being linked to all tests.
static void qt_resource_autoinit() { SastResources::ensureInitialized(); }
Q_CONSTRUCTOR_FUNCTION(qt_resource_autoinit)

// TestBase implementation
// The TestBase class provides common functionality for all test cases
// Most methods are already implemented inline in the header, but we need
// this implementation file for the MOC system to work properly with Q_OBJECT

// MockObject implementation - most methods are already implemented inline in
// the header

// TestDataGenerator implementation - methods are already implemented inline in
// header

// Note: AutoMoc automatically handles MOC file generation for Q_OBJECT classes
// No need to manually include .moc files
