#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#include <QString>

class PluginInterface
{
public:
    virtual ~PluginInterface() {}
    virtual void start() = 0;
};

QT_BEGIN_NAMESPACE

#define IRVisibleLightInterface_iid "com.zkss.glass.addons.ir-visible-light"

Q_DECLARE_INTERFACE(PluginInterface, IRVisibleLightInterface_iid)
QT_END_NAMESPACE

#endif // PLUGININTERFACE_H
