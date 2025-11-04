# Orayta Android Fork - Complete Changes Summary

## Overview
This fork transforms the original cross-platform Orayta-QT into an Android-only application with Qt6.

**Key Changes:**
- Removed: Desktop UI (22 files), Linux packaging, Windows support
- Added: 9 new Android support files
- Modified: 23 files for Qt6 and Android improvements
- Build System: qmake → CMake

---

## Files Added (9 files)

### Android Support Classes
1. **Mobile/android16support.h/cpp** - Checks 16KB page size compatibility for Android 16+
2. **Mobile/edgetoedgesupport.h/cpp** - Material Design edge-to-edge display with system bar insets
3. **Mobile/permissionutils.h/cpp** - Runtime storage permissions (Android 6+, handles Android 13+ granular permissions)
4. **Mobile/themesupport.h/cpp** - Auto-detects system dark/light theme

### Qt6 Compatibility
5. **OraytaBase/qt6_textcodec_compat.h** - QTextCodec replacement (removed in Qt6)

---

## Files Removed (22 files)

**Entire Desktop Module:**
- Desktop/about.cpp/h/ui
- Desktop/addcomment.cpp/h/ui
- Desktop/bookdisplayer.cpp/h
- Desktop/desktopapp.cpp/h/ui
- Desktop/desktopapp_bookmark.cpp
- Desktop/errorreport.cpp/h/ui
- Desktop/importbook.cpp/h/ui
- Desktop/mytreetab.cpp/h
- Desktop/mywebview.cpp/h
- Desktop/pdfwidget.cpp/h
- Desktop/searchform.cpp/ui
- Desktop/settings.cpp/h/ui

**Reason:** Desktop UI uses QtWebKit (removed in Qt6) and requires extensive Qt6 migration

---

## Files Modified (23 files)

### Major Changes (100+ lines)

#### 1. Mobile/mobileapp.cpp (+1949, -131)
**New Features:**
- **Delete books functionality** with confirmation dialogs
- **Tree state management** (save/restore expanded categories)
- **Styled message boxes** matching app theme
- **Touch movement detection** to prevent accidental clicks while scrolling
- **Android support initialization** (16KB pages, edge-to-edge, theme detection)
- **Permission checks** before file operations

**Touch/Scroll Improvements:**
```cpp
// NEW: mousePressEvent - Track touch start position
void MobileApp::mousePressEvent(QMouseEvent *ev) {
    lastPressPos = ev->globalPosition().toPoint();
    touchMoved = false;
    touchMoveTimer.start();
}

// NEW: mouseMoveEvent - Detect if user is scrolling
void MobileApp::mouseMoveEvent(QMouseEvent *ev) {
    QPoint currentPos = ev->globalPosition().toPoint();
    int dx = qAbs(currentPos.x() - lastPressPos.x());
    int dy = qAbs(currentPos.y() - lastPressPos.y());
    
    if (dx > 8 || dy > 8) {
        touchMoved = true;  // User is scrolling, not clicking
    }
}
```
**Purpose:** Prevents accidental book/chapter clicks when user is scrolling

**Android Support Initialization:**
```cpp
#ifdef ANDROID
    // Initialize 16KB page size support
    android16Support = new Android16Support(this);
    android16Support->logSystemInfo();
    android16Support->verify16KBPageSizeSupport();
    
    // Initialize edge-to-edge display
    edgeToEdgeSupport = new EdgeToEdgeSupport(this);
    edgeToEdgeSupport->enableEdgeToEdge();
    edgeToEdgeSupport->setSystemBarAppearance(true, true);
    edgeToEdgeSupport->applySystemBarInsets(this, true, false);
    
    // Initialize theme support with auto-detection
    themeSupport = new ThemeSupport(this);
    bool systemDark = themeSupport->detectSystemDarkMode();
    nightMode = systemDark;  // Auto-detect on first run
#endif
```

**New Functions:**
- `mousePressEvent()` / `mouseMoveEvent()` - Touch movement tracking
- `on_deleteBooksBTN_clicked()` - Toggle delete mode
- `removeBookOrCategory()` - Delete with confirmation
- `countBooksInCategory()` / `removeBooksInCategory()` - Category deletion
- `getExpandedCategories()` / `restoreExpandedCategories()` - Tree state
- `createStyledMessageBox()` - App-themed dialogs
- `performShowBook()` / `performAssetCopy()` - Improved operations

