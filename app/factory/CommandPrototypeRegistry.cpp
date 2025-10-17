#include "CommandFactory.h"

// This translation unit intentionally delegates to definitions in CommandFactory.cpp
// to avoid duplication and multiple-definition issues. Keeping a valid TU ensures
// build systems that expect this file still compile successfully.

// Ensure the linker pulls in CommandFactory.cpp by referencing a symbol
namespace { volatile int force_link_CommandPrototypeRegistry = 0; }