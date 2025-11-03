#include "edgetoedgesupport.h"
#include <QDebug>
#include <QApplication>
#include <QScreen>

#ifdef ANDROID
#include <QJniObject>
#include <QJniEnvironment>
#include <QCoreApplication>
#endif

EdgeToEdgeSupport::EdgeToEdgeSupport(QObject *parent)
    : QObject(parent), m_edgeToEdgeEnabled(false)
{
    updateSystemBarInsets();
}

bool EdgeToEdgeSupport::enableEdgeToEdge()
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
            qWarning() << "Could not get Android activity for edge-to-edge setup";
            return false;
        }
        
        // Get the window
        QJniObject window = activity.callObjectMethod(
            "getWindow", 
            "()Landroid/view/Window;"
        );
        
        if (!window.isValid()) {
            qWarning() << "Could not get window for edge-to-edge setup";
            return false;
        }
        
        // Enable edge-to-edge by setting system UI visibility flags
        // This is equivalent to WindowCompat.setDecorFitsSystemWindows(window, false)
        QJniObject view = window.callObjectMethod(
            "getDecorView", 
            "()Landroid/view/View;"
        );
        
        if (view.isValid()) {
            // Set system UI visibility flags for edge-to-edge
            const int SYSTEM_UI_FLAG_LAYOUT_STABLE = 0x00000100;
            const int SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION = 0x00000200;
            const int SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN = 0x00000400;
            
            int flags = SYSTEM_UI_FLAG_LAYOUT_STABLE | 
                       SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION | 
                       SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN;
            
            view.callMethod<void>("setSystemUiVisibility", "(I)V", flags);
            
            qDebug() << "Edge-to-edge display enabled successfully";
            m_edgeToEdgeEnabled = true;
            
            // Update system bar insets after enabling edge-to-edge
            updateSystemBarInsets();
            
            return true;
        }
        
    } catch (...) {
        qWarning() << "Exception occurred while enabling edge-to-edge display";
    }
    
    return false;
#else
    qDebug() << "Edge-to-edge display is only available on Android";
    return false;
#endif
}

QMargins EdgeToEdgeSupport::getSystemBarInsets()
{
    return m_systemBarInsets;
}

void EdgeToEdgeSupport::applySystemBarInsets(QWidget *widget, bool applyTop, bool applyBottom, bool applyLeft, bool applyRight)
{
    if (!widget) {
        qWarning() << "Cannot apply system bar insets to null widget";
        return;
    }
    
    QMargins currentMargins = widget->contentsMargins();
    QMargins newMargins = currentMargins;
    
    if (applyTop) {
        newMargins.setTop(currentMargins.top() + m_systemBarInsets.top());
    }
    if (applyBottom) {
        newMargins.setBottom(currentMargins.bottom() + m_systemBarInsets.bottom());
    }
    if (applyLeft) {
        newMargins.setLeft(currentMargins.left() + m_systemBarInsets.left());
    }
    if (applyRight) {
        newMargins.setRight(currentMargins.right() + m_systemBarInsets.right());
    }
    
    widget->setContentsMargins(newMargins);
    
    qDebug() << "Applied system bar insets to widget:" 
             << "top:" << (applyTop ? m_systemBarInsets.top() : 0)
             << "bottom:" << (applyBottom ? m_systemBarInsets.bottom() : 0)
             << "left:" << (applyLeft ? m_systemBarInsets.left() : 0)
             << "right:" << (applyRight ? m_systemBarInsets.right() : 0);
}

bool EdgeToEdgeSupport::isEdgeToEdgeSupported()
{
#ifdef ANDROID
    // Check Android API level
    jint apiLevel = QJniObject::getStaticField<jint>(
        "android/os/Build$VERSION", 
        "SDK_INT"
    );
    
    if (apiLevel > 0) {
        // Edge-to-edge is supported from Android 10 (API 29) but works best on Android 15+ (API 35)
        return apiLevel >= 29;
    }
    
    return false;
#else
    return false;
#endif
}

