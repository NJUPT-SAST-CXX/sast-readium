#include "TestUtilities.h"
#include <QApplication>
#include <QDebug>

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
