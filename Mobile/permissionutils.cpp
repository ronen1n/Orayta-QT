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

#include "permissionutils.h"
#include <QDebug>
#include <QMessageBox>
#include <QApplication>
#include <QOperatingSystemVersion>

#ifdef Q_OS_ANDROID
#include <QCoreApplication>
// QNativeInterface is included via QCoreApplication in Qt6
#endif

PermissionUtils::PermissionUtils(QObject *parent)
    : QObject(parent)
{
}

bool PermissionUtils::hasStoragePermissions()
{
#ifdef Q_OS_ANDROID
    // For Android 13+ (API 33+), check granular media permissions
    QOperatingSystemVersion currentVersion = QOperatingSystemVersion::current();
    if (currentVersion >= QOperatingSystemVersion(QOperatingSystemVersion::Android, 13)) {
        // Check granular media permissions for Android 13+
        return (checkStoragePermission(ReadMediaImages) == Granted ||
                checkStoragePermission(ReadMediaVideo) == Granted ||
                checkStoragePermission(ReadMediaAudio) == Granted);
    } else {
        // For Android 12 and below, check legacy storage permissions
        return (checkStoragePermission(ReadExternalStorage) == Granted);
    }
#else
    // On non-Android platforms, assume permissions are granted
    return true;
#endif
}

PermissionUtils::PermissionStatus PermissionUtils::checkStoragePermission(StoragePermissionType permissionType)
{
#ifdef Q_OS_ANDROID
    QString permission = getAndroidPermissionString(permissionType);
    return checkPermissionAndroid(permission);
#else
    Q_UNUSED(permissionType)
    return Granted;
#endif
}

void PermissionUtils::requestStoragePermissions(std::function<void(bool)> callback)
{
#ifdef Q_OS_ANDROID
    QOperatingSystemVersion currentVersion = QOperatingSystemVersion::current();
    
    if (currentVersion >= QOperatingSystemVersion(QOperatingSystemVersion::Android, 13)) {
        // For Android 13+, request granular media permissions
        // We'll request READ_MEDIA_IMAGES as it's most commonly needed for books
        requestStoragePermission(ReadMediaImages, [callback](bool granted) {
            if (callback) {
                callback(granted);
            }
        });
    } else {
        // For Android 12 and below, request legacy storage permission
        requestStoragePermission(ReadExternalStorage, [callback](bool granted) {
            if (callback) {
                callback(granted);
            }
        });
    }
#else
    if (callback) {
        callback(true); // Non-Android platforms don't need permission requests
    }
#endif
}

void PermissionUtils::requestStoragePermission(StoragePermissionType permissionType, 
                                             std::function<void(bool)> callback)
{
#ifdef Q_OS_ANDROID
    QString permission = getAndroidPermissionString(permissionType);
    requestPermissionAndroid(permission, callback);
#else
    Q_UNUSED(permissionType)
    if (callback) {
        callback(true);
    }
#endif
}

bool PermissionUtils::canReadExternalStorage()
{
#ifdef Q_OS_ANDROID
    QOperatingSystemVersion currentVersion = QOperatingSystemVersion::current();
    
    if (currentVersion >= QOperatingSystemVersion(QOperatingSystemVersion::Android, 13)) {
        // For Android 13+, check if any media permission is granted
        return (checkStoragePermission(ReadMediaImages) == Granted ||
                checkStoragePermission(ReadMediaVideo) == Granted ||
                checkStoragePermission(ReadMediaAudio) == Granted);
    } else {
        // For Android 12 and below
        return checkStoragePermission(ReadExternalStorage) == Granted;
    }
#else
    return true;
#endif
}

bool PermissionUtils::canWriteExternalStorage()
{
#ifdef Q_OS_ANDROID
    QOperatingSystemVersion currentVersion = QOperatingSystemVersion::current();
    
    if (currentVersion >= QOperatingSystemVersion(QOperatingSystemVersion::Android, 13)) {
        // Android 13+ doesn't require WRITE_EXTERNAL_STORAGE for app-specific directories
        // Apps can write to their own directories without permission
        return true;
    } else {
        // For Android 12 and below
        return checkStoragePermission(WriteExternalStorage) == Granted;
    }
#else
    return true;
#endif
}