void EdgeToEdgeSupport::setSystemBarAppearance(bool lightStatusBar, bool lightNavigationBar)
{
#ifdef ANDROID
    try {
        QJniObject activity = QJniObject::callStaticObjectMethod(
            "org/qtproject/qt/android/QtNative", 
            "activity", 
            "()Landroid/app/Activity;"
        );
        
        if (!activity.isValid()) {
            qWarning() << "Could not get Android activity for system bar appearance";
            return;
        }
        
        QJniObject window = activity.callObjectMethod(
            "getWindow", 
            "()Landroid/view/Window;"
        );
        
        if (!window.isValid()) {
            qWarning() << "Could not get window for system bar appearance";
            return;
        }
        
        QJniObject view = window.callObjectMethod(
            "getDecorView", 
            "()Landroid/view/View;"
        );
        
        if (view.isValid()) {
            // Get current system UI visibility
            int currentFlags = view.callMethod<jint>("getSystemUiVisibility", "()I");
            
            const int SYSTEM_UI_FLAG_LIGHT_STATUS_BAR = 0x00002000;
            const int SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR = 0x00000010;
            
            int newFlags = currentFlags;
            
            if (lightStatusBar) {
                newFlags |= SYSTEM_UI_FLAG_LIGHT_STATUS_BAR;
            } else {
                newFlags &= ~SYSTEM_UI_FLAG_LIGHT_STATUS_BAR;
            }
            
            if (lightNavigationBar) {
                newFlags |= SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR;
            } else {
                newFlags &= ~SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR;
            }
            
            view.callMethod<void>("setSystemUiVisibility", "(I)V", newFlags);
            
            qDebug() << "System bar appearance updated - light status bar:" << lightStatusBar 
                     << "light navigation bar:" << lightNavigationBar;
        }
        
    } catch (...) {
        qWarning() << "Exception occurred while setting system bar appearance";
    }
#else
    qDebug() << "System bar appearance is only available on Android";
#endif
}

void EdgeToEdgeSupport::logEdgeToEdgeInfo()
{
    qDebug() << "=== Edge-to-Edge Display Information ===";
    qDebug() << "Edge-to-edge supported:" << isEdgeToEdgeSupported();
    qDebug() << "Edge-to-edge enabled:" << m_edgeToEdgeEnabled;
    qDebug() << "System bar insets - top:" << m_systemBarInsets.top() 
             << "bottom:" << m_systemBarInsets.bottom()
             << "left:" << m_systemBarInsets.left()
             << "right:" << m_systemBarInsets.right();
    
    if (QApplication::primaryScreen()) {
        QRect screenGeometry = QApplication::primaryScreen()->geometry();
        qDebug() << "Screen geometry:" << screenGeometry;
        QRect availableGeometry = QApplication::primaryScreen()->availableGeometry();
        qDebug() << "Available geometry:" << availableGeometry;
    }
    
    qDebug() << "=== End Edge-to-Edge Display Information ===";
}

void EdgeToEdgeSupport::updateSystemBarInsets()
{
#ifdef ANDROID
    try {
        // Try to get system bar insets from Android
        QJniObject activity = QJniObject::callStaticObjectMethod(
            "org/qtproject/qt/android/QtNative", 
            "activity", 
            "()Landroid/app/Activity;"
        );
        
        if (!activity.isValid()) {
            // Fallback to estimated values
            m_systemBarInsets = QMargins(0, 24, 0, 48); // Typical status bar (24dp) and nav bar (48dp)
            return;
        }
        
        QJniObject window = activity.callObjectMethod(
            "getWindow", 
            "()Landroid/view/Window;"
        );
        
        if (window.isValid()) {
            QJniObject decorView = window.callObjectMethod(
                "getDecorView", 
                "()Landroid/view/View;"
            );
            
            if (decorView.isValid()) {
                // Try to get window insets
                QJniObject rootWindowInsets = decorView.callObjectMethod(
                    "getRootWindowInsets", 
                    "()Landroid/view/WindowInsets;"
                );
                
                if (rootWindowInsets.isValid()) {
                    // Get system window insets
                    int left = rootWindowInsets.callMethod<jint>("getSystemWindowInsetLeft", "()I");
                    int top = rootWindowInsets.callMethod<jint>("getSystemWindowInsetTop", "()I");
                    int right = rootWindowInsets.callMethod<jint>("getSystemWindowInsetRight", "()I");
                    int bottom = rootWindowInsets.callMethod<jint>("getSystemWindowInsetBottom", "()I");
                    
                    // Convert from pixels to device-independent pixels
                    qreal devicePixelRatio = QApplication::primaryScreen() ? QApplication::primaryScreen()->devicePixelRatio() : 1.0;
                    
                    m_systemBarInsets = QMargins(
                        static_cast<int>(left / devicePixelRatio),
                        static_cast<int>(top / devicePixelRatio),
                        static_cast<int>(right / devicePixelRatio),
                        static_cast<int>(bottom / devicePixelRatio)
                    );
                    
                    emit systemBarInsetsChanged(m_systemBarInsets);
                    return;
                }
            }
        }
        
        // Fallback to estimated values if we can't get actual insets
        m_systemBarInsets = QMargins(0, 24, 0, 48);
        
    } catch (...) {
        qWarning() << "Exception occurred while updating system bar insets";
        m_systemBarInsets = QMargins(0, 24, 0, 48);
    }
#else
    // No system bars on desktop
    m_systemBarInsets = QMargins(0, 0, 0, 0);
#endif
}