#### 2. Mobile/mobileapp_download.cpp (+216, -40)
**New Features:**
- **Stop download button** - Cancel ongoing downloads
- **Parallel download support** - Download multiple books simultaneously
- **Permission checks** before downloading
- **Better error handling** and user feedback
- **Reload book list** after partial downloads

**Parallel Download Implementation:**
```cpp
// NEW: Support for multiple simultaneous downloads
QList<FileDownloader*> activeDownloaders;
int maxParallelDownloads;
int completedDownloads;
int failedDownloads;
```

**Stop Download Function:**
```cpp
void MobileApp::on_stopDownloadBTN_clicked() {
    // Abort all active downloads
    for (FileDownloader* downloader : activeDownloaders) {
        if (downloader) {
            disconnect(downloader, nullptr, this, nullptr);
            downloader->abort();
            downloader->deleteLater();
        }
    }
    activeDownloaders.clear();
    downloadsList.clear();
    
    // Show how many files were downloaded before stopping
    if (completedDownloads > 0) {
        ui->downloadInfo->setText(tr("Download stopped. %1 file(s) downloaded successfully.").arg(completedDownloads));
    }
    
    // Reload book list to show partial downloads
    reloadBooklist();
}
```

#### 3. Mobile/mobileapp.h (+94, -8)
**Changes:**
- **New event handlers:** `mousePressEvent()`, `mouseMoveEvent()` (original only had `mouseReleaseEvent()`)
- **New includes:** QMessageBox, QSet, android support classes
- **Parallel download support:** `activeDownloaders`, `maxParallelDownloads`, `completedDownloads`, `failedDownloads`
- **Touch tracking:** `lastPressPos`, `touchMoved`, `touchMoveTimer`
- **Android support pointers:** `android16Support`, `edgeToEdgeSupport`, `themeSupport`
- **New method declarations** for delete/download/touch features

**Event Handler Changes:**
```cpp
// Original: Only had mouseReleaseEvent
void mouseReleaseEvent(QMouseEvent *ev);

// Fork: Added press and move for touch tracking
void mousePressEvent(QMouseEvent *ev) override;
void mouseMoveEvent(QMouseEvent *ev) override;
void mouseReleaseEvent(QMouseEvent *ev) override;
```

**Download Support:**
```cpp
// NEW: Parallel download tracking
QList<FileDownloader*> activeDownloaders;
int maxParallelDownloads;
int completedDownloads;
int failedDownloads;

// NEW: Touch movement tracking
QPoint lastPressPos;
bool touchMoved;
QElapsedTimer touchMoveTimer;
```

### Medium Changes (20-99 lines)

#### 4. OraytaBase/filedownloader.cpp (+59, -2)
**Changes:**
- **Error handling:** Check if file opens successfully before downloading
- **Debug logging:** Added logging for download start, URL, and target path
- **Failure handling:** Emit error signal if file can't be opened for writing

**Specific Changes:**
```cpp
// Added file open check:
if (!mTargetFile.open(QIODevice::ReadWrite)) {
    qDebug() << "Failed to open file for writing:" << rTarget;
    emit downloadError();
    return;
}

// Added debug logging:
qDebug() << "Starting download from:" << rUrl;
qDebug() << "Saving to:" << rTarget;
```

#### 5. main.cpp (+40, -21)
**Qt6 Migration:**
- `QDesktopWidget` → `QScreen`
- Desktop UI disabled (commented out)

**Android Workarounds:**
```cpp
qputenv("QT_ANDROID_DISABLE_ACCESSIBILITY", "1");  // Prevent Qt 6.10 crashes
qputenv("QSG_RHI_BACKEND", "software");            // Use software rendering
app.setLayoutDirection(Qt::RightToLeft);           // RTL for Hebrew
```

#### 6. Mobile/jnifunc.cpp (+39, -13)
**Qt6 JNI Migration:**

**Include Changes:**
```cpp
// Original:
#include <QAndroidJniObject>
#include <QAndroidJniEnvironment>
#include <QApplication>

// Fork:
#include <QJniObject>
#include <QJniEnvironment>
#include <QCoreApplication>
// QNativeInterface is included via QCoreApplication in Qt6
```

**API Changes:**
- `QAndroidJniObject` → `QJniObject` (all occurrences)
- `QAndroidJniEnvironment` → `QJniEnvironment`
- `QtAndroid::androidActivity()` → `QNativeInterface::QAndroidApplication::context()`
- Updated all JNI method calls to use Qt6 API

