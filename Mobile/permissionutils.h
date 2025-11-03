/* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2
* as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*
* Author: Qt6 Android 16 Migration
*/

#ifndef PERMISSIONUTILS_H
#define PERMISSIONUTILS_H

#include <QString>
#include <QObject>
#include <functional>

#ifdef Q_OS_ANDROID
#include <QJniObject>
#include <QJniEnvironment>
#include <QCoreApplication>
#include <QCoreApplication>
#endif

/**
 * @brief Utility class for handling Android permissions in Qt6
 * 
 * This class provides helper functions to check and request storage permissions
 * using Qt6's QPermission API where possible, falling back to QtAndroidPrivate
 * for Android-specific permissions.
 */
class PermissionUtils : public QObject
{
    Q_OBJECT

public:
    enum PermissionStatus {
        Granted,
        Denied,
        Undetermined
    };

    enum StoragePermissionType {
        ReadExternalStorage,
        WriteExternalStorage,
        ReadMediaImages,
        ReadMediaVideo,
        ReadMediaAudio
    };

    explicit PermissionUtils(QObject *parent = nullptr);

    /**
     * @brief Check if storage permissions are granted
     * @return true if all required storage permissions are granted
     */
    static bool hasStoragePermissions();

    /**
     * @brief Check specific storage permission
     * @param permissionType The type of storage permission to check
     * @return PermissionStatus indicating the current status
     */
    static PermissionStatus checkStoragePermission(StoragePermissionType permissionType);

    /**
     * @brief Request storage permissions from the user
     * @param callback Function to call when permission request is complete
     */
    static void requestStoragePermissions(std::function<void(bool)> callback = nullptr);

    /**
     * @brief Request specific storage permission
     * @param permissionType The type of storage permission to request
     * @param callback Function to call when permission request is complete
     */
    static void requestStoragePermission(StoragePermissionType permissionType, 
                                       std::function<void(bool)> callback = nullptr);

    /**
     * @brief Check if the app can read from external storage
     * @return true if read access is available
     */
    static bool canReadExternalStorage();

    /**
     * @brief Check if the app can write to external storage
     * @return true if write access is available
     */
    static bool canWriteExternalStorage();

    /**
     * @brief Get user-friendly permission name for display
     * @param permissionType The permission type
     * @return Human-readable permission name
     */
    static QString getPermissionDisplayName(StoragePermissionType permissionType);

    /**
     * @brief Show permission rationale dialog to user
     * @param permissionType The permission that needs rationale
     * @param callback Function to call when user responds
     */
    static void showPermissionRationale(StoragePermissionType permissionType,
                                      std::function<void(bool)> callback = nullptr);

private:
#ifdef Q_OS_ANDROID
    /**
     * @brief Get Android permission string for the given permission type
     * @param permissionType The permission type
     * @return Android permission string
     */
    static QString getAndroidPermissionString(StoragePermissionType permissionType);

    /**
     * @brief Check permission using QtAndroidPrivate (fallback method)
     * @param permission Android permission string
     * @return PermissionStatus
     */
    static PermissionStatus checkPermissionAndroid(const QString& permission);

    /**
     * @brief Request permission using QtAndroidPrivate (fallback method)
     * @param permission Android permission string
     * @param callback Function to call when request is complete
     */
    static void requestPermissionAndroid(const QString& permission,
                                       std::function<void(bool)> callback = nullptr);
#endif

signals:
    void permissionRequestCompleted(bool granted);
};

#endif // PERMISSIONUTILS_H