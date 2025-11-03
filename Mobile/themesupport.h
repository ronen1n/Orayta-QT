#ifndef THEMESUPPORT_H
#define THEMESUPPORT_H

#include <QObject>

/**
 * @brief The ThemeSupport class provides theme management for the application
 * 
 * This class handles:
 * - Auto-detection of system theme (dark/light)
 * - Manual theme selection
 * - Theme persistence
 */
class ThemeSupport : public QObject
{
    Q_OBJECT

public:
    enum ThemeMode {
        Auto = 0,
        Light = 1,
        Dark = 2
    };

    explicit ThemeSupport(QObject *parent = nullptr);

    /**
     * @brief Get the current theme mode setting
     * @return ThemeMode (Auto, Light, or Dark)
     */
    ThemeMode getThemeMode() const { return m_themeMode; }

    /**
     * @brief Set the theme mode
     * @param mode ThemeMode to set
     */
    void setThemeMode(ThemeMode mode);

    /**
     * @brief Check if dark theme is currently active
     * @return true if dark theme should be used
     */
    bool isDarkThemeActive() const;

    /**
     * @brief Detect system theme from Android
     * @return true if system is in dark mode
     */
    bool detectSystemDarkMode();

    /**
     * @brief Load theme settings from QSettings
     */
    void loadSettings();

    /**
     * @brief Save theme settings to QSettings
     */
    void saveSettings();

signals:
    /**
     * @brief Emitted when the active theme changes
     * @param isDark true if dark theme is now active
     */
    void themeChanged(bool isDark);

private:
    ThemeMode m_themeMode;
    bool m_systemDarkMode;

    void updateTheme();
};

#endif // THEMESUPPORT_H
