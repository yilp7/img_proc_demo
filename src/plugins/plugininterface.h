#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#include <QString>
#include <QWidget>

class AddonHost;

// Lifecycle shared by every addon, built in or loaded from a DLL. Each addon
// project carries a verbatim copy under sdk/; a change here is a change there.
class PluginInterface
{
public:
    virtual ~PluginInterface() {}
    // A widget addon builds its UI under the given host widget. An addon with
    // its own window uses the parent for ownership only.
    virtual void create(QWidget *parent) = 0;
    // The application's services and feeds. Once, after create().
    virtual void attach(AddonHost *host) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
};

QT_BEGIN_NAMESPACE

// Bumped with the lifecycle: a plugin built against the old one fails the
// cast instead of being called through a table it does not have.
#define AddonPluginInterface_iid "com.zkss.glass.addons.plugin-interface/2"

Q_DECLARE_INTERFACE(PluginInterface, AddonPluginInterface_iid)
QT_END_NAMESPACE

#endif // PLUGININTERFACE_H
