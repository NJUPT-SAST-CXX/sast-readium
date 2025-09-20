#include "TestUtilities.h"
#include <QApplication>
#include <QDebug>

// TestBase implementation
// The TestBase class provides common functionality for all test cases
// Most methods are already implemented inline in the header, but we need
// this implementation file for the MOC system to work properly with Q_OBJECT

// MockObject implementation - most methods are already implemented inline in the header

// TestDataGenerator implementation - methods are already implemented inline in header

// Include the MOC file for Q_OBJECT classes
#include "TestUtilities.moc"
