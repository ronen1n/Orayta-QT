#ifndef ANDROID16SUPPORT_H
#define ANDROID16SUPPORT_H

#include <QObject>

/**
 * @brief The Android16Support class provides utilities for Android 16 compatibility
 * 
 * This class handles Android 16 specific features including:
 * - 16KB page size support verification
 * - Android 16 feature detection
 * - System information logging
 */
class Android16Support : public QObject
{
    Q_OBJECT

public:
    explicit Android16Support(QObject *parent = nullptr);

    /**
     * @brief Verify that the application supports 16KB page size
     * @return true if 16KB page size is supported or not required
     */
    bool verify16KBPageSizeSupport();

    /**
     * @brief Check if Android 16 features are available and working
     * @return true if Android 16 features are supported
     */
    bool checkAndroid16Features();

    /**
     * @brief Log system information relevant to Android 16 support
     */
    void logSystemInfo();

signals:

};

#endif // ANDROID16SUPPORT_H