#### 7. Mobile/textdisplayer.cpp (+21, -11)
**Changes:**
- **Iterator validation:** Check if iterator is empty before displaying
- **Better debug logging:** More descriptive debug messages
- **Code cleanup:** Improved comments

**Specific Changes:**
```cpp
// Added iterator validation:
if (!itr.isEmpty()) {
    display(book, itr);
}

// Improved debug messages:
// qDebug() << "Processing anchor click:" << url;
// qDebug() << "Loading chapter:" << itr.toString() << "from book:" << book->getName();
```

#### 8. OraytaBase/bmarklist.cpp (+21, -7)
**Changes:**
- **Mouse event handling:** Separated press and move events properly
- **Touch support:** Better touch/click detection

**Specific Changes:**
```cpp
// Original: Used mouseMoveEvent for press detection
void BMarkList::mouseMoveEvent(QMouseEvent* event) {
    clickT = QTime::currentTime();
}

// Fork: Proper mousePressEvent
void BMarkList::mousePressEvent(QMouseEvent* event) {
    pressTime = QTime::currentTime();
    pressPos = event->pos();
    QListWidget::mousePressEvent(event);
}
```

### Small Changes (1-19 lines)

#### 9. OraytaBase/functions.cpp (+19, -17)
**Qt6 Migration:**
- `QRegExp` → `QRegularExpression` in `withNikudAndTeamim()`
- Updated regex pattern handling

#### 10. Mobile/mobileapp_bookmark.cpp (+17, -7)
- Bookmark UI improvements

#### 11. OraytaBase/book.cpp (+16, -13)
**Changes:**
- **Default state fix:** Books now default to unselected (GREY) instead of selected (BLUE)
- **Qt6 text codec:** Replaced `QTextCodec::codecForHtml()` with `QString::fromUtf8()`
- **Qt6 regex:** `QRegExp` → `QRegularExpression`
- **Qt6 string split:** `QString::SkipEmptyParts` → `Qt::SkipEmptyParts`

```cpp
// Fixed default state:
mInSearch = false;  // was: true
mIconState = GREY;  // was: BLUE

// Qt6 text handling:
QString textData = QString::fromUtf8(data);
text = textData.replace(QRegularExpression("<[^>]*>"), "").split('\n', Qt::SkipEmptyParts);
```

#### 12. OraytaBase/bookfind.cpp (+9, -3)
**Changes:**
- **Removed Desktop include:** Removed `#include "Desktop/desktopapp.h"`
- **RTL search input:** Added proper RTL layout and styling for Hebrew search
- **Placeholder text:** Added Hebrew placeholder with RTL mark

```cpp
// Added RTL search input configuration:
searchLine->setAlignment(Qt::AlignRight | Qt::AlignVCenter | Qt::AlignAbsolute);
searchLine->setLayoutDirection(Qt::RightToLeft);
searchLine->setPlaceholderText(QString::fromUtf16(u"\u200Fחפש ספר..."));
```

#### 13. OraytaBase/search.cpp (+8, -8)
**Changes:**
- **Qt6 string split:** `QString::SkipEmptyParts` → `Qt::SkipEmptyParts`
- **Qt6 regex:** `QRegExp` → `QRegularExpression` in function signature
- **Text improvements:** Better spacing and formatting in search results
- **Translation improvements:** Better Hebrew result messages

```cpp
// Function signature change:
QUrl SearchInBooks(const QRegularExpression& regexp, ...)  // was: QRegExp

// Improved result message:
QObject::tr("Showing first %1 results only").arg(results)  // was: separate strings
```

#### 14. OraytaBase/booklist.cpp (+6, -6)
**Changes:**
- **Qt6 regex:** All `QRegExp` → `QRegularExpression`
- **Qt6 string split:** `QString::SkipEmptyParts` → `Qt::SkipEmptyParts`

```cpp
// All regex replacements updated:
Name.replace(QRegularExpression("^[0-9 ]*"), "")  // was: QRegExp
```

#### 15. OraytaBase/bmarklist.h (+4, -2)
- Header updates

#### 16. OraytaBase/functions.h (+3, -3)
**Qt6 Migration:**
```cpp
// Changed:
#include <QTextCodec>           → #include "qt6_textcodec_compat.h"
#include <QRegExp>              → #include <QRegularExpression>
QRegExp withNikudAndTeamim()    → QRegularExpression withNikudAndTeamim()
```

#### 17. OraytaBase/book.h (+3, -3)
- Header updates

#### 18. OraytaBase/filedownloader.h (+3, -0)
- New method declarations

