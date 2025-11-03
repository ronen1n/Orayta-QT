#include "themesupport.h"
#include <QSettings>
#include <QDebug>

#ifdef ANDROID
#include <QJniObject>
#endif

ThemeSupport::ThemeSupport(QObject *parent)
    : QObject(parent), m_themeMode(Auto), m_systemDarkMode(false)
{
    // Detect system theme on initialization
    m_systemDarkMode = detectSystemDarkMode();
    qDebug() << "System dark mode detected:" << m_systemDarkMode;
}

void ThemeSupport::setThemeMode(ThemeMode mode)
{
    if (m_themeMode != mode) {
        m_themeMode = mode;
        updateTheme();
        saveSettings();
    }
}

bool ThemeSupport::isDarkThemeActive() const
{
    switch (m_themeMode) {
        case Auto:
            return m_systemDarkMode;
        case Light:
            return false;
        case Dark:
            return true;
        default:
            return false;
    }
}

bool ThemeSupport::detectSystemDarkMode()
{
#ifdef ANDROID
    try {
        // Get the current activity
        QJniObject activity = QJniObject::callStaticObjectMethod(
            "org/qtproject/qt/android/QtNative",
            "activity",
            "()Landroid/app/Activity;"
        );

        if (!activity.isValid()) {
            qWarning() << "Could not get Android activity for theme detection";
            return false;
        }

        // Get resources
        QJniObject resources = activity.callObjectMethod(
            "getResources",
            "()Landroid/content/res/Resources;"
        );

        if (!resources.isValid()) {
            qWarning() << "Could not get resources for theme detection";
            return false;
        }

        // Get configuration
        QJniObject configuration = resources.callObjectMethod(
            "getConfiguration",
            "()Landroid/content/res/Configuration;"
        );

        if (!configuration.isValid()) {
            qWarning() << "Could not get configuration for theme detection";
            return false;
        }

        // Get UI mode
        jint uiMode = configuration.getField<jint>("uiMode");

        // UI_MODE_NIGHT_MASK = 0x30
        // UI_MODE_NIGHT_YES = 0x20
        const jint UI_MODE_NIGHT_MASK = 0x30;
        const jint UI_MODE_NIGHT_YES = 0x20;

        bool isDark = (uiMode & UI_MODE_NIGHT_MASK) == UI_MODE_NIGHT_YES;

        qDebug() << "Android system theme detected - Dark mode:" << isDark;
        m_systemDarkMode = isDark;
        return isDark;

    } catch (...) {
        qWarning() << "Exception occurred while detecting system theme";
        return false;
    }
#else
    // On non-Android platforms, default to light mode
    qDebug() << "System theme detection is only available on Android";
    return false;
#endif
}

void ThemeSupport::loadSettings()
{
    QSettings settings("Orayta", "SingleUser");
    settings.beginGroup("Confs");

    // Load theme mode (0=Auto, 1=Light, 2=Dark)
    // For backward compatibility, check old "nightMode" setting
    if (settings.contains("themeMode")) {
        int mode = settings.value("themeMode", 0).toInt();
        m_themeMode = static_cast<ThemeMode>(mode);
    } else if (settings.contains("nightMode")) {
        // Migrate old nightMode setting
        bool oldNightMode = settings.value("nightMode", false).toBool();
        m_themeMode = oldNightMode ? Dark : Light;
        // Save in new format
        settings.setValue("themeMode", static_cast<int>(m_themeMode));
        settings.remove("nightMode");
    } else {
        m_themeMode = Auto;
    }

    settings.endGroup();

    qDebug() << "Theme mode loaded:" << m_themeMode;

    // Update system theme detection
    m_systemDarkMode = detectSystemDarkMode();
    updateTheme();
}

void ThemeSupport::saveSettings()
{
    QSettings settings("Orayta", "SingleUser");
    settings.beginGroup("Confs");
    settings.setValue("themeMode", static_cast<int>(m_themeMode));
    settings.endGroup();

    qDebug() << "Theme mode saved:" << m_themeMode;
}

void ThemeSupport::updateTheme()
{
    bool isDark = isDarkThemeActive();
    qDebug() << "Theme updated - Dark mode active:" << isDark;
    emit themeChanged(isDark);
}
