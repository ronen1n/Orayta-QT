#include "android16support.h"

#ifdef ANDROID
#include <QJniObject>
#include <QJniEnvironment>
#include <QDebug>
#include <unistd.h>
#include <sys/mman.h>
#endif

Android16Support::Android16Support(QObject *parent)
    : QObject(parent)
{
}

bool Android16Support::verify16KBPageSizeSupport()
{
#ifdef ANDROID
    // Check if the device supports 16KB page size
    long pageSize = sysconf(_SC_PAGESIZE);
    qDebug() << "Current page size:" << pageSize << "bytes";
    
    // 16KB = 16384 bytes
    if (pageSize == 16384) {
        qDebug() << "Device is using 16KB page size";
        return true;
    } else if (pageSize == 4096) {
        qDebug() << "Device is using 4KB page size (standard)";
        return true; // App should work on both
    } else {
        qDebug() << "Device is using non-standard page size:" << pageSize;
        return true; // Assume compatibility
    }
#else
    qDebug() << "16KB page size check is only available on Android";
    return true;
#endif
}

bool Android16Support::checkAndroid16Features()
{
#ifdef ANDROID
    // Check Android API level
    QJniObject activity = QJniObject::callStaticObjectMethod(
        "org/qtproject/qt/android/QtNative", 
        "activity", 
        "()Landroid/app/Activity;"
    );
    
    if (!activity.isValid()) {
        qWarning() << "Could not get Android activity";
        return false;
    }
    
    // Get Android version
    QJniObject versionRelease = QJniObject::getStaticObjectField(
        "android/os/Build$VERSION", 
        "RELEASE", 
        "Ljava/lang/String;"
    );
    
    jint apiLevel = QJniObject::getStaticField<jint>(
        "android/os/Build$VERSION", 
        "SDK_INT"
    );
    
    if (versionRelease.isValid()) {
        QString androidVersion = versionRelease.toString();
        qDebug() << "Android version:" << androidVersion;
    }
    
    if (apiLevel > 0) {
        qDebug() << "Android API level:" << apiLevel;
        
        if (apiLevel >= 36) {
            qDebug() << "Running on Android 16+ (API 36+)";
            return verify16KBPageSizeSupport();
        } else {
            qDebug() << "Running on Android" << apiLevel << "(pre-Android 16)";
            return true; // Backward compatibility
        }
    }
    
    return false;
#else
    qDebug() << "Android 16 feature check is only available on Android";
    return true;
#endif
}

void Android16Support::logSystemInfo()
{
#ifdef ANDROID
    qDebug() << "=== Android 16 Support Information ===";
    
    // Log page size
    long pageSize = sysconf(_SC_PAGESIZE);
    qDebug() << "System page size:" << pageSize << "bytes";
    
    // Log memory info
    long totalPages = sysconf(_SC_PHYS_PAGES);
    long availablePages = sysconf(_SC_AVPHYS_PAGES);
    
    if (totalPages > 0 && pageSize > 0) {
        qDebug() << "Total memory:" << (totalPages * pageSize / 1024 / 1024) << "MB";
        qDebug() << "Available memory:" << (availablePages * pageSize / 1024 / 1024) << "MB";
    }
    
    // Check if we're running with 16KB page size support
    #ifdef ANDROID_16KB_PAGE_SIZE_SUPPORT
    qDebug() << "16KB page size support: ENABLED";
    #else
    qDebug() << "16KB page size support: DISABLED";
    #endif
    
    // Check edge-to-edge display support
    jint apiLevel = QJniObject::getStaticField<jint>(
        "android/os/Build$VERSION", 
        "SDK_INT"
    );
    
    if (apiLevel > 0) {
        bool edgeToEdgeSupported = apiLevel >= 29;
        qDebug() << "Edge-to-edge display support:" << (edgeToEdgeSupported ? "SUPPORTED" : "NOT SUPPORTED");
    }
    
    qDebug() << "=== End Android 16 Support Information ===";
#else
    qDebug() << "System info logging is only available on Android";
#endif
}