#### 19. OraytaBase/htmlgen.cpp (+3, -3)
**Changes:**
- **Null check fix:** `NULL == userCss` → `userCss.isNull()` (modern C++ style)
- **Encoding fix:** `"ISO-88598"` → `"ISO-8859-8"` (correct encoding name)
- **Qt6 regex:** `QRegExp` → `QRegularExpression` in function signature

```cpp
// Fixed encoding name:
QTextCodec * codec = QTextCodec::codecForName("ISO-8859-8");  // was: ISO-88598

// Function signature:
QUrl Book::renderChapterHtml(..., QRegularExpression mark)  // was: QRegExp
```

#### 20. OraytaBase/guematria.cpp (+2, -2)
**Changes:**
- **Qt6 regex:** All `QRegExp` → `QRegularExpression`

```cpp
QRegularExpression notText("[^ א-ת]");  // was: QRegExp
text[i].replace(QRegularExpression("..."), "\\1");  // was: QRegExp
```

#### 21. OraytaBase/search.h (+2, -1)
- Header updates

#### 22. Mobile/textdisplayer.h (+1, -0)
- Header updates

#### 23. Mobile/swipegesturerecognizer.cpp (+1, -1)
- Minor fix

---

## Files Unchanged (10 files)

These files are identical in both projects:
- Mobile/jnifunc.h
- Mobile/swipegesturerecognizer.h
- OraytaBase/bookfind.h
- OraytaBase/bookiter.cpp/h
- OraytaBase/booklist.h
- OraytaBase/guematria.h
- OraytaBase/htmlgen.h
- OraytaBase/minibmark.cpp/h

---

## Build System Changes

### Removed
- `Orayta.pro` (qmake project file)
- `BuildDeb.sh` (Debian build script)
- `Orayta.desktop` (Linux desktop shortcut)
- `debian/` directory (entire Debian packaging - 15+ files)
- `TodoList` file

### Added
- `CMakeLists.txt` (root CMake configuration)
- `OraytaBase/CMakeLists.txt` (core library)
- `Mobile/CMakeLists.txt` (mobile UI)
- `build-android-arm64.ps1` (ARM64 build script)
- `build-android-armv7.ps1` (ARMv7 build script)
- `build-android-with-books.ps1` (offline APK with books)
- `compile-and-update-translations.ps1` (translation compiler)

**CMake Configuration:**
- C++17 standard (required for Qt6)
- Qt6 packages: Core, Gui, Widgets, Network, PrintSupport
- Android-specific: CorePrivate for JNI
- Multi-ABI support: arm64-v8a, armeabi-v7a, x86, x86_64

---

## Android Configuration Changes

### AndroidManifest.xml

**SDK Versions:**
```xml
<!-- Original -->
<uses-sdk android:minSdkVersion="16" android:targetSdkVersion="29"/>

<!-- Fork -->
<uses-sdk android:minSdkVersion="28" android:targetSdkVersion="35"/>
```

**Permissions:**
```xml
<!-- Original: Simple permissions -->
<uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE"/>
<uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE"/>

<!-- Fork: Granular permissions with SDK limits -->
<uses-permission android:name="android.permission.READ_MEDIA_IMAGES"/>
<uses-permission android:name="android.permission.READ_MEDIA_VIDEO"/>
<uses-permission android:name="android.permission.READ_MEDIA_AUDIO"/>
<uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE" android:maxSdkVersion="32"/>
<uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" android:maxSdkVersion="32"/>
```

**16KB Page Size Support:**
```xml
<application android:extractNativeLibs="true">
```

**Qt6 Workarounds:**
```xml
<meta-data android:name="android.app.extract_android_style" android:value="minimal"/>
<meta-data android:name="android.app.accessibility.enabled" android:value="false"/>
<meta-data android:name="android.app.auto_dark_mode_handling" android:value="false"/>
```

**Activity Name:**
```xml
<!-- Original -->
<activity android:name="org.qtproject.qt5.android.bindings.QtActivity">

<!-- Fork -->
<activity android:name="org.Orayta.OraytaActivity">
```

### Gradle Configuration

**Android Gradle Plugin:** 3.x → 8.7.3  
**Gradle Wrapper:** 5.x → 8.11.1  
**Compile SDK:** 28 → 35  
**Target SDK:** 28 → 35

**New Properties:**
```properties
android.bundle.enableUncompressedNativeLibs=false
android.enableR8.fullMode=true
```

---

## OpenSSL Libraries

**New Directory:** `openssl-android/`

