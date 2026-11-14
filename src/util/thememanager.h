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

    static ThemeManager &instance();

    QByteArray build_style_sheet(const QString &theme_name, const QString &color_mode) const;
    bool load_theme(const QString &theme_name, const QString &color_mode, Theme *theme) const;
    QStringList available_themes() const;

    bool set_current_theme(const QString &theme_name, const QString &color_mode);
    QString token(const QString &name, const QString &fallback = QString()) const;
    QString get_current_theme_name() const { return m_current.name; }
    QString get_current_color_mode() const { return m_current.color_mode; }

private:
    ThemeManager() = default;

    QByteArray read_resource(const QString &path) const;
    QByteArray apply_tokens(QByteArray style, const Theme &theme) const;
    QString normalized_color_mode(const QString &color_mode) const;
    QString normalized_token_name(const QString &name) const;

    Theme m_current;
};

#endif // THEMEMANAGER_H
