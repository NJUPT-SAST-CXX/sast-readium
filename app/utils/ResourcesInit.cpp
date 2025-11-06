#include "ResourcesInit.h"
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QtGlobal>
#include "../logging/LoggingMacros.h"

// Q_INIT_RESOURCE declarations for our qrc files
extern int QT_MANGLE_NAMESPACE(qInitResources_app)();
extern int QT_MANGLE_NAMESPACE(qInitResources_ela_ui)();

namespace SastResources {

void ensureInitialized() {
    static QMutex mtx;
    static bool initialized = false;
    QMutexLocker lock(&mtx);
    if (initialized) {
        return;
    }

    // Register both resource collections explicitly
    QT_MANGLE_NAMESPACE(qInitResources_app)();
    QT_MANGLE_NAMESPACE(qInitResources_ela_ui)();
// Debug check: verify a couple of resources are visible
#ifdef QT_DEBUG
    if (!QFile(":/images/filetypes/pdf.svg").exists()) {
        LOG_WARNING(
            "ResourcesInit: :/images/filetypes/pdf.svg not found after "
            "initialization");
    } else {
        LOG_DEBUG("ResourcesInit: app resources registered successfully");
    }
    if (!QFile(":/icons/app_icon").exists()) {
        LOG_WARNING(
            "ResourcesInit: :/icons/app_icon not found after initialization");
    }
#endif
    initialized = true;
}

}  // namespace SastResources
