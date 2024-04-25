#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#include <QString>
#include <QWidget>

class PluginInterface
{
public:
    virtual ~PluginInterface() {}
    virtual void create(QWidget *parent) = 0;
    virtual void start() = 0;
};

QT_BEGIN_NAMESPACE

#define HikControlInterface_iid "com.zkss.glass.addons.hik-control"

Q_DECLARE_INTERFACE(PluginInterface, HikControlInterface_iid)
QT_END_NAMESPACE

#endif // PLUGININTERFACE_H
