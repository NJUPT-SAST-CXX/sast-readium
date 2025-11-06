#pragma once

namespace SastResources {

// Ensure Qt resources from app.qrc and ela_ui.qrc are registered at runtime.
// Safe to call multiple times; it will only initialize once per process.
void ensureInitialized();

}  // namespace SastResources
