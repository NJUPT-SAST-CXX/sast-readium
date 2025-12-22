#include "PluginHookPoint.h"

PluginHookPoint::PluginHookPoint(const QString& name, QObject* parent)
    : QObject(parent), m_name(name) {}

PluginHookPoint::~PluginHookPoint() = default;