Contains OpenSSL 3.x libraries for HTTPS/TLS:
```
openssl-android/
├── arm64-v8a/libssl_3.so, libcrypto_3.so
├── armeabi-v7a/libssl_3.so, libcrypto_3.so
├── x86/libssl_3.so, libcrypto_3.so
└── x86_64/libssl_3.so, libcrypto_3.so
```

**Purpose:** Secure HTTPS downloads for books and updates

---

## Qt5 → Qt6 API Changes

### Core API Replacements

| Qt5 API | Qt6 Replacement | Files Affected |
|---------|----------------|----------------|
| `QTextCodec` | `qt6_textcodec_compat.h` | functions.h, functions.cpp |
| `QRegExp` | `QRegularExpression` | functions.h, functions.cpp, book.cpp |
| `QDesktopWidget` | `QScreen` | main.cpp |
| `QAndroidJniObject` | `QJniObject` | jnifunc.cpp |
| `QtAndroid` | `QNativeInterface::QAndroidApplication` | jnifunc.cpp |

### Text Codec Compatibility Layer

**File:** `OraytaBase/qt6_textcodec_compat.h`

Provides QTextCodec API using QStringConverter internally:
- Supports UTF-8 and ISO-8859-8 (Hebrew)
- Maintains same API as Qt5
- Macro: `SET_TEXTSTREAM_CODEC(stream, codec)`

---

## New Features Summary

### 1. Delete Books
- Toggle delete mode with button
- Click books/categories to delete
- Confirmation dialogs
- Preserves tree expansion state
- Permission checks

### 2. Stop Downloads
- Cancel button during downloads
- Shows completed count
- Reloads book list
- Clears download queue
- Parallel download support

### 3. Runtime Permissions
- Checks before file operations
- Handles Android 13+ granular permissions
- User-friendly rationale dialogs

### 4. Tree State Management
- Saves expanded categories
- Restores after operations
- Uses book paths as identifiers

### 5. Styled Dialogs
- App-themed message boxes
- Consistent styling
- Better visual integration

### 6. Toolbar Style Options
- **Text and Icons** (default) - Shows both text and icons
- **Text Only** - Shows only text labels
- **Icons Only** - Shows only icons
- Saved in settings, applies to all toolbar buttons

**Implementation:**
```cpp
void MobileApp::applyToolbarStyle(int style) {
    Qt::ToolButtonStyle buttonStyle;
    switch(style) {
        case 0: buttonStyle = Qt::ToolButtonTextUnderIcon; break;  // Text and Icons
        case 1: buttonStyle = Qt::ToolButtonTextOnly; break;       // Text Only
        case 2: buttonStyle = Qt::ToolButtonIconOnly; break;       // Icons Only
    }
    // Apply to all toolbar buttons
    for (QToolButton* btn : toolButtons) {
        btn->setToolButtonStyle(buttonStyle);
    }
}
```

### 7. Touch/Scroll Improvements
- **Prevents accidental clicks while scrolling** - 8px movement threshold
- **Delayed click processing** - 50ms delay to allow Android back gesture detection
- **Double-click prevention** - 150ms debounce timer
- **Category expand on click** - Single click now expands/opens categories

**Tree Widget Click Handler:**
```cpp
void MobileApp::on_treeWidget_itemClicked(QTreeWidgetItem *item, int column) {
    // Ignore if user was scrolling
    if (touchMoved) return;
    
    // Prevent double-click on some Android devices
    qint64 miliSec = timer.restart();
    if (miliSec < 150) return;
    
    // Delay to allow edge swipe detection (Android back gesture)
    QTimer::singleShot(50, this, [this, item, column]() {
        if (!touchMoved) {
            on_treeWidget_itemDoubleClicked(item, column);
        }
    });
}
```

### 8. Search Results Visual Improvements
- Better spacing in result messages (from search.cpp changes)
- Improved Hebrew text formatting
- Cleaner result count display

---

## Statistics

- **Total Files Changed:** 54 files
  - Added: 9 files
  - Removed: 22 files
  - Modified: 23 files
  - Unchanged: 10 files (verified identical)

- **Lines Changed:** ~2,500 lines added, ~300 lines removed

- **Build System:** Complete migration from qmake to CMake

- **Android SDK:** Min 16→28, Target 29→35

- **Qt Version:** Qt5 → Qt6.10

---

## Verification Method

All changes verified using PowerShell `Compare-Object` on actual source files.

**Last Updated:** November 4, 2025