QString PermissionUtils::getPermissionDisplayName(StoragePermissionType permissionType)
{
    switch (permissionType) {
        case ReadExternalStorage:
            return QObject::tr("Storage Access");
        case WriteExternalStorage:
            return QObject::tr("Storage Write Access");
        case ReadMediaImages:
            return QObject::tr("Media Images Access");
        case ReadMediaVideo:
            return QObject::tr("Media Video Access");
        case ReadMediaAudio:
            return QObject::tr("Media Audio Access");
        default:
            return QObject::tr("Storage Permission");
    }
}

void PermissionUtils::showPermissionRationale(StoragePermissionType permissionType,
                                            std::function<void(bool)> callback)
{
    QString permissionName = getPermissionDisplayName(permissionType);
    QString message;
    
    switch (permissionType) {
        case ReadExternalStorage:
        case ReadMediaImages:
        case ReadMediaVideo:
        case ReadMediaAudio:
            message = QObject::tr("Orayta needs %1 permission to access and display books stored on your device. "
                                "Without this permission, you won't be able to import or read books from external storage.")
                     .arg(permissionName);
            break;
        case WriteExternalStorage:
            message = QObject::tr("Orayta needs %1 permission to save books and bookmarks to your device. "
                                "Without this permission, you won't be able to download or save books.")
                     .arg(permissionName);
            break;
        default:
            message = QObject::tr("Orayta needs %1 permission to function properly.")
                     .arg(permissionName);
            break;
    }

    QMessageBox msgBox;
    msgBox.setWindowTitle(QObject::tr("Permission Required"));
    msgBox.setText(message);
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Ok);
    
    int result = msgBox.exec();
    
    if (callback) {
        callback(result == QMessageBox::Ok);
    }
}

#ifdef Q_OS_ANDROID
QString PermissionUtils::getAndroidPermissionString(StoragePermissionType permissionType)
{
    switch (permissionType) {
        case ReadExternalStorage:
            return QStringLiteral("android.permission.READ_EXTERNAL_STORAGE");
        case WriteExternalStorage:
            return QStringLiteral("android.permission.WRITE_EXTERNAL_STORAGE");
        case ReadMediaImages:
            return QStringLiteral("android.permission.READ_MEDIA_IMAGES");
        case ReadMediaVideo:
            return QStringLiteral("android.permission.READ_MEDIA_VIDEO");
        case ReadMediaAudio:
            return QStringLiteral("android.permission.READ_MEDIA_AUDIO");
        default:
            return QStringLiteral("android.permission.READ_EXTERNAL_STORAGE");
    }
}

PermissionUtils::PermissionStatus PermissionUtils::checkPermissionAndroid(const QString& permission)
{
    try {
        // Use QJniObject to check permission via Android API
        QJniObject context = QNativeInterface::QAndroidApplication::context();
        if (!context.isValid()) {
            qWarning() << "Failed to get Android context";
            return Denied;
        }
        
        // Call Android's checkSelfPermission
        jint result = QJniObject::callStaticMethod<jint>(
            "androidx/core/content/ContextCompat",
            "checkSelfPermission",
            "(Landroid/content/Context;Ljava/lang/String;)I",
            context.object<jobject>(),
            QJniObject::fromString(permission).object<jstring>()
        );
        
        // PackageManager.PERMISSION_GRANTED = 0
        if (result == 0) {
            return Granted;
        } else {
            return Denied;
        }
    } catch (...) {
        qWarning() << "Exception occurred while checking permission:" << permission;
        return Denied;
    }
}

void PermissionUtils::requestPermissionAndroid(const QString& permission,
                                             std::function<void(bool)> callback)
{
    try {
        // First check if permission is already granted
        if (checkPermissionAndroid(permission) == Granted) {
            if (callback) {
                callback(true);
            }
            return;
        }

        // For Qt6, we'll use a simplified approach
        // In a full implementation, you would need to create a custom Java activity
        // or use Qt6's new permission system when it becomes more mature
        
        qWarning() << "Permission request not fully implemented in Qt6 migration";
        qWarning() << "Requested permission:" << permission;
        qWarning() << "Please grant permissions manually in Android settings";
        
        // For now, assume permission is denied and let user know
        if (callback) {
            callback(false);
        }
        
    } catch (...) {
        qWarning() << "Exception occurred while requesting permission:" << permission;
        if (callback) {
            callback(false);
        }
    }
}
#endif