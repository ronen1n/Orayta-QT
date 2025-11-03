#ifndef EDGETOEDGESUPPORT_H
#define EDGETOEDGESUPPORT_H

#include <QObject>
#include <QWidget>
#include <QMargins>

/**
 * @brief The EdgeToEdgeSupport class provides utilities for Android edge-to-edge display
 * 
 * This class handles Android edge-to-edge display features including:
 * - System bar insets detection
 * - UI layout adjustments for system bars
 * - Status bar and navigation bar handling
 */
class EdgeToEdgeSupport : public QObject
{
    Q_OBJECT

public:
    explicit EdgeToEdgeSupport(QObject *parent = nullptr);

    /**
     * @brief Enable edge-to-edge display for the application
     * @return true if edge-to-edge was successfully enabled
     */
    bool enableEdgeToEdge();

    /**
     * @brief Get system bar insets (status bar, navigation bar)
     * @return QMargins with insets (left, top, right, bottom)
     */
    QMargins getSystemBarInsets();

    /**
     * @brief Apply system bar insets to a widget's margins
     * @param widget The widget to apply insets to
     * @param applyTop Whether to apply top inset (status bar)
     * @param applyBottom Whether to apply bottom inset (navigation bar)
     * @param applyLeft Whether to apply left inset
     * @param applyRight Whether to apply right inset
     */
    void applySystemBarInsets(QWidget *widget, bool applyTop = true, bool applyBottom = true, bool applyLeft = false, bool applyRight = false);

    /**
     * @brief Check if edge-to-edge display is supported and enabled
     * @return true if edge-to-edge is available
     */
    bool isEdgeToEdgeSupported();

    /**
     * @brief Set system bar appearance (light/dark)
     * @param lightStatusBar Whether to use light status bar
     * @param lightNavigationBar Whether to use light navigation bar
     */
    void setSystemBarAppearance(bool lightStatusBar = true, bool lightNavigationBar = true);

    /**
     * @brief Log edge-to-edge display information
     */
    void logEdgeToEdgeInfo();

signals:
    /**
     * @brief Emitted when system bar insets change
     * @param insets New system bar insets
     */
    void systemBarInsetsChanged(const QMargins &insets);

private:
    QMargins m_systemBarInsets;
    bool m_edgeToEdgeEnabled;

    /**
     * @brief Update system bar insets from Android system
     */
    void updateSystemBarInsets();
};

#endif // EDGETOEDGESUPPORT_H