#include "thememanager.h"

#include "nlohmann/json.hpp"

#include <QDebug>
#include <QDir>
#include <QFile>

#include <exception>

ThemeManager& ThemeManager::instance()
{
    static ThemeManager manager;
    return manager;
}

QByteArray ThemeManager::buildStyleSheet(const QString &theme_name, const QString &color_mode) const
{
    const QString mode_name = normalizedColorMode(color_mode);
    QByteArray style = readResource(QString(":/style/style_%1.qss").arg(mode_name));
    if (style.isEmpty()) {
        return style;
    }

    Theme theme;
    if (!loadTheme(theme_name, mode_name, &theme)) {
        qWarning() << "Using raw QSS; failed to load theme tokens" << theme_name << mode_name;
        return style;
    }

    return applyTokens(style, theme);
}

bool ThemeManager::loadTheme(const QString &theme_name, const QString &color_mode, Theme *theme) const
{
    if (!theme) return false;

    QString safe_name = theme_name.trimmed();
    if (safe_name.isEmpty() || safe_name == "default") safe_name = "gruvbox";
    const QString mode_name = normalizedColorMode(color_mode);
    QByteArray bytes = readResource(QString(":/themes/%1/%2.json").arg(safe_name, mode_name));
    if (bytes.isEmpty() && safe_name != "gruvbox") {
        bytes = readResource(QString(":/themes/gruvbox/%1.json").arg(mode_name));
    }
    if (bytes.isEmpty()) return false;

    try {
        const nlohmann::json root = nlohmann::json::parse(bytes.constData(), bytes.constData() + bytes.size());
        const nlohmann::json *tokens = &root;
        if (root.contains("tokens") && root["tokens"].is_object()) {
            tokens = &root["tokens"];
        }

        Theme loaded;
        loaded.name = root.contains("name") && root["name"].is_string()
                    ? QString::fromStdString(root["name"].get<std::string>())
                    : safe_name;
        if (root.contains("color_mode") && root["color_mode"].is_string()) {
            loaded.color_mode = normalizedColorMode(QString::fromStdString(root["color_mode"].get<std::string>()));
        } else if (root.contains("mode") && root["mode"].is_string()) {
            loaded.color_mode = normalizedColorMode(QString::fromStdString(root["mode"].get<std::string>()));
        } else {
            loaded.color_mode = mode_name;
        }

        for (auto it = tokens->begin(); it != tokens->end(); ++it) {
            if (!it.value().is_string()) continue;
            loaded.tokens.insert(normalizedTokenName(QString::fromStdString(it.key())),
                                 QString::fromStdString(it.value().get<std::string>()));
        }

        *theme = loaded;
        return !theme->tokens.isEmpty();
    } catch (const std::exception &e) {
        qWarning() << "Failed to parse theme" << safe_name << mode_name << e.what();
        return false;
    }
}

QStringList ThemeManager::availableThemes() const
{
    QStringList themes = QDir(":/themes").entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    if (!themes.contains("gruvbox")) {
        themes.prepend("gruvbox");
    }
    return themes;
}

bool ThemeManager::setCurrentTheme(const QString &theme_name, const QString &color_mode)
{
    Theme loaded;
    if (!loadTheme(theme_name, color_mode, &loaded)) return false;
    m_current = loaded;
    return true;
}

QString ThemeManager::token(const QString &name, const QString &fallback) const
{
    return m_current.tokens.value(normalizedTokenName(name), fallback);
}

QByteArray ThemeManager::readResource(const QString &path) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open theme resource" << path;
        return QByteArray();
    }
    return file.readAll();
}

QByteArray ThemeManager::applyTokens(QByteArray style, const Theme &theme) const
{
    for (auto it = theme.tokens.constBegin(); it != theme.tokens.constEnd(); ++it) {
        const QByteArray key = it.key().toUtf8();
        const QByteArray value = it.value().toUtf8();
        style.replace(QByteArray("${") + key + "}", value);
        style.replace(QByteArray("${--") + key + "}", value);
    }

    if (style.contains("${")) {
        qWarning() << "Theme stylesheet still contains unresolved token placeholders";
    }
    return style;
}

QString ThemeManager::normalizedColorMode(const QString &color_mode) const
{
    return color_mode.compare("light", Qt::CaseInsensitive) == 0 ? QString("light") : QString("dark");
}

QString ThemeManager::normalizedTokenName(const QString &name) const
{
    QString normalized = name.trimmed();
    while (normalized.startsWith("--")) {
        normalized.remove(0, 2);
    }
    return normalized;
}
