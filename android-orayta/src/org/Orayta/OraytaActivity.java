package org.Orayta;

import org.qtproject.qt.android.bindings.QtActivity;
import android.os.Bundle;
import android.os.Build;
import android.window.OnBackInvokedDispatcher;
import android.window.OnBackInvokedCallback;
import android.util.Log;
import java.lang.reflect.Method;

public class OraytaActivity extends QtActivity
{
    private static final String TAG = "OraytaActivity";
    
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        // Qt 6.10.0 bug workaround: Force library loading before super.onCreate()
        // This prevents UnsatisfiedLinkError when Qt tries to call native methods
        try {
            // Use reflection to call QtNative.loadQtLibraries() before super.onCreate()
            Class<?> qtNativeClass = Class.forName("org.qtproject.qt.android.QtNative");
            Method loadLibrariesMethod = qtNativeClass.getDeclaredMethod("loadQtLibraries");
            loadLibrariesMethod.setAccessible(true);
            loadLibrariesMethod.invoke(null);
            Log.d(TAG, "Successfully pre-loaded Qt libraries");
        } catch (Exception e) {
            Log.w(TAG, "Could not pre-load Qt libraries: " + e.getMessage());
            // Continue anyway, super.onCreate() will try to load them
        }
        
        try {
            super.onCreate(savedInstanceState);
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Qt initialization failed: " + e.getMessage(), e);
            
            // Show error to user and exit
            android.app.AlertDialog.Builder builder = new android.app.AlertDialog.Builder(this);
            builder.setTitle("Initialization Error");
            builder.setMessage("Failed to initialize Qt libraries.\n\nThis is a known Qt 6.10.0 bug. Please try:\n1. Reinstalling the app\n2. Restarting your device\n\nError: " + e.getMessage());
            builder.setPositiveButton("OK", (dialog, which) -> finish());
            builder.setCancelable(false);
            builder.show();
            return;
        }
        
        // Register back callback for Android 13+ (API 33+)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            Log.d(TAG, "Registering OnBackInvokedCallback for Android 13+");
            getOnBackInvokedDispatcher().registerOnBackInvokedCallback(
                OnBackInvokedDispatcher.PRIORITY_DEFAULT,
                new OnBackInvokedCallback() {
                    @Override
                    public void onBackInvoked() {
                        Log.d(TAG, "onBackInvoked called");
                        if (!handleBackButton()) {
                            Log.d(TAG, "C++ returned false, finishing activity");
                            finish();
                        } else {
                            Log.d(TAG, "C++ handled back button");
                        }
                    }
                }
            );
        }
    }

    @Override
    public void onBackPressed()
    {
        // For Android 12 and below
        Log.d(TAG, "onBackPressed called");
        if (!handleBackButton()) {
            Log.d(TAG, "C++ returned false, calling super.onBackPressed()");
            super.onBackPressed();
        } else {
            Log.d(TAG, "C++ handled back button");
        }
    }

    // Native method declaration - will be implemented in C++
    public native boolean handleBackButton();
}
