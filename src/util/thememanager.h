#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QStringList>

class ThemeManager
{
public:
    struct Theme {
        QString name = "gruvbox";
        QString color_mode = "dark";
        QHash<QString, QString> tokens;
    };

    static ThemeManager& instance();

    QByteArray buildStyleSheet(const QString &theme_name, const QString &color_mode) const;
    bool loadTheme(const QString &theme_name, const QString &color_mode, Theme *theme) const;
    QStringList availableThemes() const;

    bool setCurrentTheme(const QString &theme_name, const QString &color_mode);
    QString token(const QString &name, const QString &fallback = QString()) const;
    QString currentThemeName() const { return m_current.name; }
    QString currentColorMode() const { return m_current.color_mode; }

private:
    ThemeManager() = default;

    QByteArray readResource(const QString &path) const;
    QByteArray applyTokens(QByteArray style, const Theme &theme) const;
    QString normalizedColorMode(const QString &color_mode) const;
    QString normalizedTokenName(const QString &name) const;

    Theme m_current;
};

#endif // THEMEMANAGER_H
