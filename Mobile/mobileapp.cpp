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
* Authors:
* Izar <izar00@gmail.com>
* Moshe Wagner. <moshe.wagner@gmail.com>
*/


/*TODO:
 *
  - Make font sizes consistant...
  - Crashes on exit (sometimes)
  - Exit always takes a long time
  - Can't navigate from links from gmara (swipes dont work)

*/




#include "mobileapp.h"
#include "ui_mobileapp.h"
#include "../OraytaBase/functions.h"
#include "../OraytaBase/booklist.h"
#include <QStandardPaths>
#include "../OraytaBase/search.h"
#include <QMessageBox>
#include <QDebug>
#include <QFile>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QButtonGroup>
#include <QDir>
#include <QCloseEvent>
#include <QSettings>
#include <QListWidget>
#include <QListWidgetItem>
#include <QTimer>
#include <QScreen>
#include <QEasingCurve>
#include <QMenu>
#include <QScroller>
#include <QClipboard>
#include <QDesktopServices>
#include <QRegularExpression>
#include <QTouchEvent>
#include <QGuiApplication>
#include <QInputMethod>

#ifdef Q_OS_ANDROID
    #include <jni.h>
    #include "jnifunc.h"
    #include "permissionutils.h"
#endif

#include "../OraytaBase/quazip/quazipfile.h"

// Global
#define DEF_FONT "Droid Sans Hebrew Orayta"

QString gFontFamily = DEF_FONT;
int gFontSize = 0;

// Declare external global (defined in OraytaBase/functions.cpp)
extern bool nightMode;

#ifdef Q_OS_ANDROID
// Global pointer to the MobileApp instance for JNI callback
MobileApp* g_mobileAppInstance = nullptr;
#endif

MobileApp::MobileApp(QWidget *parent) :QDialog(parent), ui(new Ui::MobileApp)
{
#ifdef Q_OS_ANDROID
    // Set global instance for JNI callback
    extern MobileApp* g_mobileAppInstance;
    g_mobileAppInstance = this;
#endif

#ifdef TIMEDBG
    qDebug() << "Mobile start" << QTime::currentTime();
#endif

    //Set all QString to work with unicode
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("utf8"));

    //set the display to native android look.
    //QApplication::setStyle(new QAndroidStyle()); //dosnt work

    ui->setupUi(this);

#ifdef TIMEDBG
    qDebug() << "setupUI"<<QTime::currentTime();
#endif

    // Add text labels to icon buttons as fallback when icons don't load
    // Use tr() for proper translation support
    ui->settingsBTN->setText(tr("Settings"));
    ui->settingsBTN->setProperty("originalText", tr("Settings"));
    ui->helpBTN->setText(tr("Help"));
    ui->helpBTN->setProperty("originalText", tr("Help"));
    ui->aboutBTN->setText(tr("About"));
    ui->aboutBTN->setProperty("originalText", tr("About"));
    ui->openBTN->setText(tr("Browse"));
    ui->searchBTN->setText(tr("Search"));
    ui->getbooksBTN->setText(tr("Download"));
    ui->bookMarksBTN->setText(tr("Bookmarks"));
    
    // Set icons for main page buttons
    ui->settingsBTN->setIcon(QIcon(":Icons/configure.png"));
    ui->helpBTN->setIcon(QIcon(":Icons/help-contents.png"));
    ui->aboutBTN->setIcon(QIcon(":Icons/help-about.png"));
    ui->openBTN->setIcon(QIcon(":Icons/Orayta.png"));
    ui->searchBTN->setIcon(QIcon(":Icons/booksearch.png"));
    ui->getbooksBTN->setIcon(QIcon(":Icons/dl-book.png"));
    ui->bookMarksBTN->setIcon(QIcon(":Icons/bookmarks-big.png"));
    
    // Set icons for other buttons
    ui->clearSearchBTN->setIcon(QIcon(":Icons/edit-delete.png"));
    
    // Display page buttons - use symbols with fallback font
    // Font size will be set dynamically in adjustFontSize() based on screen DPI
    ui->menuBTN->setText("☰");  // Hamburger menu
    ui->toIndexMenuBTN->setText("↑");  // Up arrow
    ui->toMainMenuBTN->setText("⌂");  // Home symbol
    // Books are in Hebrew (RTL), so arrows match Hebrew reading direction
    ui->backBTN->setText("←");  // Back = left arrow (previous page)
    ui->forwardBTN->setText("→");  // Forward = right arrow (next page)

    //show the about page while app loads

    ui->stackedWidget->setCurrentIndex(ABOUT_PAGE);

    QApplication::processEvents();

    timer.start();
    touchMoved = false;
    touchMoveTimer.start();
    tabSwipeInProgress = false;
    isVerticalScrolling = false;
    selectionAreaScrolling = false;
    textDisplayerScrolling = false;
    
    // Initialize parallel download settings
    maxParallelDownloads = 10;  // Download 10 files at once for maximum speed (good for GitHub)
    completedDownloads = 0;
    failedDownloads = 0;

    copyAssetsToDisk();

    //set stuff as null only for pertection
    viewHistory = NULL;
    listdownload = NULL;
    downloader = NULL;
    menu = NULL;
    bm = NULL;
    action = NULL;
    autoInstallKukBooksFlag=false;

    //Initialize a new FileDownloader to download the list
    listdownload = new FileDownloader();
    connect(listdownload, SIGNAL(done()), this, SLOT(listDownloadDone()));
    //Initialize a new FileDownloader object for books downloading
    downloader = new FileDownloader();


    autoInstallKukBooksFlag=false;

    //Initialize the bookdisplayer object
    displayer = new textDisplayer(this, &bookList);
    ui->displayArea->layout()->addWidget(displayer);
    ui->displayArea->layout()->addWidget(ui->loadBar);

    //Initialize bookFind page
    bookFindDialog = new bookfind(this, bookList);
    ui->findBook->layout()->addWidget(bookFindDialog);
    connect(bookFindDialog, SIGNAL(openBook(int)), this, SLOT(showBook(int)));


    connect(displayer, SIGNAL(sourceChanged(QUrl)), this, SLOT(titleUpdate(QUrl)));

    connect(displayer, SIGNAL(loadStart()), this, SLOT(tdloadStarted()));
    connect(displayer, SIGNAL(loadEnd(QUrl, Book*, BookIter)), this, SLOT(tdloadFinished(QUrl, Book*, BookIter)));

    exclude << SETTINGS_PAGE << MIXED_SELECTION_PAGE ;//<< HISTORY_PAGE;
    viewHistory = new QList<int>;
    //The base of history should always point to the main page
    viewHistory->append(MAIN_PAGE);
    connect(ui->stackedWidget, SIGNAL(currentChanged(int)), this, SLOT(viewChanged(int)));

    // setup the search page
    showHideSearch(false);
    
    // Fix RTL alignment: Use AlignAbsolute to make AlignRight mean physical right edge
    // Without AlignAbsolute, AlignRight in RTL layout means trailing edge (left side)
    QLineEdit *searchLine = ui->searchInBooksLine;
    searchLine->setStyleSheet(QString()); // Remove any conflicting CSS
    searchLine->setAlignment(Qt::AlignRight | Qt::AlignVCenter | Qt::AlignAbsolute);
    searchLine->setLayoutDirection(Qt::RightToLeft);
    
    // Disable clear button to prevent accidental clears
    searchLine->setClearButtonEnabled(false);
    
    // Add RLM (Right-to-Left Mark) to placeholder for proper bidi
    searchLine->setPlaceholderText(QString::fromUtf16(u"\u200Fחיפוש..."));
    
    // Disable auto-corrections that can reset cursor
    searchLine->setInputMethodHints(Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText);
    
    // Ensure strong focus policy for immediate keyboard activation
    searchLine->setFocusPolicy(Qt::StrongFocus);
    searchLine->setAttribute(Qt::WA_InputMethodEnabled, true);

    QList<QWidget *> addscroll;
    addscroll << displayer << ui->treeWidget << ui->SearchTreeWidget << ui->staticBookMarkList << ui->dailyLearningList << ui->historyBookmarkList << ui->scrollArea;
    addscroll << ui->downloadListWidget;

#ifdef TIMEDBG
    qDebug() << "UI stuff 1"<<QTime::currentTime();
#endif

    // Install event filter on download list
    ui->downloadListWidget->installEventFilter(this);
    ui->downloadListWidget->viewport()->installEventFilter(this);
    
    // Install event filter on tree widgets to track touch movement
    ui->treeWidget->installEventFilter(this);
    ui->treeWidget->viewport()->installEventFilter(this);
    ui->SearchTreeWidget->installEventFilter(this);
    ui->SearchTreeWidget->viewport()->installEventFilter(this);
    
    // Install event filter on selection area (chapters/sources list) to prevent accidental clicks
    ui->selectionArea->installEventFilter(this);
    ui->selectionArea->viewport()->installEventFilter(this);
    
    // Install event filter on text displayer to prevent accidental chapter clicks after scrolling
    displayer->installEventFilter(this);
    displayer->viewport()->installEventFilter(this);
    
    // Install event filter for swipe navigation between tabs
    // Bookmarks window (tabWidget_2)
    ui->staticBookMarkList->installEventFilter(this);
    ui->staticBookMarkList->viewport()->installEventFilter(this);
    ui->dailyLearningList->installEventFilter(this);
    ui->dailyLearningList->viewport()->installEventFilter(this);
    ui->historyBookmarkList->installEventFilter(this);
    ui->historyBookmarkList->viewport()->installEventFilter(this);
    
    // Settings window (tabWidget) - install on all tab pages
    for (int i = 0; i < ui->tabWidget->count(); ++i) {
        QWidget *tabPage = ui->tabWidget->widget(i);
        if (tabPage) {
            tabPage->installEventFilter(this);
        }
    }

    for (QWidget * w : addscroll)
    {
        // For QScrollArea, apply scroller to viewport for better performance
        QWidget* scrollTarget = w;
        if (QScrollArea* scrollArea = qobject_cast<QScrollArea*>(w)) {
            scrollTarget = scrollArea->viewport();
        }
        
        // Use TouchGesture for native Android feel, fallback to LeftMouseButton for desktop
        #ifdef Q_OS_ANDROID
        QScroller::grabGesture(scrollTarget, QScroller::TouchGesture);
        #else
        QScroller::grabGesture(scrollTarget, QScroller::LeftMouseButtonGesture);
        #endif

        QScroller* s = QScroller::scroller(scrollTarget);
        QScrollerProperties p = s->scrollerProperties();
        
        // Disable overshoot/bounce for cleaner scrolling
        p.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
        p.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, QScrollerProperties::OvershootAlwaysOff);
        p.setScrollMetric(QScrollerProperties::OvershootDragResistanceFactor, 0.9);
        p.setScrollMetric(QScrollerProperties::OvershootDragDistanceFactor, 0);
        p.setScrollMetric(QScrollerProperties::OvershootScrollDistanceFactor, 0);
        p.setScrollMetric(QScrollerProperties::OvershootScrollTime, 0.3);

        // Balanced drag start distance - responsive but distinguishes taps from scrolls
        p.setScrollMetric(QScrollerProperties::DragStartDistance, 0.005);
        
        // Increased delay to better distinguish tap from drag gesture
        p.setScrollMetric(QScrollerProperties::MousePressEventDelay, 0.2);
        
        // Smooth velocity tracking during drags
        p.setScrollMetric(QScrollerProperties::DragVelocitySmoothingFactor, 0.15);
        
        // Smoother scrolling curve - OutExpo for exponential ease-out like native apps
        p.setScrollMetric(QScrollerProperties::ScrollingCurve, QEasingCurve::OutExpo);
        
        // Stricter axis locking for cleaner vertical scrolling
        p.setScrollMetric(QScrollerProperties::AxisLockThreshold, 0.95);
        
        // Improved momentum and deceleration for smoother feel
        p.setScrollMetric(QScrollerProperties::DecelerationFactor, 0.28);  // Slower deceleration = longer momentum
        p.setScrollMetric(QScrollerProperties::MinimumVelocity, 100);  // Minimum speed before stopping
        p.setScrollMetric(QScrollerProperties::MaximumVelocity, 4500);  // Maximum flick speed
        p.setScrollMetric(QScrollerProperties::MaximumClickThroughVelocity, 50);  // Higher threshold prevents accidental clicks during scrolling
        
        // Smoother flick behavior
        p.setScrollMetric(QScrollerProperties::AcceleratingFlickSpeedupFactor, 4.0);  // Faster acceleration
        p.setScrollMetric(QScrollerProperties::AcceleratingFlickMaximumTime, 2.0);  // Longer acceleration time
        
        // Frame rate for smooth animation (60 FPS)
        p.setScrollMetric(QScrollerProperties::FrameRate, QScrollerProperties::Fps60);

        s->setScrollerProperties(p);
    }

#ifdef TIMEDBG
    qDebug() << "Qscroller"<<QTime::currentTime();
#endif


    reloadBooklist();

#ifdef TIMEDBG
    qDebug() << "Books loaded" <<QTime::currentTime();
#endif


    //Connect slots to the signalls of the book downloader
    connect(downloader, SIGNAL(done()), this, SLOT(downloadDone()));
    connect(downloader, SIGNAL(downloadProgress(int)), this, SLOT(downloadProgress(int)));
    connect(downloader, SIGNAL(downloadError()), this, SLOT(downloadError()));


    ui->downloadPrgBar->hide();

    //IZAR
    // hack to enable me to test downloads without internet
    // listDownloadDoneOverride();

    // the default for the menu in display page is hidden.
    ui->dispalyMenu->hide();

#ifdef TIMEDBG
    qDebug() << "UI2"<<QTime::currentTime();
#endif



    //load saved settings
    setupSettings();

#ifdef TIMEDBG
    qDebug() << "Settings" <<QTime::currentTime();
#endif


    adjustToScreenSize();

#ifdef TIMEDBG
    qDebug() << "Screensize" <<QTime::currentTime();
#endif

    //Download daily dd file
    downloadDailyLimudFiles();

    setupBookmarkList();

#ifdef TIMEDBG
    qDebug() << "Bm list" <<QTime::currentTime();
#endif

    connect (ui->staticBookMarkList, SIGNAL(shortPress(QListWidgetItem*)), this, SLOT(BMShortClicked(QListWidgetItem*)));
    connect (ui->dailyLearningList, SIGNAL(shortPress(QListWidgetItem*)), this, SLOT(BMShortClicked(QListWidgetItem*)));
    connect (ui->historyBookmarkList, SIGNAL(shortPress(QListWidgetItem*)), this, SLOT(BMShortClicked(QListWidgetItem*)));

    QApplication::processEvents();

//    initCrypterRequest();

    //Reopen last book, if relevant
    //get last open book
    QSettings settings("Orayta", "SingleUser");
    settings.beginGroup("History");
        int page = settings.value("lastPage").toInt();
        int lastBookId = settings.value("lastBook").toInt();
        Book *b = bookList.findBookById(lastBookId);
        BookIter itr = BookIter::fromEncodedString(settings.value("position", "").toString());
        lastViewPosition =  settings.value("viewposition").toInt();
    settings.endGroup();

    // restore the bookmark list
    ui->historyBookmarkList->loadHistory(bookList);
    ui->staticBookMarkList->loadHistory(bookList);

#ifdef TIMEDBG
    qDebug() << "last book" <<QTime::currentTime();
#endif


    if (page != DISPLAY_PAGE || !b) ui->stackedWidget->setCurrentIndex(MAIN_PAGE);
    else
    {
        showBook(b, itr);

        //Yuchy hack. but I culdn't get it to work otherwise...
        QTimer::singleShot(100, this, SLOT(jumpToLastPos()));
    }

    ui->bmLBL->hide();
    ui->deleteBookLBL->hide();

    ui->stackedWidget->currentWidget()->setFocus();


#ifdef TIMEDBG
    qDebug() << "Jump" <<QTime::currentTime();
#endif


    adjustToScreenSize();

#ifdef TIMEDBG
    qDebug() << "Adjust 2" <<QTime::currentTime();
#endif

#ifdef ANDROID
    // Initialize Android 16 support
    android16Support = new Android16Support(this);
    
    // Log system information for debugging
    android16Support->logSystemInfo();
    
    // Verify 16KB page size support
    if (android16Support->verify16KBPageSizeSupport()) {
        qDebug() << "16KB page size support verified successfully";
    } else {
        qWarning() << "16KB page size support verification failed";
    }
    
    // Check Android 16 features
    if (android16Support->checkAndroid16Features()) {
        qDebug() << "Android 16 features check passed";
    } else {
        qWarning() << "Android 16 features check failed";
    }
    
    // Initialize edge-to-edge display support
    edgeToEdgeSupport = new EdgeToEdgeSupport(this);
    
    // Log edge-to-edge information
    edgeToEdgeSupport->logEdgeToEdgeInfo();
    
    // Enable edge-to-edge display if supported
    if (edgeToEdgeSupport->isEdgeToEdgeSupported()) {
        if (edgeToEdgeSupport->enableEdgeToEdge()) {
            qDebug() << "Edge-to-edge display enabled successfully";
            
            // Set light system bars for better visibility
            edgeToEdgeSupport->setSystemBarAppearance(true, true);
            
            // Apply top inset for status bar, but not bottom for navigation bar
            // since we want the content to extend behind the navigation bar
            edgeToEdgeSupport->applySystemBarInsets(this, true, false, false, false);
            
        } else {
            qWarning() << "Failed to enable edge-to-edge display";
        }
    } else {
        qDebug() << "Edge-to-edge display not supported on this device";
    }
    
    // Initialize theme support for auto-detection
    themeSupport = new ThemeSupport(this);
    
    // On first run, auto-detect system theme
    QSettings themeSettings("Orayta", "SingleUser");
    themeSettings.beginGroup("Confs");
    if (!themeSettings.contains("nightMode")) {
        // First run - detect system theme
        bool systemDark = themeSupport->detectSystemDarkMode();
        nightMode = systemDark;
        themeSettings.setValue("nightMode", nightMode);
        qDebug() << "First run - Auto-detected theme, dark mode:" << nightMode;
    } else {
        // Load saved preference
        nightMode = themeSettings.value("nightMode", false).toBool();
        qDebug() << "Loaded saved theme, dark mode:" << nightMode;
    }
    themeSettings.endGroup();
#endif

    //displayKukaytaMessage();
}

void MobileApp::resizeEvent(QResizeEvent *)
{
    adjustToScreenSize();
}

void MobileApp::copyAssetsToDisk()
{
#ifdef Q_OS_ANDROID
    // Check if we have storage permissions before copying assets
    if (!PermissionUtils::canWriteExternalStorage()) {
        qDebug() << "No write permission for copying assets, requesting permission...";
        PermissionUtils::requestStoragePermissions([this](bool granted) {
            if (granted) {
                qDebug() << "Permission granted, copying assets to disk";
                performAssetCopy();
            } else {
                qWarning() << "Storage permission denied, cannot copy assets to disk";
                // Show user-friendly message about limited functionality
                QMessageBox::warning(this, tr("Permission Required"), 
                    tr("Storage permission is required to set up the application. "
                       "Some features may not work properly without this permission."));
            }
        });
        return;
    }
#endif
    
    performAssetCopy();
}

void MobileApp::performAssetCopy()
{
    QDir *dir = new QDir("assets:/Orayta/");

    copyFolder(dir->absolutePath(), QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/Orayta/", QStringList("*.*"));
}

//Adjust UI properties depending on device screen size
void MobileApp::adjustToScreenSize()
{
    QScreen* screen = QApplication::primaryScreen();
    QSize size = screen->availableGeometry().size();
// hack to enable desktop usage as well as
#ifndef Q_OS_ANDROID
    size = this->size();
#endif

    //Mazimize the app
    resize(size);

    //Adjust main page icons:
    int w = (size.width() / 2);
    int max = 1000;

    //Round to the closest 10
    w = int(w / 10) * 10;

    ui->gridLayout->setVerticalSpacing(w / 80);
    ui->gridLayout->setHorizontalSpacing(w / 80);

    QSize a(w -20, w - 20);
    //qDebug() << "Icon size:" << a;

    int h = (size.height() / 15);
    //ui->aboutBTN->setIconSize(a);
    ui->aboutBTN->setMaximumSize(w,h);
    ui->settingsBTN->setMaximumSize(w,h);
    //ui->helpBTN->setIconSize(a);
    ui->helpBTN->setMaximumSize(w,h);

    ui->openBTN->setIconSize(a);
    ui->openBTN->setMaximumSize(w,max);
    ui->searchBTN->setIconSize(a);
    ui->searchBTN->setMaximumSize(w,max);
    ui->getbooksBTN->setIconSize(a);
    ui->getbooksBTN->setMaximumSize(w,max);
    ui->bookMarksBTN->setIconSize(a);
    ui->bookMarksBTN->setMaximumSize(w,max);

    int wth = size.width();
    ui->treeWidget->setColumnWidth(0, wth);
    ui->SearchTreeWidget->setColumnWidth(0, wth);

    // if the font size wasn't set manually by the user, we will calculate optimal values
    // based on screen DPI and physical size for consistent readability

    if (gFontSize < 1)
    {
        QScreen* screen = QApplication::primaryScreen();
        int dpix = screen->physicalDotsPerInchX();
        int dpiy = screen->physicalDotsPerInchY();
        int dpi = (dpix+dpiy)/2;

        qDebug() << "x: " << dpix << " y: " << dpiy << " average: " << dpi;

        // Unified font size calculation: base size scales linearly with DPI
        // This ensures consistent physical text size across all devices
        // Formula: fontSize = baseDPI * scaleFactor, where scaleFactor = dpi / 160 (Android baseline)
        // Clamped between reasonable min/max for readability
        int fontSize = qMax(18, qMin(50, (int)(dpi * 0.15)));  // 0.15 = 24pt at 160dpi baseline
        
        qDebug() << "Calculated font size:" << fontSize << "for DPI:" << dpi;

        gFontSize = fontSize;

        qDebug() << "=== AUTO-CALCULATING FONT SIZE ===";
        qDebug() << "gFontSize was < 1, setting to calculated value:" << gFontSize;
        qDebug() << "Note: This will be saved when app closes or user saves settings";
    }
    else
    {
        qDebug() << "=== USING LOADED FONT SIZE ===";
        qDebug() << "gFontSize is" << gFontSize << ", not recalculating";
    }

    adjustFontSize();

    // catch pause events from android
    //connect(downloader, SIGNAL(done()), this, SLOT(downloadDone()));
    connect(qApp, SIGNAL(applicationStateChanged(Qt::ApplicationState)), this, SLOT(stateChanged()));

}

int MobileApp::getAutoFontSize()
{
    // Calculate automatic interface size based on screen DPI
    // This is independent of book font size
    QScreen* screen = QApplication::primaryScreen();
    int dpix = screen->physicalDotsPerInchX();
    int dpiy = screen->physicalDotsPerInchY();
    int dpi = (dpix+dpiy)/2;
    
    // Interface font: slightly smaller than book font for better space usage
    // Formula: fontSize = baseDPI * scaleFactor * 0.7
    int fontSize = qMax(MIN_INTERFACE_SIZE, qMin(40, (int)(dpi * 0.11)));  // 0.11 = ~18pt at 160dpi
    
    qDebug() << "Auto interface font size:" << fontSize << "for DPI:" << dpi;
    
    return fontSize;
}

//set global font size to ui.
void MobileApp::adjustFontSize()
{
    int fontSize;
    
    // Check if automatic interface size is enabled
    if (ui->autoInterfaceSizeCKBX->isChecked()) {
        fontSize = getAutoFontSize();
    } else {
        fontSize = ui->interfaceSizeSpinBox->value();
        if (fontSize < MIN_INTERFACE_SIZE) // Ensure minimum size
            fontSize = MIN_INTERFACE_SIZE;
    }

    // Calculate proportional sizes based on interface font
    int symbolFontSize = fontSize * 1.3;  // Symbols slightly larger for visibility
    int mainTitleFontSize = fontSize * 1.5;  // Main page title
    int bookTitleFontSize = fontSize * 1.5;   // Book page title
    
    // Book title should be proportional to book text, not interface
    // Use gFontSize (the global variable) instead of reading from spinbox
    // because the spinbox might not be set yet during initialization
    if (gFontSize > 0) {
        bookTitleFontSize = gFontSize * 0.8; 
    }

    // Enable/Disable night mode:
    QString nStyleSheet("");
    if (nightMode) {
        nStyleSheet = "color: #7faf70; background-color:black;";
    } else {
        // Force light mode colors to override system dark theme
        nStyleSheet = "color: #000000; background-color: #FFFFFF;";
    }

    QString styleSheet("*{font-size: " +QString::number(fontSize) +"pt;" + nStyleSheet +"}");
    styleSheet += "QLabel#intro_label{font-size: " + QString::number(mainTitleFontSize) + "pt;" ;
    styleSheet += "background: transparent; border: none;}";
    styleSheet += "QLabel#bookNameLBL{font-size: " + QString::number(bookTitleFontSize) + "pt;" ;
    styleSheet += "background: transparent;}";
    
    // Ensure buttons are visible in both modes
    if (nightMode) {
        styleSheet += "QPushButton{color: #FFFFFF; background-color: #333333;}";
        styleSheet += "QToolButton{color: #FFFFFF; background-color: #333333;}";
        styleSheet += "QTextBrowser{color: #7faf70; background-color: black;}";
        styleSheet += "QLabel{color: #7faf70;}";
    } else {
        // Light mode: ensure buttons have proper contrast
        styleSheet += "QPushButton{color: #000000; background-color: #E0E0E0;}";
        styleSheet += "QToolButton{color: #000000; background-color: #E0E0E0;}";
        styleSheet += "QWidget{color: #000000; background-color: #FFFFFF;}";
        styleSheet += "QListWidget{color: #000000; background-color: #FFFFFF;}";
        styleSheet += "QTreeWidget{color: #000000; background-color: #FFFFFF;}";
        styleSheet += "QTextBrowser{color: black; background-color: white;}";
        styleSheet += "QLabel{color: black;}";
    }

     ui->stackedWidget->setStyleSheet(styleSheet);

    qDebug() << "Font sizes - Interface:" << fontSize << "Symbol:" << symbolFontSize << "MainTitle:" << mainTitleFontSize << "BookTitle:" << bookTitleFontSize;

    // Re-apply button text and icons after stylesheet change (fixes buttons losing text/icons)
    ui->settingsBTN->setText(tr("Settings"));
    ui->settingsBTN->setIcon(QIcon(":Icons/configure.png"));
    ui->helpBTN->setText(tr("Help"));
    ui->helpBTN->setIcon(QIcon(":Icons/help-contents.png"));
    ui->aboutBTN->setText(tr("About"));
    ui->aboutBTN->setIcon(QIcon(":Icons/help-about.png"));
    ui->openBTN->setText(tr("Browse"));
    ui->openBTN->setIcon(QIcon(":Icons/Orayta.png"));
    ui->searchBTN->setText(tr("Search"));
    ui->searchBTN->setIcon(QIcon(":Icons/booksearch.png"));
    ui->getbooksBTN->setText(tr("Download"));
    ui->getbooksBTN->setIcon(QIcon(":Icons/dl-book.png"));
    ui->bookMarksBTN->setText(tr("Bookmarks"));
    ui->bookMarksBTN->setIcon(QIcon(":Icons/bookmarks-big.png"));
    
    // Re-apply other button icons
    ui->clearSearchBTN->setIcon(QIcon(":Icons/edit-delete.png"));
    
    // Navigation buttons with symbols - use calculated symbol font size
    QFont symbolFont("DejaVu Sans", symbolFontSize);
    
    ui->menuBTN->setFont(symbolFont);
    ui->menuBTN->setText("☰");
    
    ui->toIndexMenuBTN->setFont(symbolFont);
    ui->toIndexMenuBTN->setText("↑");
    
    ui->toMainMenuBTN->setFont(symbolFont);
    ui->toMainMenuBTN->setText("⌂");
    
    ui->backBTN->setFont(symbolFont);
    ui->backBTN->setText("←");  // Back = left arrow (previous page)
    
    ui->forwardBTN->setFont(symbolFont);
    ui->forwardBTN->setText("→");  // Forward = right arrow (next page)
    
    // Set font size for label_10 (language warning) to interface size
    QFont warningFont;
    warningFont.setPointSize(fontSize);
    ui->label_10->setFont(warningFont);
    
    // Set book title font
    QFont bookTitleFont;
    bookTitleFont.setPointSize(bookTitleFontSize);
    ui->bookNameLBL->setFont(bookTitleFont);
    
    // Update about page colors for night mode
    QString aboutText = ui->label->text();
    if (nightMode) {
        // Replace all blue/dark colors with green
        aboutText.replace("color:#0000ff", "color:#7faf70");  // Blue links
        aboutText.replace("color:#000031", "color:#7faf70");  // Dark blue text (Hebrew)
    } else {
        // Restore original colors
        aboutText.replace("color:#7faf70", "color:#0000ff");  // Restore blue links
        // Note: #000031 will be restored when language is changed or app restarted
    }
    ui->label->setText(aboutText);
}

//Yuchy hack. but I culdn't get it to work otherwise...
void MobileApp::jumpToLastPos()
{
    ui->stackedWidget->currentWidget()->setFocus();
    displayer->setFocus();

    displayer->verticalScrollBar()->setValue(lastViewPosition);
    lastViewPosition = -1;
}

MobileApp::~MobileApp()
{
#ifdef Q_OS_ANDROID
    // Clear global instance
    extern MobileApp* g_mobileAppInstance;
    g_mobileAppInstance = nullptr;
#endif

    delete downloader;
    delete listdownload;

    //delete action;
    if (action) action->deleteLater();
    //delete menu;
    if (menu) menu->deleteLater();

#ifdef ANDROID
    delete android16Support;
    delete edgeToEdgeSupport;
    delete themeSupport;
#endif

    delete ui;
}

//IZAR
// reload the whole book list and tree
//IZAR
// reload the whole book list and tree
void MobileApp::reloadBooklist(){

    //**TIMER**//
//    qDebug()<< "main timer, elapsed: " << timer_n1.elapsed() << "begin loading books";

    //create a new empty booklist
    bookList = BookList();

    //Refresh book list
    ui->treeWidget->clear();

    //**TIMER**//
//    qDebug()<< "main timer, elapsed: " << timer_n1.elapsed() << "begin build booklist from folder";

#ifdef Q_OS_ANDROID
    // Check if BOOKPATH is outside app-specific directories and we need permissions
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!QString(BOOKPATH).startsWith(appDataPath) && !PermissionUtils::canReadExternalStorage()) {
        qDebug() << "No read permission for book path:" << BOOKPATH;
        // Still try to build from folder, but warn user if it fails
        bookList.BuildFromFolder(BOOKPATH);
        if (bookList.empty()) {
            qWarning() << "Could not load books, possibly due to missing storage permissions";
        }
    } else {
        bookList.BuildFromFolder(BOOKPATH);
    }
#else
    bookList.BuildFromFolder(BOOKPATH);
#endif

    //**TIMER**//
//    qDebug()<< "main timer, elapsed: " << timer_n1.elapsed() << "done build from folder";

    if (bookList.empty())
        qDebug()<<"can't find books in: " << BOOKPATH;

    //**TIMER**//
//    qDebug()<< "main timer, elapsed: " << timer_n1.elapsed() << "begin check uids";

    // Check all uids
    bookList.CheckUid();

    //**TIMER**//
//    qDebug()<< "main timer, elapsed: " << timer_n1.elapsed() << "end check uids";

    QSettings settings("Orayta", "SingleUser");

    //Load books settings
    for (Book *book : bookList)
    {
        if (!book || book->getUniqueId() == -1)
            continue;

        settings.beginGroup("Book" + stringify(book->getUniqueId()));
        //default is to show no commentaries
        book->showAlone = settings.value("ShowAlone", true).toBool();
        //int n = settings.value("MixedDisplayes", 0).toInt();
        int n = book->mWeavedSources.size();
        //start from 1, ignore first source which shold always be shown.
        for (int j=1; j<n; j++)
        {
            book->mWeavedSources[j].show = settings.value("Shown" + stringify(j), false).toBool();
        }

        settings.endGroup();
    }

    //**TIMER**//
//    qDebug()<< "main timer, elapsed: " << timer_n1.elapsed() << "4";

    bookList.displayInTree(ui->treeWidget, false);

    //if booklist has changed, reset also the books in search tree
    resetSearchBookTree();

    //**TIMER**//
//    qDebug()<< "main timer, elapsed: " << timer_n1.elapsed() << "done loading books";

}

//IZAR
//when going to 'books in search' page, reset the page
void MobileApp::resetSearchBookTree()
{
    ui->SearchTreeWidget->clear();
    booksInSearch = BookList(bookList);
    booksInSearch.displayInTree(ui->SearchTreeWidget, true, true);
}


void MobileApp::on_openBTN_clicked()
{
    ui->stackedWidget->setCurrentIndex(LIST_PAGE);
}

void MobileApp::on_searchBTN_clicked()
{
    ui->stackedWidget->setCurrentIndex(SEARCH_PAGE);
    // Don't auto-focus - let user select books first, then click input when ready
}



void MobileApp::on_getbooksBTN_clicked()
{
    ui->stackedWidget->setCurrentIndex(GET_BOOKS_PAGE);
}

void MobileApp::on_aboutBTN_clicked()
{
    ui->stackedWidget->setCurrentIndex(ABOUT_PAGE);
}

void MobileApp::on_treeWidget_clicked(const QModelIndex &index)
{
    // Get the item to check if it's a category or book
    QTreeWidgetItem *item = ui->treeWidget->itemFromIndex(index);
    if (!item) return;
    
    Book *b = bookList.findBookByTWI(item);
    if (!b) return;
    
    // For categories, be more lenient with touch detection to allow expansion
    // For books, keep strict checks to prevent accidental opening
    if (!b->IsDir()) {
        // Book - strict checks
        if (touchMoved) return;
        
        qint64 touchDuration = touchMoveTimer.elapsed();
        if (touchDuration < 40) return;
    }
    
    //This is a little hack to prevent double events on some android machines and emulators.
    //If the function is called again in less than 2 ms, the second time is ignored.
    qint64 miliSec = timer.restart();
    if (miliSec < 200) return;

    if (ui->treeWidget->isExpanded(index)) ui->treeWidget->collapse(index);
    else ui->treeWidget->expand(index);
}


void MobileApp::on_openBook_clicked()
{
    if ( ui->treeWidget->currentItem() == 0)
        return;
    Book *b = bookList.findBookByTWI(ui->treeWidget->currentItem());
    if (!b->IsDir()) showBook(b);
}
 
void MobileApp::on_deleteBooksBTN_clicked(bool checked)
{
    if (checked) {
        ui->deleteBookLBL->show();
    } else {
        ui->deleteBookLBL->hide();
    }
}

QSet<QString> MobileApp::getExpandedCategories()
{
    QSet<QString> expandedPaths;
    
    // Iterate through all items in the tree
    QTreeWidgetItemIterator it(ui->treeWidget);
    while (*it) {
        QTreeWidgetItem *item = *it;
        if (item->childCount() > 0 && item->isExpanded()) {
            // Find the book for this item
            Book *book = bookList.findBookByTWI(item);
            if (book && book->IsDir()) {
                // Save the book's path as identifier
                expandedPaths.insert(book->getPath());
            }
        }
        ++it;
    }
    
    return expandedPaths;
}

void MobileApp::restoreExpandedCategories(const QSet<QString> &expandedPaths)
{
    // Iterate through all items in the tree
    QTreeWidgetItemIterator it(ui->treeWidget);
    while (*it) {
        QTreeWidgetItem *item = *it;
        if (item->childCount() > 0) {
            // Find the book for this item
            Book *book = bookList.findBookByTWI(item);
            if (book && book->IsDir()) {
                // Check if this category was expanded
                if (expandedPaths.contains(book->getPath())) {
                    ui->treeWidget->expandItem(item);
                }
            }
        }
        ++it;
    }
}

QMessageBox* MobileApp::createStyledMessageBox(QMessageBox::Icon icon, const QString &title, const QString &text)
{
    QMessageBox *msgBox = new QMessageBox(this);
    msgBox->setIcon(icon);
    msgBox->setWindowTitle(title);
    msgBox->setText(text);
    
    // Apply app styling
    QString styleSheet = "QMessageBox { background-color: rgb(249, 236, 225); }"
                        "QMessageBox QLabel { color: rgb(40, 19, 1); font-size: 14pt; }"
                        "QPushButton { "
                        "  background-color: rgba(249, 211, 176, 30%);"
                        "  border-style: outset;"
                        "  border-radius: 3px;"
                        "  border-width: 1px;"
                        "  border-color: #8F653F;"
                        "  color: rgb(40, 19, 1);"
                        "  min-height: 2em;"
                        "  min-width: 4em;"
                        "  padding: 5px 15px;"
                        "  font-size: 12pt;"
                        "}"
                        "QPushButton:pressed {"
                        "  border-style: inset;"
                        "  background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #dadbde, stop: 1 #f6f7fa);"
                        "}";
    
    msgBox->setStyleSheet(styleSheet);
    
    return msgBox;
}

void MobileApp::removeBookOrCategory(Book *b)
{
    if (!b) return;
    
#ifdef Q_OS_ANDROID
    // Check if we have write permissions before deleting
    if (!PermissionUtils::canWriteExternalStorage()) {
        qDebug() << "No write permission for deleting books, requesting permission...";
        PermissionUtils::requestStoragePermissions([this, b](bool granted) {
            if (granted) {
                qDebug() << "Permission granted, proceeding with deletion";
                removeBookOrCategory(b);  // Retry after permission granted
            } else {
                qWarning() << "Storage permission denied, cannot delete books";
                QMessageBox *msgBox = createStyledMessageBox(
                    QMessageBox::Warning,
                    tr("Permission Required"),
                    tr("Storage permission is required to delete books.")
                );
                msgBox->exec();
                msgBox->deleteLater();
            }
        });
        return;
    }
#endif
    
    QString itemName = b->getNormallDisplayName();
    
    // Check if this book is currently being displayed
    if (displayer && displayer->getCurrentBook() == b) {
        QMessageBox *msgBox = createStyledMessageBox(
            QMessageBox::Warning,
            tr("Cannot Delete"),
            tr("Cannot delete the currently open book. Please close it first.")
        );
        msgBox->exec();
        msgBox->deleteLater();
        return;
    }
    
    // Check if currently displayed book is in this category
    if (b->IsDir() && displayer && displayer->getCurrentBook()) {
        Book *currentBook = displayer->getCurrentBook();
        Book *parent = currentBook->getParent();
        while (parent) {
            if (parent == b) {
                QMessageBox *msgBox = createStyledMessageBox(
                    QMessageBox::Warning,
                    tr("Cannot Delete"),
                    tr("Cannot delete this category because it contains the currently open book. Please close the book first.")
                );
                msgBox->exec();
                msgBox->deleteLater();
                return;
            }
            parent = parent->getParent();
        }
    }
    
    // Save expanded state before reloading
    QSet<QString> expandedCategories = getExpandedCategories();
    
    if (b->IsDir()) {
        // Count books in category
        int bookCount = countBooksInCategory(b);
        
        if (bookCount == 0) {
            // Empty category, just remove the folder
            QString categoryPath = b->getPath();
            if (!categoryPath.isEmpty()) {
                QFile categoryFile(categoryPath);
                if (categoryFile.exists()) {
                    if (categoryFile.remove()) {
                        qDebug() << "Removed empty category:" << categoryPath;
                    } else {
                        qWarning() << "Failed to remove category folder:" << categoryPath;
                        QMessageBox *msgBox = createStyledMessageBox(
                            QMessageBox::Warning,
                            tr("Delete Failed"),
                            tr("Failed to delete category: %1").arg(itemName)
                        );
                        msgBox->exec();
                        msgBox->deleteLater();
                        return;
                    }
                }
            }
        } else {
            // Category has books - ask for confirmation
            QMessageBox *msgBox = createStyledMessageBox(
                QMessageBox::Question,
                tr("Confirm Deletion"),
                tr("Delete category '%1' and all %2 book(s) inside it?\n\nThis cannot be undone.")
                    .arg(itemName).arg(bookCount)
            );
            msgBox->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox->setDefaultButton(QMessageBox::No);
            
            int result = msgBox->exec();
            msgBox->deleteLater();
            
            if (result != QMessageBox::Yes) {
                return;  // User cancelled
            }
            
            // Remove all books in category
            int removedCount = removeBooksInCategory(b);
            
            // Also remove the category folder file
            QString categoryPath = b->getPath();
            if (!categoryPath.isEmpty()) {
                QFile categoryFile(categoryPath);
                if (categoryFile.exists()) {
                    if (categoryFile.remove()) {
                        qDebug() << "Removed category folder:" << categoryPath;
                    } else {
                        qWarning() << "Failed to remove category folder:" << categoryPath;
                    }
                }
            }
            
            qDebug() << "Removed" << removedCount << "book(s) from category:" << itemName;
            
            // Show success message
            QMessageBox *successMsg = createStyledMessageBox(
                QMessageBox::Information,
                tr("Deletion Complete"),
                tr("Deleted %1 book(s) from category '%2'").arg(removedCount).arg(itemName)
            );
            successMsg->exec();
            successMsg->deleteLater();
        }
        
        reloadBooklist();
        restoreExpandedCategories(expandedCategories);
    } else {
        // Single book removal
        QString bookPath = b->getPath();
        
        if (bookPath.isEmpty()) {
            qWarning() << "Could not find book file path for:" << itemName;
            QMessageBox *msgBox = createStyledMessageBox(
                QMessageBox::Warning,
                tr("Delete Failed"),
                tr("Could not find file path for: %1").arg(itemName)
            );
            msgBox->exec();
            msgBox->deleteLater();
            return;
        }
        
        qDebug() << "Attempting to remove book:" << itemName << "at path:" << bookPath;
        
        QFile bookFile(bookPath);
        if (bookFile.exists()) {
            if (bookFile.remove()) {
                qDebug() << "Successfully removed book file:" << bookPath;
                reloadBooklist();
                restoreExpandedCategories(expandedCategories);
            } else {
                qWarning() << "Failed to remove book file:" << bookPath;
                QMessageBox *msgBox = createStyledMessageBox(
                    QMessageBox::Warning,
                    tr("Delete Failed"),
                    tr("Failed to delete book: %1\n\nThe file may be in use or protected.").arg(itemName)
                );
                msgBox->exec();
                msgBox->deleteLater();
            }
        } else {
            qWarning() << "Book file does not exist:" << bookPath;
            // File doesn't exist, just reload to update the list
            reloadBooklist();
            restoreExpandedCategories(expandedCategories);
        }
    }
}

int MobileApp::countBooksInCategory(Book *category)
{
    if (!category || !category->IsDir()) return 0;
    
    int count = 0;
    
    // Iterate through the entire booklist and check parent relationship
    for (Book *book : bookList) {
        if (!book) continue;
        
        // Check if this book's parent is the category (or a subcategory)
        Book *parent = book->getParent();
        while (parent) {
            if (parent == category) {
                if (!book->IsDir()) {
                    count++;
                }
                break;
            }
            parent = parent->getParent();
        }
    }
    
    return count;
}

int MobileApp::removeBooksInCategory(Book *category)
{
    if (!category || !category->IsDir()) return 0;
    
    int removedCount = 0;
    QList<Book*> booksToRemove;
    
    // Iterate through the entire booklist and check parent relationship
    for (Book *book : bookList) {
        if (!book || book->IsDir()) continue;
        
        // Check if this book's parent is the category (or a subcategory)
        Book *parent = book->getParent();
        while (parent) {
            if (parent == category) {
                booksToRemove.append(book);
                break;
            }
            parent = parent->getParent();
        }
    }
    
    // Remove the collected books
    for (Book *book : booksToRemove) {
        QString bookPath = book->getPath();
        if (!bookPath.isEmpty()) {
            QFile bookFile(bookPath);
            if (bookFile.exists() && bookFile.remove()) {
                qDebug() << "Removed book:" << book->getNormallDisplayName() << "at" << bookPath;
                removedCount++;
            } else {
                qWarning() << "Failed to remove book:" << bookPath;
            }
        }
    }
    
    return removedCount;
}


void MobileApp::on_treeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Book *b = bookList.findBookByTWI(item);

    if (!b) return;

    // Check if we're in delete mode
    if (ui->deleteBooksBTN->isChecked()) {
        removeBookOrCategory(b);
        return;
    }

    if (b->IsDir()) {
        // Handle category - expand/collapse
        if (item->isExpanded()) {
            ui->treeWidget->collapseItem(item);
        } else {
            ui->treeWidget->expandItem(item);
        }
    } else {
        // Handle book - open it
        showBook(b);
    }
}

void MobileApp::on_treeWidget_itemClicked(QTreeWidgetItem *item, int column)
{
    // Ignore if user was scrolling/swiping
    if (touchMoved) return;
    
    // Prevent double-click events on some Android devices
    qint64 miliSec = timer.restart();
    if (miliSec < 150) return;
    
    // Delay click processing slightly to allow edge swipe detection
    // This gives time for Android to steal the gesture if it's a back swipe
    QTimer::singleShot(50, this, [this, item, column]() {
        // Double-check touchMoved after delay
        if (!touchMoved) {
            on_treeWidget_itemDoubleClicked(item, column);
        }
    });
}

void MobileApp::showBook(Book *book, BookIter itr)
{
#ifdef Q_OS_ANDROID
    // Check if we need storage permissions to access this book
    if (book && !book->getPath().isEmpty()) {
        // Check if the book path is outside app-specific directories
        QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (!book->getPath().startsWith(appDataPath) && !PermissionUtils::canReadExternalStorage()) {
            qDebug() << "No read permission for book at:" << book->getPath();
            PermissionUtils::showPermissionRationale(PermissionUtils::ReadMediaImages, [this, book, itr](bool userAccepted) {
                if (userAccepted) {
                    PermissionUtils::requestStoragePermissions([this, book, itr](bool granted) {
                        if (granted) {
                            qDebug() << "Permission granted, opening book";
                            performShowBook(book, itr);
                        } else {
                            qWarning() << "Storage permission denied, cannot open book";
                            QMessageBox::warning(this, tr("Permission Required"), 
                                tr("Storage permission is required to access books stored on external storage. "
                                   "Please grant permission to read this book."));
                        }
                    });
                } else {
                    qDebug() << "User declined permission rationale";
                }
            });
            return;
        }
    }
#endif
    
    performShowBook(book, itr);
}

void MobileApp::performShowBook(Book *book, BookIter itr)
{
    //IZAR: temporary work-around. the problem is that orayta reads the global font settings ONLY on startup, and is careless if it is changed latter.
    //TODO: fix this.
    QString fontname= useCustomFontForAll? gFontFamily: DEF_FONT;
    fontname = (book->hasNikud || book->hasTeamim)? gFontFamily: fontname;
    QFont font( fontname, gFontSize );
    book->setFont(font);


    // display mixed selection button only if the book has commentaries
    ui->mixedSelectBTN->setEnabled(book->IsMixed());

    // qml test !!!@@@###%%%



//    QQuickView *viewer = new QQuickView();
//    if (viewer) {
//    viewer->setSource(QUrl("qrc:/testqml.qml"));
//   this->createWindowContainer(viewer);
//        viewer->show();
//    QWidget *container = QWidget::createWindowContainer(viewer, this);
//        container->setMinimumSize(200, 200);
//        container->setMaximumSize(200, 200);
//        container->setFocusPolicy(Qt::TabFocus);
    //QWidget * empty = new QWidget;
//        QLayout* ly = ui->displayArea->layout();
//        ly->removeWidget(displayer);
//        ly->addWidget(container);
//    this->layout()->addWidget(viewer);
//    }


// testing qml !!@@%%
//    QDeclarativeView *qmlView = new QDeclarativeView;
//     qmlView->setSource(QUrl::fromLocalFile("./testqml.qml"));
//    qmlView->setSource(QUrl("qrc:/testqml.qml"));

//     QWidget *widget = myExistingWidget();
  //QVBoxLayout *layout = new QVBoxLayout(this);

          //   layout->addWidget(qmlView);
//     this->layout()->addWidget(qmlView);


    //IZAR TODO: find a way

    switch ( book->fileType() )
    {
        case ( Book::Normal ):
        {
    #ifdef KOOKITA
            initRequest();
            //book is kukayta book but kukayta isn't installed
            if ((! isKukaytaInstalled()) && book->isEncrypted){
                //display install kukayta page
                ui->stackedWidget->setCurrentIndex(KUKAYTA_PAGE);
                ui->kukaytaArea->setCurrentIndex(0);
                return;
            }
    #endif

            ui->stackedWidget->setCurrentIndex(DISPLAY_PAGE);
            qApp->processEvents();

            //book->readBook(1);
            ui->bookNameLBL->setText(book->getNormallDisplayName());
            
            // Set book title font to 35% of the book text size
            QFont bookTitleFont;
            bookTitleFont.setPointSize(gFontSize * 0.35);
            ui->bookNameLBL->setFont(bookTitleFont);
            
            displayer->display(book, itr);


            break;
        }
        case ( Book::Html ):
        {
            ui->stackedWidget->setCurrentIndex(DISPLAY_PAGE);
            qApp->processEvents();
            //QFile f(book->getPath());
            //ui->textBrowser->setHtml( f.readAll() );
            displayer->setSource(QUrl::fromLocalFile(book->getPath()));
            ui->bookNameLBL->setText(book->getName());
            
            // Set book title font to 35% of the book text size
            QFont bookTitleFont;
            bookTitleFont.setPointSize(gFontSize * 0.35);
            ui->bookNameLBL->setFont(bookTitleFont);

            break;
        }
    /*
      //@@@@@@@@
        case ( Book::Pdf ):
        {
            //TODO: Add poppler support?
            wview->page()->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
            wview->setHtml(pluginPage(book->getNormallDisplayName()));
            ui->stackedWidget->setCurrentIndex(DISPLAY_PAGE);
            if ( wview->page()->mainFrame()->evaluateJavaScript("testPdfPlugin()").toString() == "yes" )
            {
                wview->load( QUrl::fromLocalFile( "file:///" + book->getPath() ) );
                //TODO: title
                //ui->viewTab->setTabText(CURRENT_TAB, book->getNormallDisplayName());
            }
            break;

        }*/
        case ( Book::Link ):
        {
            ui->stackedWidget->setCurrentIndex(DISPLAY_PAGE);
            qApp->processEvents();

            //Process link:

            //Read link file
            QList <QString> t;
            ReadFileToList(book->getPath(), t, "UTF-8");

            //Find the id of the book the link points to
            int lId = -1;
            for (int i=0; i<t.size(); i++)
            {
                int p = t[i].indexOf("Link=");
                if (p != -1) ToNum(t[i].mid(p + 5), &lId);
            }

            if (lId != -1) showBook( bookList.findBookById(lId) );
            else qDebug("Invalid link!");

            break;
        }
        case ( Book::Dir ):
        {
            // Directory type - should not be displayed directly
            qDebug() << "Attempted to display directory as book:" << book->getName();
            break;
        }
        case ( Book::Pdf ):
        {
            // PDF support not implemented in mobile version
            qDebug() << "PDF display not supported in mobile version:" << book->getName();
            ui->stackedWidget->setCurrentIndex(DISPLAY_PAGE);
            displayer->setHtml(simpleHtmlPage(book->getName(), tr("PDF files are not supported in the mobile version")));
            break;
        }
        case ( Book::Unkown ):
        {
            // Unknown file type
            qDebug() << "Unknown file type for book:" << book->getName();
            ui->stackedWidget->setCurrentIndex(DISPLAY_PAGE);
            displayer->setHtml(simpleHtmlPage(book->getName(), tr("Unknown file type")));
            break;
        }
    }


}

void MobileApp::showBook(Book *book)
{
    if (!book)
    {
        qDebug() << "Error! Can't open book";
        return;
    }

    showBook(book, BookIter());

}

void MobileApp::showBook(int id)
{
    qDebug() << "open book " << id;
    Book *b= bookList.findBookById(id);
    showBook(b);
}


void MobileApp::tdloadFinished(QUrl u, Book* book, BookIter iter)
{
   QApplication::processEvents();

   titleUpdate(u);
   displayer->setEnabled(true);
   ui->loadBar->hide();

    //IZAR- add a bookmark in new location
   // this seems now to be unnecesary, beacuase we save bookmark when we leave the displayer page.
//   if (book)
//   {
//       addBookMark(book, iter);
//   }
}

void MobileApp::tdloadStarted()
{
    ui->loadBar->show();
    displayer->setEnabled(false);
    QApplication::processEvents();
}

void MobileApp::titleUpdate(QUrl u)
{
//    qDebug() << "titleupdate. url: " << u;
    QString titleTail = displayer->getCurrentIter().humanDisplay();
    QString titleHead, title = "";

    if (displayer && displayer->getCurrentBook()){
        titleHead= displayer->getCurrentBook()->getNormallDisplayName();
        // add separator
        titleHead   += " | ";
    }

    //if titleTail is empty we may be at a search results page. TODO: find a way to really check and not only guess.
    if (u.toString().contains("SEARCH")) { title = tr("Search results"); }
    else {title = titleHead + titleTail;}

    ui->bookNameLBL->setText(title);
    
    // Set book title font to 35% of the book text size
    QFont bookTitleFont;
    bookTitleFont.setPointSize(gFontSize * 0.35);
    ui->bookNameLBL->setFont(bookTitleFont);
}


//Overrides the normal "closeEvent", so it can save tha window's state before quiting
bool MobileApp::event(QEvent *event)
{
    // Intercept close events on Android to handle back button
    if (event->type() == QEvent::Close)
    {
        qDebug() << "QEvent::Close received";
        
        #ifdef Q_OS_ANDROID
        int currentPage = ui->stackedWidget->currentIndex();
        qDebug() << "Current page:" << currentPage;
        
        // If we're not on the main page, navigate back instead of closing
        if (currentPage != MAIN_PAGE)
        {
            qDebug() << "Not on main page, preventing close and navigating back";
            event->ignore();
            goBack();
            return true;  // Event handled
        }
        
        qDebug() << "On main page, allowing close";
        #endif
    }
    
    // Pass event to base class
    return QDialog::event(event);
}

void MobileApp::closeEvent(QCloseEvent *event)
{
    qDebug()<<"close event";

    // On Android, intercept the close event and handle back navigation
    #ifdef Q_OS_ANDROID
    int currentPage = ui->stackedWidget->currentIndex();
    qDebug() << "Close event on Android. Current page:" << currentPage;
    
    // If we're not on the main page, go back instead of closing
    if (currentPage != MAIN_PAGE)
    {
        qDebug() << "Not on main page, ignoring close and calling goBack()";
        event->ignore();  // Don't close the app
        goBack();  // Navigate back
        return;
    }
    
    // If we're on the main page, allow the app to close
    qDebug() << "On main page, allowing close";
    #endif

    //Cancel any running searches
    stopSearchFlag = true;

    saveSettings();

    //Delete the old downloadable-books list
    QFile f(SAVEDBOOKLIST);
    if (f.exists()) f.remove();

    ClearTmp();

    event->accept();  // Allow the close
}

// store all settings from the app
void MobileApp::saveSettings(){

    qDebug()<<"saving global settings...";

    QSettings settings("Orayta", "SingleUser");

    //remmeber last open book
    settings.beginGroup("History");
    settings.setValue("lastPage", ui->stackedWidget->currentIndex());

    if (displayer->getCurrentBook()) settings.setValue("lastBook", displayer->getCurrentBook()->getUniqueId());

    settings.setValue("position", displayer->getCurrentIter().toEncodedString());
    settings.setValue("viewposition", displayer->getVpos());
    settings.endGroup();

    // Save font settings (in case user changed them but didn't click Save button)
    settings.beginGroup("Confs");
    settings.setValue("fontfamily", gFontFamily);
    settings.setValue("fontsize", gFontSize);
    settings.setValue("useCustomFontForAll", useCustomFontForAll);
    settings.setValue("nightMode", nightMode);
    settings.endGroup();
    
    qDebug() << "=== SAVING ON APP CLOSE ===";
    qDebug() << "Saved font size:" << gFontSize;
    
    // Force sync to disk
    settings.sync();
    qDebug() << "Settings synced to disk. Status:" << settings.status();

    /*
    // This takes way too long!

    for (Book *book : bookList)
    {
        if (book->getUniqueId() == -1 || book->hasRandomId)
            continue;

        settings.beginGroup("Book" + stringify(book->getUniqueId()));
        settings.setValue("ShowAlone", book->showAlone);
        for (int j=1; j<book->mWeavedSources.size(); j++)
        {
            settings.setValue("Shown" + stringify(j), book->mWeavedSources[j].show);
        }
        settings.setValue("InSearch", book->IsInSearch());
        settings.endGroup();
    }
    */

    addBookMark(displayer->getCurrentBook(), displayer->getCurrentIter(), displayer->getVpos());
    
#ifdef Q_OS_ANDROID
    // Check if we have write permissions for saving bookmarks
    if (PermissionUtils::canWriteExternalStorage()) {
        ui->staticBookMarkList->saveSettings();
        ui->historyBookmarkList->saveSettings();
    } else {
        qDebug() << "No write permission for saving bookmarks, skipping bookmark save";
        // Note: QSettings should still work for app-specific data
    }
#else
    ui->staticBookMarkList->saveSettings();
    ui->historyBookmarkList->saveSettings();
#endif

    qDebug()<<"done saving settings.";
}

//Remove all temporary html files the program created
void MobileApp::ClearTmp()
{
    QDir dir;
    QStringList list;
    QFile f;

    //remove all html rended files in temp path
    dir = QDir(TMPPATH);
    list = dir.entryList(QStringList("*.html"));
    for (int i=0; i<list.size(); i++)
    {
        f.remove(dir.absoluteFilePath(list[i]));
    }
}


void MobileApp::keyPressEvent(QKeyEvent *keyEvent)
{
    //We use the press event here, so that auto-repeat works
    switch ( keyEvent->key() )
    {
        //case Qt::Key_U:
        case Qt::Key_VolumeUp:
            displayer->verticalScrollBar()->setValue(displayer->verticalScrollBar()->value() - 60);
            break;
        //case Qt::Key_D:
        case Qt::Key_VolumeDown:
            displayer->verticalScrollBar()->setValue(displayer->verticalScrollBar()->value() + 60);
            break;
    }
}


void MobileApp::keyReleaseEvent(QKeyEvent *keyEvent){

    switch ( keyEvent->key() )
    {

    // onPause event from android
    case Qt::Key_MediaTogglePlayPause:
    case Qt::Key_MediaPlay:
        saveSettings();
        break;

    /*
    //stop event sent from android. exit app
    case Qt::Key_MediaStop:
        qDebug()<< "android stop request";

        // if autoResume selected by user, dont terminate app.
        if (!autoResume)  close();
        break;
    */

    //back button was clicked
    case Qt::Key_Close:
    case Qt::Key_MediaPrevious:
    case Qt::Key_Back:
        goBack();
        break;

    //ctrl + backspace clicked , go back.
    case Qt::Key_Backspace:
        if (keyEvent->modifiers() == Qt::CTRL) goBack();
        break;

        return;


    //Test different screen sizes:
    case Qt::Key_0:
        resize(QSize(240,300));
        adjustToScreenSize();
        break;
    case Qt::Key_1:
        resize(QSize(240,380));
        adjustToScreenSize();
        break;
    case Qt::Key_2:
        resize(QSize(240,412));
        adjustToScreenSize();
        break;
    case Qt::Key_3:
        resize(QSize(320,460));
        adjustToScreenSize();
        break;
    case Qt::Key_4:
        resize(QSize(480,780));
        adjustToScreenSize();
        break;
    case Qt::Key_5:
        resize(QSize(480,854));
        adjustToScreenSize();
        break;

    //menu button was clicked
    case Qt::Key_TopMenu:
    case Qt::Key_Menu:
    case Qt::Key_Explorer:
    case Qt::Key_Meta:
    case Qt::Key_Super_L:
        showMenu();
        break;

    default:
        qDebug() << "unknown key pressed: " << keyEvent->key();
        QDialog::keyReleaseEvent(keyEvent);
    }

    keyEvent->accept();
}

bool MobileApp::eventFilter(QObject *obj, QEvent *event)
{
    // Handle search input focus - removed cursor manipulation that was causing keyboard issues
    
    // Reverse horizontal scrolling for download list (RTL behavior)
    if ((obj == ui->downloadListWidget || obj == ui->downloadListWidget->viewport()) && 
        event->type() == QEvent::Wheel) {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
        // Check if it's horizontal scrolling
        if (qAbs(wheelEvent->angleDelta().x()) > qAbs(wheelEvent->angleDelta().y())) {
            // Reverse the horizontal scroll direction
            QWheelEvent reversedEvent(
                wheelEvent->position(),
                wheelEvent->globalPosition(),
                wheelEvent->pixelDelta(),
                QPoint(-wheelEvent->angleDelta().x(), wheelEvent->angleDelta().y()), // Reverse X
                wheelEvent->buttons(),
                wheelEvent->modifiers(),
                wheelEvent->phase(),
                wheelEvent->inverted(),
                wheelEvent->source()
            );
            QApplication::sendEvent(obj, &reversedEvent);
            return true; // Event handled
        }
    }
    
    // Handle swipe gestures for tab navigation
    bool isBookmarkWidget = (obj == ui->staticBookMarkList || obj == ui->staticBookMarkList->viewport() ||
                             obj == ui->dailyLearningList || obj == ui->dailyLearningList->viewport() ||
                             obj == ui->historyBookmarkList || obj == ui->historyBookmarkList->viewport());
    
    // Check if obj is one of the settings tab pages
    bool isSettingsWidget = false;
    for (int i = 0; i < ui->tabWidget->count(); ++i) {
        if (obj == ui->tabWidget->widget(i)) {
            isSettingsWidget = true;
            break;
        }
    }
    
    if (isBookmarkWidget || isSettingsWidget) {
        // Determine which tab widget to control
        QTabWidget *targetTabWidget = isSettingsWidget ? ui->tabWidget : ui->tabWidget_2;
        
        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::TouchBegin) {
            if (event->type() == QEvent::MouseButtonPress) {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
                tabSwipeStartPos = mouseEvent->globalPosition().toPoint();
            } else {
                QTouchEvent *touchEvent = static_cast<QTouchEvent*>(event);
                if (!touchEvent->points().isEmpty()) {
                    tabSwipeStartPos = touchEvent->points().first().globalPosition().toPoint();
                }
            }
            tabSwipeInProgress = true;
        }

        else if (event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::TouchEnd) {
            if (tabSwipeInProgress) {
                QPoint endPos;
                if (event->type() == QEvent::MouseButtonRelease) {
                    QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
                    endPos = mouseEvent->globalPosition().toPoint();
                } else {
                    QTouchEvent *touchEvent = static_cast<QTouchEvent*>(event);
                    if (!touchEvent->points().isEmpty()) {
                        endPos = touchEvent->points().first().globalPosition().toPoint();
                    }
                }
                
                int dx = endPos.x() - tabSwipeStartPos.x();
                int dy = endPos.y() - tabSwipeStartPos.y();
                
                // Horizontal swipe: 2x more horizontal than vertical, 50px minimum
                if (qAbs(dx) > qAbs(dy) * 2.0 && qAbs(dx) > 50) {
                    int currentIndex = targetTabWidget->currentIndex();
                    int tabCount = targetTabWidget->count();
                    
                    // For RTL app: Swipe right = next tab (dx > 0), Swipe left = previous tab (dx < 0)
                    if (dx > 0 && currentIndex < tabCount - 1) {
                        targetTabWidget->setCurrentIndex(currentIndex + 1);
                    } else if (dx < 0 && currentIndex > 0) {
                        targetTabWidget->setCurrentIndex(currentIndex - 1);
                    }
                }
                
                tabSwipeInProgress = false;
            }
        }
    }
    
    // Handle smart scrolling for tree widget - prevent accidental horizontal scrolling
    if (obj == ui->treeWidget->viewport()) {
        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::TouchBegin) {
            if (event->type() == QEvent::MouseButtonPress) {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
                treeScrollStartPos = mouseEvent->globalPosition().toPoint();
            } else {
                QTouchEvent *touchEvent = static_cast<QTouchEvent*>(event);
                if (!touchEvent->points().isEmpty()) {
                    treeScrollStartPos = touchEvent->points().first().globalPosition().toPoint();
                }
            }
            isVerticalScrolling = false;
        }
        else if (event->type() == QEvent::MouseMove || event->type() == QEvent::TouchUpdate) {
            QPoint currentPos;
            if (event->type() == QEvent::MouseMove) {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
                currentPos = mouseEvent->globalPosition().toPoint();
            } else {
                QTouchEvent *touchEvent = static_cast<QTouchEvent*>(event);
                if (!touchEvent->points().isEmpty()) {
                    currentPos = touchEvent->points().first().globalPosition().toPoint();
                }
            }
            
            int dx = qAbs(currentPos.x() - treeScrollStartPos.x());
            int dy = qAbs(currentPos.y() - treeScrollStartPos.y());
            
            // Determine scroll direction after minimum movement
            if (!isVerticalScrolling && (dx > 10 || dy > 10)) {
                isVerticalScrolling = (dy > dx * 1.5); // Prefer vertical scrolling
                
                // If it's clearly horizontal scrolling and there's no horizontal content overflow,
                // block the horizontal scroll
                if (!isVerticalScrolling) {
                    QScrollBar *hScrollBar = ui->treeWidget->horizontalScrollBar();
                    if (hScrollBar->maximum() == 0) {
                        // No horizontal content to scroll, block horizontal movement
                        return true;
                    }
                }
            }
            
            // Block horizontal scrolling if we determined this is vertical scrolling
            if (isVerticalScrolling && dx > dy) {
                return true;
            }
        }
        else if (event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::TouchEnd) {
            isVerticalScrolling = false;
        }
    }
    
    // Handle smart scrolling for selection area - prevent accidental clicks after scrolling
    if (obj == ui->selectionArea || obj == ui->selectionArea->viewport()) {
        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::TouchBegin) {
            if (event->type() == QEvent::MouseButtonPress) {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
                selectionScrollStartPos = mouseEvent->globalPosition().toPoint();
            } else {
                QTouchEvent *touchEvent = static_cast<QTouchEvent*>(event);
                if (!touchEvent->points().isEmpty()) {
                    selectionScrollStartPos = touchEvent->points().first().globalPosition().toPoint();
                }
            }
            selectionAreaScrolling = false;
        }
        else if (event->type() == QEvent::MouseMove || event->type() == QEvent::TouchUpdate) {
            QPoint currentPos;
            if (event->type() == QEvent::MouseMove) {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
                currentPos = mouseEvent->globalPosition().toPoint();
            } else {
                QTouchEvent *touchEvent = static_cast<QTouchEvent*>(event);
                if (!touchEvent->points().isEmpty()) {
                    currentPos = touchEvent->points().first().globalPosition().toPoint();
                }
            }
            
            int dx = qAbs(currentPos.x() - selectionScrollStartPos.x());
            int dy = qAbs(currentPos.y() - selectionScrollStartPos.y());
            
            // If there's significant movement, mark as scrolling
            if (dx > 8 || dy > 8) {
                selectionAreaScrolling = true;
            }
        }
        else if (event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::TouchEnd) {
            // If we were scrolling, block the click event briefly
            if (selectionAreaScrolling) {
                selectionAreaScrolling = false;
                // Block click events for a short time after scrolling
                QTimer::singleShot(150, this, [this]() {
                    selectionAreaScrolling = false;
                });
                return true; // Block the click
            }
            selectionAreaScrolling = false;
        }
    }
    
    // Handle smart scrolling for text displayer - prevent accidental chapter clicks after scrolling
    if (obj == displayer || obj == displayer->viewport()) {
        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::TouchBegin) {
            if (event->type() == QEvent::MouseButtonPress) {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
                textScrollStartPos = mouseEvent->globalPosition().toPoint();
            } else {
                QTouchEvent *touchEvent = static_cast<QTouchEvent*>(event);
                if (!touchEvent->points().isEmpty()) {
                    textScrollStartPos = touchEvent->points().first().globalPosition().toPoint();
                }
            }
            textDisplayerScrolling = false;
        }
        else if (event->type() == QEvent::MouseMove || event->type() == QEvent::TouchUpdate) {
            QPoint currentPos;
            if (event->type() == QEvent::MouseMove) {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
                currentPos = mouseEvent->globalPosition().toPoint();
            } else {
                QTouchEvent *touchEvent = static_cast<QTouchEvent*>(event);
                if (!touchEvent->points().isEmpty()) {
                    currentPos = touchEvent->points().first().globalPosition().toPoint();
                }
            }
            
            int dx = qAbs(currentPos.x() - textScrollStartPos.x());
            int dy = qAbs(currentPos.y() - textScrollStartPos.y());
            
            // If there's significant movement, mark as scrolling
            // Use higher threshold to avoid blocking normal clicks
            if (dx > 50 || dy > 50) {
                textDisplayerScrolling = true;
                // qDebug() << "Text displayer scrolling detected, dx:" << dx << "dy:" << dy;
            }
        }
        else if (event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::TouchEnd) {
            // If we were scrolling, block the click event
            if (textDisplayerScrolling) {
                textDisplayerScrolling = false;
                // qDebug() << "Blocked click due to scrolling in text displayer";
                return true; // Block the event
            }
            textDisplayerScrolling = false;
        }
    }
    
    // GLOBAL touch tracking for edge swipe detection (Android back gesture)
    // This applies to all widgets to prevent accidental clicks during back gestures
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::TouchBegin) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            lastPressPos = mouseEvent->globalPosition().toPoint();
        } else {
            QTouchEvent *touchEvent = static_cast<QTouchEvent*>(event);
            if (!touchEvent->points().isEmpty()) {
                lastPressPos = touchEvent->points().first().globalPosition().toPoint();
            }
        }
        
        // Special handling for search input - reset touchMoved and focus
        if (obj == ui->searchInBooksLine) {
            touchMoved = false;
            ui->searchInBooksLine->setFocus(Qt::MouseFocusReason);
            
            #ifdef Q_OS_ANDROID
            QTimer::singleShot(200, this, [this]() {
                if (ui->searchInBooksLine->hasFocus()) {
                    QGuiApplication::inputMethod()->show();
                }
            });
            QTimer::singleShot(400, this, [this]() {
                if (ui->searchInBooksLine->hasFocus()) {
                    QGuiApplication::inputMethod()->show();
                }
            });
            #endif
            return QDialog::eventFilter(obj, event);
        }
        
        touchMoved = false;
        touchMoveTimer.start();
    }
    else if (event->type() == QEvent::MouseMove || event->type() == QEvent::TouchUpdate) {
        QPoint currentPos;
        if (event->type() == QEvent::MouseMove) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            currentPos = mouseEvent->globalPosition().toPoint();
        } else {
            QTouchEvent *touchEvent = static_cast<QTouchEvent*>(event);
            if (!touchEvent->points().isEmpty()) {
                currentPos = touchEvent->points().first().globalPosition().toPoint();
            }
        }
        
        int dx = currentPos.x() - lastPressPos.x(); // Signed for direction
        int dy = currentPos.y() - lastPressPos.y();
        
        // Detect edge swipes (Android back gesture)
        // Back gesture = swipe inward from edge (left edge: swipe right, right edge: swipe left)
        QScreen* screen = QApplication::primaryScreen();
        int screenWidth = screen->availableGeometry().width();
        int edgeThreshold = qMax(20, qMin(40, (int)(screenWidth * 0.03))); // 3% of width
        
        bool startedAtLeftEdge = (lastPressPos.x() < edgeThreshold);
        bool startedAtRightEdge = (lastPressPos.x() > screenWidth - edgeThreshold);
        bool swipingInwardFromLeft = startedAtLeftEdge && dx > 15; // Swiping right from left edge
        bool swipingInwardFromRight = startedAtRightEdge && dx < -15; // Swiping left from right edge
        bool isEdgeSwipe = swipingInwardFromLeft || swipingInwardFromRight;
        
        if (qAbs(dx) > 8 || qAbs(dy) > 8 || isEdgeSwipe) {
            touchMoved = true;
        }
    }
    else if (event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::TouchEnd) {
        // Final check on release: if started at edge, mark as moved to be safe
        QScreen* screen = QApplication::primaryScreen();
        int screenWidth = screen->availableGeometry().width();
        int edgeThreshold = qMax(20, qMin(40, (int)(screenWidth * 0.03)));
        bool startedAtEdge = (lastPressPos.x() < edgeThreshold || lastPressPos.x() > screenWidth - edgeThreshold);
        
        if (startedAtEdge) {
            touchMoved = true; // Mark as moved if started at edge, to be safe
        }
        
        if (touchMoved) {
            QTimer::singleShot(500, this, [this]() { 
                touchMoved = false;
            });
        } else {
            QTimer::singleShot(100, this, [this]() { 
                touchMoved = false;
            });
        }
    }
    
    return QDialog::eventFilter(obj, event);
}

void MobileApp::mousePressEvent(QMouseEvent *ev)
{
    lastPressPos = ev->globalPosition().toPoint();  // Use global coordinates
    touchMoved = false;
    touchMoveTimer.start();  // Start timer to track when touch started
    QDialog::mousePressEvent(ev);
}

void MobileApp::mouseMoveEvent(QMouseEvent *ev)
{
    // Check if finger moved more than 8 pixels in any direction
    // Lower threshold to catch even small movements
    QPoint currentPos = ev->globalPosition().toPoint();
    int dx = qAbs(currentPos.x() - lastPressPos.x());
    int dy = qAbs(currentPos.y() - lastPressPos.y());
    
    if (dx > 8 || dy > 8) {
        touchMoved = true;
    }
    QDialog::mouseMoveEvent(ev);
}

void MobileApp::mouseReleaseEvent(QMouseEvent *ev){
    if (ev->button() == Qt::RightButton) showMenu();

    QDialog::mouseReleaseEvent(ev);
}

// when menu butten clicked, show the menu
void MobileApp::showMenu()
{
//    qDebug() << "show menu";
    switch (ui->stackedWidget->currentIndex())
    {
    case DISPLAY_PAGE:
        //show/hide the toolbar menu
        if (ui->dispalyMenu->isVisible())
            ui->dispalyMenu->hide();
        else
        {
            QToolButton * idxBtn = ui->toIndexMenuBTN;
            if (displayer->isLastSearch())
            {
                idxBtn->setText(tr("Back to results"));
                //shrink font to fit button
                idxBtn->setFont(QFont(idxBtn->font().family(), 7));
            }
            else
            {
                idxBtn->setText(tr("Index"));
                idxBtn->setFont(QFont(idxBtn->font().family(), 9));
            }

            ui->dispalyMenu->show();
        }
        return;

    case MAIN_PAGE:
        //go to the main setting page
        ui->stackedWidget->setCurrentIndex(SETTINGS_PAGE);
        return;

    //TODO: set more options
    default:
        ui->stackedWidget->setCurrentIndex(SETTINGS_PAGE);
        return;
    }
}

// stacked widget currnet view canged.
void MobileApp::viewChanged(int index)
{
    //IZAR
    // this is a list of things to do when we go to a certain page
    
    switch (index){
    //when going to books in search page, reset the page

    //when going to the book list page, turn off delete mode
    case (LIST_PAGE):
        if (ui->deleteBooksBTN->isChecked()) {
            ui->deleteBooksBTN->setChecked(false);
            ui->deleteBookLBL->hide();
        }
        break;

    case (SEARCH_PAGE):
        break;

    //when going to get books page get the list from server.
    case (GET_BOOKS_PAGE):
        downloadBookList();
        break;

    case (SETTINGS_PAGE):
        resetSettingsPage();
        //don't put this page in history
//        return;
        break;

    //when going to commentary selection page, reset it.
    case (MIXED_SELECTION_PAGE):
        setupMixedSelection();
        //don't put this page in history
//        return;
        break;
    }

    if(!viewHistory)
    {
        qDebug()<< "cant stat view history";
        return;
    }
    

    //add this page to history
    if (viewHistory->size() == 0) viewHistory->append(index);
    else
    {
        int previousPage = viewHistory->at(viewHistory->size()-1);

        // if we came from the displayer page then we want to save the scrolled location
        if (previousPage == DISPLAY_PAGE)
        {
            Book* b= displayer->getCurrentBook();
            BookIter itr = displayer->getCurrentIter();
            if (b)
            {
                int viewPosition = displayer->getVpos();
                addBookMark(b, itr, viewPosition);
            }
        }

        // never add the same page twice
        if (previousPage != index)
            // ignore pages we want to exclude
            if (!exclude.contains(index))
                viewHistory->append(index);
    }
}

//go to previous view of stacked widget.
void MobileApp::goBack()
{
    int currentPage = ui->stackedWidget->currentIndex();
    qDebug() << "Back pressed. Current page:" << currentPage;
    
    // If we're on the main page, exit the app
    if (currentPage == MAIN_PAGE)
    {
        qDebug()<< "On main page. Exiting!";
        close();
        return;
    }
    
    // For simple pages (Search, List, Get Books, About, History/Bookmarks, Settings), 
    // always go back to main page
    if (currentPage == SEARCH_PAGE || currentPage == LIST_PAGE || 
        currentPage == GET_BOOKS_PAGE || currentPage == ABOUT_PAGE || 
        currentPage == HISTORY_PAGE || currentPage == SETTINGS_PAGE)
    {
        qDebug() << "On simple page. Going to main page.";
        ui->stackedWidget->setCurrentIndex(MAIN_PAGE);
        return;
    }

    // if we have only one object it probably is the current view and we can only go back to the main page
    if(viewHistory->size() <= 1)
    {
        ui->stackedWidget->setCurrentIndex(MAIN_PAGE);
        return;
    }

    int currentId = ui->stackedWidget->currentIndex();

    if (currentId == DISPLAY_PAGE)
    {
        // if the menu is displayed then hide it
        if (ui->dispalyMenu->isVisible())
        {
            ui->dispalyMenu->hide();
            return;
        }

        //If we are at the index or the search page
        if (displayer->source().path().indexOf("Index") != -1 || displayer->source().path().indexOf("SEARCH") != -1)
        {
            //do nothing here. we'll get to the next statement and execute there.
//            int id = viewHistory->at(viewHistory->size()-2);
//            viewHistory->removeLast();
//            ui->stackedWidget->setCurrentIndex(id);
        }
        else
        {
            if (displayer->isLastSearch())
            {
                displayer->backward();
            }
            else
            {
                //save bookmark at previous location
                if(displayer->getCurrentBook())
                {
                    addBookMark(displayer->getCurrentBook(), displayer->getCurrentIter(), displayer->getVpos());
                }

                displayer->goToIndex();
            }
            return ;
        }
    }
//    else
    {

            int id = viewHistory->at(viewHistory->size()-1);
            // if we are at a excluded page, this means that it isn't in wiewHistory, in which case we should go to the last item in the list
            if (id == currentId)
            {
                if (viewHistory->size() > 0) viewHistory->removeLast();

                // no where to go back to, go to main page instead of exiting
                if (viewHistory->size() <= 0) { 
                    qDebug()<< "Nowhere to go. Going to main page."; 
                    ui->stackedWidget->setCurrentIndex(MAIN_PAGE); 
                    return;
                }

                id = viewHistory->at(viewHistory->size()-1);
            }
            if (viewHistory->size() > 0) viewHistory->removeLast();
            ui->stackedWidget->setCurrentIndex(id);

            return;
    }

    //If we got 'till here then go to main page instead of exiting
    qDebug()<< "Nowhere to go. Going to main page.";
    ui->stackedWidget->setCurrentIndex(MAIN_PAGE);
}

// Handle Android back button - called from Java
bool MobileApp::handleAndroidBackButton()
{
    int currentPage = ui->stackedWidget->currentIndex();
    qDebug() << "Android back button pressed. Current page:" << currentPage;
    
    // If we're on the main page, return false to allow default behavior (minimize app)
    if (currentPage == MAIN_PAGE)
    {
        qDebug() << "On main page, allowing default back behavior";
        return false;
    }
    
    // If we're reading a book (DISPLAY_PAGE)
    if (currentPage == DISPLAY_PAGE)
    {
        // First press: show menu if hidden
        if (!ui->dispalyMenu->isVisible())
        {
            qDebug() << "First press: showing menu";
            showMenu();
            return true;
        }
        
        // Second press: menu is visible, hide it and go back to previous page
        qDebug() << "Second press: menu visible, hiding menu and going back";
        
        // Hide the menu first
        ui->dispalyMenu->hide();
        
        // Save bookmark before leaving
        if (displayer->getCurrentBook())
        {
            addBookMark(displayer->getCurrentBook(), displayer->getCurrentIter(), displayer->getVpos());
        }
        
        // Go back to previous page (book list)
        ui->stackedWidget->setCurrentIndex(LIST_PAGE);
        return true;
    }
    
    // For other pages, use the standard goBack logic
    qDebug() << "Not on main page, handling back navigation";
    goBack();
    return true;
}

#ifdef Q_OS_ANDROID
// JNI function called from Java
extern "C" JNIEXPORT jboolean JNICALL
Java_org_Orayta_OraytaActivity_handleBackButton(JNIEnv *env, jobject obj);
#endif

void MobileApp::on_toIndexMenuBTN_clicked()
{
    if (displayer->isLastSearch())
    {
        displayer->backward();
    }
    else
    {
        displayer->goToIndex();
    }

    //ui->textBrowser->scrollToAnchor("Top");
    //wview->page()->mainFrame()->scrollToAnchor("Top");
}


void MobileApp::on_saveConf_clicked()
{
    //Save font
    gFontFamily = ui->fontComboBox->currentFont().family();
    gFontSize = ui->fonSizeSpinBox->value();

    useCustomFontForAll = ui->customFontRDBTN->isChecked();
    nightMode = ui->NightModeCKBX->isChecked();
    
    qDebug() << "=== SAVING SETTINGS ===";
    qDebug() << "Font family:" << gFontFamily;
    qDebug() << "Font size:" << gFontSize;
    qDebug() << "Use custom font:" << useCustomFontForAll;
    
    // Update font preview to reflect saved settings
    updateFontPreview();

    ui->saveConf->setEnabled(false);

    //Change language if needed
    QSettings settings("Orayta", "SingleUser");
    settings.beginGroup("Confs");

    //save current font settings
    settings.setValue("fontfamily", gFontFamily);
    settings.setValue("fontsize", gFontSize);
    
    qDebug() << "Settings file:" << settings.fileName();
    qDebug() << "Saved fontsize to settings:" << gFontSize;
    
    // Force sync to disk immediately
    settings.sync();
    
    // Verify the save worked
    QVariant savedValue = settings.value("fontsize");
    qDebug() << "Verified saved fontsize:" << savedValue.toInt();
    if (settings.status() != QSettings::NoError) {
        qWarning() << "QSettings error status:" << settings.status();
    }

    settings.setValue("useCustomFontForAll", useCustomFontForAll);
    settings.setValue("nightMode", nightMode);

    settings.setValue("autoInterfaceSize", ui->autoInterfaceSizeCKBX->isChecked());
    settings.setValue("interfaceSize", ui->interfaceSizeSpinBox->value());

    int toolbarStyle = ui->toolbarStyleComboBox->currentIndex();
    settings.setValue("toolbarStyle", toolbarStyle);
    applyToolbarStyle(toolbarStyle);

    /* disabled
    //Change language if needed
    settings.setValue("systemLang",ui->systemLangCbox->isChecked());
    if (ui->systemLangCbox->isChecked())
    {
        LANG = QLocale::languageToString(QLocale::system().language());
    }
    //Use custom language only if "useSystemLang" is not checked
    else
    */

    {
        int i = langsDisplay.indexOf(ui->langComboBox->currentText());
        if (i != -1)
        {
            settings.setValue("lang", langs[i]);
//            settings.endGroup();
            LANG = langs[i];
        }
    }

    settings.endGroup();
    
    // Final sync to ensure everything is written to disk
    settings.sync();
    qDebug() << "=== SAVE COMPLETE ===";
    qDebug() << "Final settings status:" << settings.status();

//    emit ChangeLang(LANG);
    translate(LANG);

    adjustFontSize();

    //also, clear currently displayed book.

    // test if the previous view was the book itself. if so we want to reload the book.
    if (viewHistory->length() > 1 &&
                      viewHistory->at(viewHistory->length()-1) == DISPLAY_PAGE)
    {
        if (displayer->getCurrentBook()) {
            // remove two last itmes from history. (settings page and dispaly page).
            //viewHistory->removeLast(); viewHistory->removeLast();

            //reload previously shown book
            displayer->getCurrentBook()->setFont(QFont(gFontFamily,gFontSize));
            showBook(displayer->getCurrentBook(), displayer->getCurrentIter());

        }

    }
    else goBack();
}

void MobileApp::on_fontComboBox_currentIndexChanged(int index)
{
    //Settings have changed, so the save button should be enabled
    ui->saveConf->setEnabled(true);

    //Apply preview immediately
    applySettingsPreview();
}

void MobileApp::on_horizontalSlider_sliderMoved(int position)
{
    //Apply preview immediately
    applySettingsPreview();
}


void MobileApp::on_fonSizeSpinBox_valueChanged(int size)
{
    //Settings have changed, so the save button should be enabled
    ui->saveConf->setEnabled(true);

    //set the slider to the same value
    ui->horizontalSlider->setValue(size);

    //Apply preview immediately
    applySettingsPreview();
}

void MobileApp::on_interfaceSizeSpinBox_valueChanged(int size)
{
    //Settings have changed, so the save button should be enabled
    ui->saveConf->setEnabled(true);

    //Apply preview immediately
    applySettingsPreview();
}

void MobileApp::on_autoInterfaceSizeCKBX_clicked(bool checked)
{
    //Settings have changed, so the save button should be enabled
    ui->saveConf->setEnabled(true);
    
    // Enable/disable the spinbox based on checkbox
    ui->interfaceSizeSpinBox->setEnabled(!checked);
    
    if (checked) {
        // When auto is enabled, show the calculated value in the spinbox (read-only)
        ui->interfaceSizeSpinBox->setValue(getAutoFontSize());
    }
    
    //Apply preview immediately
    applySettingsPreview();
}

void MobileApp::on_toolbarStyleComboBox_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    //Settings have changed, so the save button should be enabled
    ui->saveConf->setEnabled(true);
    
    //Apply preview immediately
    applySettingsPreview();
}

void MobileApp::applyToolbarStyle(int style)
{
    Qt::ToolButtonStyle buttonStyle;
    
    switch(style)
    {
        case 0: // Text and Icons
            buttonStyle = Qt::ToolButtonTextUnderIcon;
            break;
        case 1: // Text Only
            buttonStyle = Qt::ToolButtonTextOnly;
            break;
        case 2: // Icons Only
        default:
            buttonStyle = Qt::ToolButtonIconOnly;
            break;
    }
    
    // Apply style to all QToolButton widgets with icons
    QList<QToolButton*> toolButtons = {
        // Main page buttons (QToolButton)
        ui->openBTN,
        ui->bookMarksBTN,
        ui->searchBTN,
        ui->getbooksBTN,
        // Display menu buttons (hamburger menu)
        ui->forwardBTN,
        ui->toIndexMenuBTN,
        ui->backBTN,
        ui->ZoomOutBTN,
        ui->toMainMenuBTN,
        ui->ZoomInBTN,
        ui->copyTextBTN,
        ui->mixedSelectBTN,
        ui->addBM_BTN,
        // List page buttons
        ui->resetBookListBTN,
        ui->findBookBTN,
        ui->lastBookBTN
        // Note: clearSearchBTN is excluded - it always shows icon only
    };
    
    for (QToolButton* btn : toolButtons) {
        btn->setToolButtonStyle(buttonStyle);
    }
    
    // Always keep clearSearchBTN as icon-only
    ui->clearSearchBTN->setToolButtonStyle(Qt::ToolButtonIconOnly);
    
    // For QPushButton widgets, we need to handle text/icon visibility differently
    // since QPushButton doesn't have setToolButtonStyle
    QList<QPushButton*> pushButtons = {
        ui->settingsBTN,
        ui->helpBTN,
        ui->aboutBTN
    };
    
    for (QPushButton* btn : pushButtons) {
        switch(style) {
            case 0: // Text and Icons
                btn->setText(btn->property("originalText").toString());
                // Icons are already set, just ensure text is visible
                break;
            case 1: // Text Only
                // Save original text if not already saved
                if (!btn->property("originalText").isValid()) {
                    btn->setProperty("originalText", btn->text());
                }
                btn->setIcon(QIcon()); // Remove icon
                break;
            case 2: // Icons Only
                // Save original text if not already saved
                if (!btn->property("originalText").isValid()) {
                    btn->setProperty("originalText", btn->text());
                }
                btn->setText(""); // Remove text
                // Icons are already set
                break;
        }
    }
}

void MobileApp::on_horizontalSlider_valueChanged(int value)
{
    //set this value to the SpinBox
    ui->fonSizeSpinBox->setValue(value);
    
    //Show the new font in the preview box
    updateFontPreview();
    
    MobileApp::on_fonSizeSpinBox_valueChanged(value);
}



void MobileApp::on_cancelBTN_clicked()
{
    // Restore original settings before leaving
    restoreOriginalSettings();
    
    goBack();
    resetSettingsPage();
}

//perform search in books
void MobileApp::on_SearchInBooksBTN_released()
{

        //Do search
        QString otxt = ui->searchInBooksLine->text();

        //do nothing if no search text.
        if (otxt.isEmpty()) return;

        QString stxt = otxt;
        QRegularExpression regexp;

        /*
        if (ui->fuzzyCheckBox->isChecked())
            stxt = AllowKtivHasser(stxt);
        */

        regexp = QRegularExpression( createSearchPattern (stxt) );

        //show the stop button and search bar
        showHideSearch(true);

        QApplication::processEvents();

        QUrl u = SearchInBooks (regexp, otxt, booksInSearch.BooksInSearch(), ui->progressBar);
        displayer->setSource(u);

        //wview->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
        ui->stackedWidget->setCurrentIndex(DISPLAY_PAGE);

        //done search, reset the ui
        showHideSearch(false);

}

void MobileApp::on_searchInBooksLine_returnPressed() { on_SearchInBooksBTN_released(); }



//Cancel search
void MobileApp::on_stopSearchBTN_clicked()
{
    stopSearchFlag = true;
    showHideSearch(false);
}

// switch the view from normal to in search mode
void MobileApp::showHideSearch(bool inSearch){

    ui->inSearchGroup->setVisible(inSearch);
    ui->SearchInBooksBTN->setVisible(!inSearch);
}

void MobileApp::on_settingsMenuBTN_clicked()
{
    //Copy all
}

void MobileApp::on_SearchTreeWidget_itemClicked(QTreeWidgetItem *item, int column)
{
    // Ignore if user was scrolling/swiping
    if (touchMoved) return;
    
    // Prevent double-click events on some Android devices
    qint64 miliSec = timer.restart();
    if (miliSec < 150) return;

    Book* book = booksInSearch.findBookByTWI(item);
    if (!book) return;

    // Toggle selection state
    if (book->IsInSearch()) {
        book->unselect();
    } else {
        book->select();
    }
}


//---------------------------------------
//the folowing methods where copied from Settings.cpp, but modified.
//TODO: they shold be implemented in a separate class.

/* disabled because system lang doesnt work on android.
  //maybe we should report a bug and fix this when it is fixed.
void MobileApp::on_systemLangCbox_clicked(bool checked)
{
    // Disable language choosing if the "use system language" is selected
    ui->groupBox->setEnabled(!checked);

    //Settings have changed, so the save button should be enabled
    ui->saveConf->setEnabled(true);
}
*/

void MobileApp::setupSettings(){

    QSettings settings("Orayta", "SingleUser");
    settings.beginGroup("Confs");
        useCustomFontForAll = settings.value("useCustomFontForAll", false).toBool();
        // nightMode is loaded during Android initialization (with auto-detection on first run)
    settings.endGroup();

    //Set available languages
    langs << "Hebrew" << "English" << "French";
    langsDisplay << tr("עברית") << "English" << tr("Français");

    //Show available languages in the language combobox
    for (int i=0; i<langs.size(); i++)
    {
        ui->langComboBox->addItem(langsDisplay[i]);
    }
    
#ifdef Q_OS_ANDROID
    // On Android, hide the combobox and add simple buttons instead to avoid Qt 6.10 crash
    ui->langComboBox->hide();
    
    // Create a button group for language selection
    QWidget* langButtonWidget = new QWidget();
    QVBoxLayout* langLayout = new QVBoxLayout(langButtonWidget);
    langLayout->addWidget(new QLabel("בחר שפה / Select Language:"));
    
    QButtonGroup* langButtonGroup = new QButtonGroup(this);
    
    for (int i = 0; i < langs.size(); i++) {
        QPushButton* langBtn = new QPushButton(langsDisplay[i]);
        langBtn->setCheckable(true);
        langBtn->setProperty("langIndex", i);
        langBtn->setMinimumHeight(50);
        
        // Style to match the app's brown theme
        langBtn->setStyleSheet(
            "QPushButton { "
            "  background-color: rgba(249, 211, 176, 30%); "
            "  border: 1px solid #8F653F; "
            "  border-radius: 3px; "
            "  padding: 10px; "
            "  color: rgb(40, 19, 1); "
            "} "
            "QPushButton:checked { "
            "  background-color: #6D3603; "
            "  color: white; "
            "  border: 2px solid #8F653F; "
            "  font-weight: bold; "
            "} "
            "QPushButton:pressed { "
            "  border-style: inset; "
            "  background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #dadbde, stop: 1 #f6f7fa); "
            "}"
        );
        
        langButtonGroup->addButton(langBtn, i);
        
        // Check the current language button
        if (i == ui->langComboBox->currentIndex()) {
            langBtn->setChecked(true);
        }
        
        connect(langBtn, &QPushButton::clicked, this, [this, i]() {
            ui->langComboBox->setCurrentIndex(i);
            
            // Apply language change immediately
            int langIndex = i;
            if (langIndex >= 0 && langIndex < langs.size()) {
                LANG = langs[langIndex];
                
                extern QTranslator *translator;
                QApplication::removeTranslator(translator);
                
                if (LANG != "English") {
                    if (!translator->load(LANG + ".qm", ".")) {
                        (void)translator->load(LANG + ".qm", MAINPATH);
                    }
                    QApplication::installTranslator(translator);
                }
                
                // Retranslate UI to apply language immediately
                ui->retranslateUi(this);
                
                // Re-apply button text and icons after retranslation
                ui->settingsBTN->setText(tr("Settings"));
                ui->settingsBTN->setIcon(QIcon(":Icons/configure.png"));
                ui->menuBTN->setText("☰");
                ui->helpBTN->setText(tr("Help"));
                ui->helpBTN->setIcon(QIcon(":Icons/help-contents.png"));
                ui->aboutBTN->setText(tr("About"));
                ui->aboutBTN->setIcon(QIcon(":Icons/help-about.png"));
                ui->openBTN->setText(tr("Browse"));
                ui->openBTN->setIcon(QIcon(":Icons/Orayta.png"));
                ui->searchBTN->setText(tr("Search"));
                ui->searchBTN->setIcon(QIcon(":Icons/booksearch.png"));
                ui->getbooksBTN->setText(tr("Download"));
                ui->getbooksBTN->setIcon(QIcon(":Icons/dl-book.png"));
                ui->bookMarksBTN->setText(tr("Bookmarks"));
                ui->bookMarksBTN->setIcon(QIcon(":Icons/bookmarks-big.png"));
                ui->clearSearchBTN->setIcon(QIcon(":Icons/edit-delete.png"));
                ui->toIndexMenuBTN->setText("↑");
                ui->toMainMenuBTN->setText("⌂");
                ui->backBTN->setText("←");  // Back = left arrow (previous page)
                ui->forwardBTN->setText("→");  // Forward = right arrow (next page)
                
                // Save immediately
                QSettings settings("Orayta", "SingleUser");
                settings.beginGroup("Confs");
                settings.setValue("lang", LANG);
                settings.endGroup();
                
                qDebug() << "Language changed to:" << LANG << "and UI retranslated";
            }
            
            ui->saveConf->setEnabled(true);
        });
        langLayout->addWidget(langBtn);
    }
    
    // Add the button widget to the combobox's parent layout
    QLayout* parentLayout = ui->langComboBox->parentWidget()->layout();
    if (parentLayout) {
        parentLayout->addWidget(langButtonWidget);
    }
#endif

//    QSettings settings("Orayta", "SingleUser");

    /* this feature is disabled
    //Check if "use system lang" is set
    settings.beginGroup("Confs");
    bool useSystemLang = settings.value("systemLang",true).toBool();
    settings.endGroup();


    ui->systemLangCbox->setChecked(useSystemLang);
    ui->groupBox->setEnabled(!useSystemLang);
    */

    //Show current language
    int is = -1;
    for (int i=0; i<langs.size(); i++) if (LANG == langs[i]) is = i;
    ui->langComboBox->setCurrentIndex(is);

    //get stored settings for display font
    settings.beginGroup("Confs");
        qDebug() << "=== LOADING SETTINGS ===";
        qDebug() << "Settings file:" << settings.fileName();
        qDebug() << "Settings status:" << settings.status();
        
        // Check if the key exists
        bool hasFontSize = settings.contains("fontsize");
        qDebug() << "Settings contains 'fontsize' key:" << hasFontSize;
        
        QString defaultFont = "Droid Sans Hebrew Orayta";
        gFontFamily = settings.value("fontfamily", defaultFont).toString();

        //default set to 0. if it is so, adjustToScreenSize() will guess a better value depending on target screen dpi.
        gFontSize = settings.value("fontsize",0).toInt();
        
        qDebug() << "Loaded font family:" << gFontFamily;
        qDebug() << "Loaded font size:" << gFontSize;
        
        // List all keys in this group for debugging
        QStringList keys = settings.childKeys();
        qDebug() << "All keys in Confs group:" << keys;

        // Load interface size settings
        bool autoInterfaceSize = settings.value("autoInterfaceSize", true).toBool();
        int interfaceSize = settings.value("interfaceSize", 18).toInt();
        
        // Block signals to prevent triggering handlers during initialization
        ui->autoInterfaceSizeCKBX->blockSignals(true);
        ui->interfaceSizeSpinBox->blockSignals(true);
        ui->toolbarStyleComboBox->blockSignals(true);
        
        ui->autoInterfaceSizeCKBX->setChecked(autoInterfaceSize);
        ui->interfaceSizeSpinBox->setEnabled(!autoInterfaceSize);
        
        if (autoInterfaceSize) {
            // Show calculated auto size in spinbox (read-only)
            ui->interfaceSizeSpinBox->setValue(getAutoFontSize());
        } else {
            ui->interfaceSizeSpinBox->setValue(interfaceSize);
        }

        int toolbarStyle = settings.value("toolbarStyle", 0).toInt(); // Default to Text and Icons (0)
        ui->toolbarStyleComboBox->setCurrentIndex(toolbarStyle);
        
        // Unblock signals
        ui->autoInterfaceSizeCKBX->blockSignals(false);
        ui->interfaceSizeSpinBox->blockSignals(false);
        ui->toolbarStyleComboBox->blockSignals(false);
        
        applyToolbarStyle(toolbarStyle);
    settings.endGroup();
    
    qDebug() << "After loading settings, gFontSize is:" << gFontSize;

    resetSettingsPage();

    //as default display the font selection page int the settings page
    ui->tabWidget->setCurrentIndex(0);
}

void MobileApp::resetSettingsPage()
{
    qDebug() << "=== RESET SETTINGS PAGE ===";
    qDebug() << "Setting UI controls to gFontSize:" << gFontSize;
    
    // Block signals while setting values to prevent triggering preview/save
    ui->fontComboBox->blockSignals(true);
    ui->fonSizeSpinBox->blockSignals(true);
    ui->horizontalSlider->blockSignals(true);
    ui->NightModeCKBX->blockSignals(true);
    ui->customFontRDBTN->blockSignals(true);
    ui->defaultFontRDBTN->blockSignals(true);
    
    //Show current font values in the UI
    ui->fontComboBox->setCurrentFont(QFont(gFontFamily));
    ui->fonSizeSpinBox->setValue(gFontSize);
    ui->horizontalSlider->setValue(gFontSize);
    ui->NightModeCKBX->setChecked(nightMode);
    ui->customFontRDBTN->setChecked(useCustomFontForAll);
    ui->defaultFontRDBTN->setChecked(!useCustomFontForAll);
    
    // Unblock signals
    ui->fontComboBox->blockSignals(false);
    ui->fonSizeSpinBox->blockSignals(false);
    ui->horizontalSlider->blockSignals(false);
    ui->NightModeCKBX->blockSignals(false);
    ui->customFontRDBTN->blockSignals(false);
    ui->defaultFontRDBTN->blockSignals(false);
    
    //Update font preview with correct colors
    updateFontPreview();
    
    ui->saveConf->setEnabled(false);
    
    // Backup current settings for preview/cancel functionality
    backupCurrentSettings();
    
    qDebug() << "Reset complete. Spinbox value:" << ui->fonSizeSpinBox->value() << "Slider value:" << ui->horizontalSlider->value();
}

// Backup current settings before preview
void MobileApp::backupCurrentSettings()
{
    originalSettings.fontFamily = gFontFamily;
    originalSettings.fontSize = gFontSize;
    originalSettings.useCustomFont = useCustomFontForAll;
    originalSettings.nightMode = nightMode;
    originalSettings.autoInterfaceSize = ui->autoInterfaceSizeCKBX->isChecked();
    originalSettings.interfaceSize = ui->interfaceSizeSpinBox->value();
    originalSettings.toolbarStyle = ui->toolbarStyleComboBox->currentIndex();
    originalSettings.language = LANG;
    
    qDebug() << "Settings backed up for preview";
}

// Restore original settings (when user cancels)
void MobileApp::restoreOriginalSettings()
{
    qDebug() << "=== RESTORING ORIGINAL SETTINGS ===";
    qDebug() << "Restoring gFontSize to:" << originalSettings.fontSize;
    
    gFontFamily = originalSettings.fontFamily;
    gFontSize = originalSettings.fontSize;
    useCustomFontForAll = originalSettings.useCustomFont;
    nightMode = originalSettings.nightMode;
    
    // Block signals while restoring to prevent triggering preview
    ui->fontComboBox->blockSignals(true);
    ui->fonSizeSpinBox->blockSignals(true);
    ui->horizontalSlider->blockSignals(true);
    ui->NightModeCKBX->blockSignals(true);
    ui->customFontRDBTN->blockSignals(true);
    ui->defaultFontRDBTN->blockSignals(true);
    ui->autoInterfaceSizeCKBX->blockSignals(true);
    ui->interfaceSizeSpinBox->blockSignals(true);
    ui->toolbarStyleComboBox->blockSignals(true);
    
    // Restore UI controls
    ui->fontComboBox->setCurrentFont(QFont(gFontFamily));
    ui->fonSizeSpinBox->setValue(gFontSize);
    ui->horizontalSlider->setValue(gFontSize);
    ui->NightModeCKBX->setChecked(nightMode);
    ui->customFontRDBTN->setChecked(useCustomFontForAll);
    ui->defaultFontRDBTN->setChecked(!useCustomFontForAll);
    ui->autoInterfaceSizeCKBX->setChecked(originalSettings.autoInterfaceSize);
    ui->interfaceSizeSpinBox->setEnabled(!originalSettings.autoInterfaceSize);
    ui->interfaceSizeSpinBox->setValue(originalSettings.interfaceSize);
    ui->toolbarStyleComboBox->setCurrentIndex(originalSettings.toolbarStyle);
    
    // Unblock signals
    ui->fontComboBox->blockSignals(false);
    ui->fonSizeSpinBox->blockSignals(false);
    ui->horizontalSlider->blockSignals(false);
    ui->NightModeCKBX->blockSignals(false);
    ui->customFontRDBTN->blockSignals(false);
    ui->defaultFontRDBTN->blockSignals(false);
    ui->autoInterfaceSizeCKBX->blockSignals(false);
    ui->interfaceSizeSpinBox->blockSignals(false);
    ui->toolbarStyleComboBox->blockSignals(false);
    
    // Restore language
    if (LANG != originalSettings.language) {
        LANG = originalSettings.language;
        translate(LANG);
    }
    
    // Apply restored settings to UI
    adjustFontSize();
    applyToolbarStyle(originalSettings.toolbarStyle);
    updateFontPreview();
    
    // Reload book if we're viewing one
    if (displayer->getCurrentBook()) {
        displayer->getCurrentBook()->setFont(QFont(gFontFamily, gFontSize));
        showBook(displayer->getCurrentBook(), displayer->getCurrentIter());
    }
    
    qDebug() << "Original settings restored";
}

// Apply settings preview (immediate visual feedback without saving)
void MobileApp::applySettingsPreview()
{
    // Apply BOOK font changes (from Font tab)
    gFontFamily = ui->fontComboBox->currentFont().family();
    gFontSize = ui->fonSizeSpinBox->value();
    useCustomFontForAll = ui->customFontRDBTN->isChecked();
    nightMode = ui->NightModeCKBX->isChecked();
    
    // Apply toolbar style
    int toolbarStyle = ui->toolbarStyleComboBox->currentIndex();
    applyToolbarStyle(toolbarStyle);
    
    // Apply INTERFACE visual changes (affects UI, not book content)
    adjustFontSize();
    updateFontPreview();
    
    // Only reload book if we're currently viewing it (on DISPLAY_PAGE)
    // Don't reload if we're in settings - just update the preview box
    if (ui->stackedWidget->currentIndex() == DISPLAY_PAGE && displayer->getCurrentBook()) {
        displayer->getCurrentBook()->setFont(QFont(gFontFamily, gFontSize));
        showBook(displayer->getCurrentBook(), displayer->getCurrentIter());
    }
    
    qDebug() << "Settings preview applied - Book font:" << gFontSize << "Interface font:" << 
        (ui->autoInterfaceSizeCKBX->isChecked() ? "auto" : QString::number(ui->interfaceSizeSpinBox->value()));
}


//------------------------------------

//copied from desktopapp
void MobileApp::translate(QString newlang)
{
    LANG = newlang;

    extern QTranslator *translator;

    //Remove old translator
    QApplication::removeTranslator(translator);

    //English needs no translator, it's the default
    if (LANG != "English")
    {
        if (!translator->load(LANG + ".qm", ".")) {
            (void)translator->load(LANG + ".qm", MAINPATH);
        }
        QApplication::installTranslator(translator);
    }

    // Skip UI retranslation on Android to prevent crashes
    // Language will take effect on next app restart
#ifndef Q_OS_ANDROID
    ui->retranslateUi(this);
#else
    // On Android, skip retranslation - language will apply on next app start
    qDebug() << "Language changed to:" << LANG << "- will take effect on restart";
#endif

//    if (LANG == "Hebrew") setDirection(true);
//    else setDirection(false);
}

void MobileApp::on_langComboBox_currentIndexChanged(int index)
{
    //Settings have changed, so the save button should be enabled
    ui->saveConf->setEnabled(true);
    
    // Apply language preview immediately
    if (index >= 0 && index < langs.size()) {
        QString newLang = langs[index];
        if (newLang != LANG) {
            LANG = newLang;
            translate(LANG);
            
            // Re-apply button text and icons after retranslation
            ui->settingsBTN->setText(tr("Settings"));
            ui->settingsBTN->setIcon(QIcon(":Icons/configure.png"));
            ui->menuBTN->setText("☰");
            ui->helpBTN->setText(tr("Help"));
            ui->helpBTN->setIcon(QIcon(":Icons/help-contents.png"));
            ui->aboutBTN->setText(tr("About"));
            ui->aboutBTN->setIcon(QIcon(":Icons/help-about.png"));
            ui->openBTN->setText(tr("Browse"));
            ui->openBTN->setIcon(QIcon(":Icons/Orayta.png"));
            ui->searchBTN->setText(tr("Search"));
            ui->searchBTN->setIcon(QIcon(":Icons/booksearch.png"));
            ui->getbooksBTN->setText(tr("Download"));
            ui->getbooksBTN->setIcon(QIcon(":Icons/dl-book.png"));
            ui->bookMarksBTN->setText(tr("Bookmarks"));
            ui->bookMarksBTN->setIcon(QIcon(":Icons/bookmarks-big.png"));
            ui->clearSearchBTN->setIcon(QIcon(":Icons/edit-delete.png"));
            ui->toIndexMenuBTN->setText("↑");
            ui->toMainMenuBTN->setText("⌂");
            ui->backBTN->setText("←");
            ui->forwardBTN->setText("→");
            
            qDebug() << "Language preview changed to:" << newLang;
        }
    }
}

void MobileApp::on_mixedSelectBTN_clicked()
{
    ui->stackedWidget->setCurrentIndex(MIXED_SELECTION_PAGE);
}

void MobileApp::setupMixedSelection(){
    Book* book = displayer->getCurrentBook();

    // no current book or no commentaries available.
    if (!book || !book->IsMixed()){
        ui->noCommemtaries->show();
        ui->mixedGroup->hide();

        return;
    }
    else if (book->IsMixed())
    //Show / Hide mixed display stuff
    {
        ui->mixedGroup->show();
        ui->noCommemtaries->hide();

        //Clear old entries
        ui->selectionArea->clear();

        for(int i=1; i<book->mWeavedSources.size(); i++)
        {
            QListWidgetItem * item = new QListWidgetItem(book->mWeavedSources[i].Title);
            item->setCheckState(book->mWeavedSources[i].show? Qt::Checked : Qt::Unchecked);
            QString id = stringify(book->mWeavedSources[i].id);
            item->setWhatsThis(id);
            item->setToolTip(item->checkState() == Qt::Checked? "True" : "False");
            ui->selectionArea->addItem(item);

            // if the book isn't installed
            QFile f (book->mWeavedSources[i].FilePath);
            if (!f.exists())
            {
                item->setText(item->text() + tr(" (Not installed. please install it from 'Get books' page)"));
                item->setForeground(QBrush(QColor("gray")));
                item->setCheckState(Qt::PartiallyChecked);
            }
        }
    }
}

void MobileApp::on_openMixed_clicked()
{
    if (!displayer->getCurrentBook()) return;
    Book* b = displayer->getCurrentBook();

    bool showalone =true;

    for(int j=1; j<b->mWeavedSources.size(); j++)
    {
        QString srcId = stringify(b->mWeavedSources[j].id);

        for (int i =0; i< ui->selectionArea->count(); i++)
        {
            QListWidgetItem *item = ui->selectionArea->item(i);

              QString widgetId = item->whatsThis();

              if (srcId == widgetId) //we found the currect item
              {
                  bool checked = item->checkState() == Qt::Checked? true : false;

                  //set the showability of this item to what the user chose.
                  b->mWeavedSources[j].show = checked;
                  if (checked) showalone = false;
                  break;
              }
        }
    }

    b->showAlone = showalone;

    // save the settings for this book
    if (b->getUniqueId() != -1 && !(b->hasRandomId)) //book has normal uid
    {
        QSettings settings("Orayta", "SingleUser");
        settings.beginGroup("Book" + stringify(b->getUniqueId()));
        settings.setValue("ShowAlone", b->showAlone);
        for (int j=1; j<b->mWeavedSources.size(); j++)
        {
            settings.setValue("Shown" + stringify(j), b->mWeavedSources[j].show);
        }
        settings.setValue("InSearch", b->IsInSearch());
        settings.endGroup();
    }

    //show the book
    qApp->processEvents();
    showBook(b, displayer->getCurrentIter());

}

void MobileApp::on_markAllBTN_clicked()
{
    //mark all items as checked
    for (int i =0; i< ui->selectionArea->count(); i++)
        ui->selectionArea->item(i)->setCheckState(Qt::Checked);
}

void MobileApp::on_unmarkAllBTN_clicked()
{
    //uncheck all items
    for (int i =0; i< ui->selectionArea->count(); i++)
        ui->selectionArea->item(i)->setCheckState(Qt::Unchecked);
}

void MobileApp::on_markAllBTN_2_clicked()
{
    //mark all items as checked
    for (int i =0; i< ui->downloadListWidget->count(); i++)
        ui->downloadListWidget->item(i)->setCheckState(Qt::Checked);
}

void MobileApp::on_unmarkAllBTN_2_clicked()
{
    //uncheck all items
    for (int i =0; i< ui->downloadListWidget->count(); i++)
    {
        ui->downloadListWidget->item(i)->setCheckState(Qt::Unchecked);
    }
}

void MobileApp::on_markAllBTN_3_clicked()
{
    for (Book * book : booksInSearch)
    {
        book->select();
    }
}

void MobileApp::on_unmarkAllBTN_3_clicked()
{
    for (Book * book : booksInSearch)
    {
        book->unselect();
    }
}


void MobileApp::on_copyTextBTN_clicked()
{
    //Copy the text of the whole chapter to the clipboard
    QApplication::clipboard()->setText(displayer->toPlainText());
}


void MobileApp::on_selectionArea_itemClicked(QListWidgetItem *item)
{
    // Ignore clicks if we were just scrolling
    if (selectionAreaScrolling) return;
    
    // Ignore if user was scrolling/swiping
    if (touchMoved) return;
    
    //This is a little hack to prevent double events on some android machines and emulators.
    //If the function is called again in less than 2 ms, the second time is ignored.
    qint64 miliSec = timer.restart();
    if (miliSec < 200) return;

    // ignore books that aren't installed:
    if (item->checkState() == Qt::PartiallyChecked) return;

    //Invert the selection of the item only if it was not chnaged by the click itself already.
    // (In other words, if the user clicked the checkbox, it will work without us. if he clicked somewhere else - we should invert the value)
    if ((item->checkState() == Qt::Checked && item->toolTip() == "True") ||
        (item->checkState() == Qt::Unchecked && item->toolTip() == "False") )
    {
        if (item->checkState() == Qt::Checked)
        {
            item->setCheckState(Qt::Unchecked);
            item->setToolTip("False");
        }
        else
        {
            item->setCheckState(Qt::Checked);
            item->setToolTip("True");
        }
    }
    else
    {
        if (item->checkState() == Qt::Checked) item->setToolTip("True");
        else item->setToolTip("False");
    }

}

/* derecated
void MobileApp::on_moreInfoBTN_clicked()
{
    //show wellcome page
    Book *wellcome = bookList.findBookById(1);

    showBook(wellcome);
}

void MobileApp::on_helpBTN_clicked()
{
    if (LANG.contains( "Hebrew"))
        //show hebrew help
        showBook(bookList.findBookById(2));
    else
       showBook( bookList.findBookById(3));
}
*/

void MobileApp::on_resetBookListBTN_clicked()
{
    ui->treeWidget->collapseAll();
}

void MobileApp::on_lastBookBTN_clicked()
{
    //probably we just loaded the app
    if (!displayer->getCurrentBook())
    {
        //get last open book
        QSettings settings("Orayta", "SingleUser");
        settings.beginGroup("History");
            int lastBookId = settings.value("lastBook").toInt();
            Book *b = bookList.findBookById(lastBookId);
            BookIter itr = BookIter::fromEncodedString(settings.value("position", "").toString());
            int vp =  settings.value("viewposition").toInt();
        settings.endGroup();

        if (!b) return;
        showBook(b, itr);
        displayer->verticalScrollBar()->setValue(vp);
    }
    else
        ui->stackedWidget->setCurrentIndex(DISPLAY_PAGE);
}


void MobileApp::on_gtoHelp_clicked()
{
    //try to show the help page
    Book *helpBook = NULL;
    if (LANG.contains( "Hebrew"))
        //show hebrew help
        helpBook = bookList.findBookById(2);
    else
        helpBook = bookList.findBookById(3);

    if (helpBook)
        showBook(helpBook);
}

void MobileApp::on_backBTN_clicked()
{
    // In RTL/Hebrew books, "back" means previous page which is to the RIGHT
    // Use the same navigation as right swipe to keep currentIter in sync
    displayer->rightSwipe();
}

void MobileApp::on_forwardBTN_clicked()
{
    // In RTL/Hebrew books, "forward" means next page which is to the LEFT
    // Use the same navigation as left swipe to keep currentIter in sync
    displayer->leftSwipe();
}

void MobileApp::on_ZoomInBTN_clicked()
{
    float p = (float) displayer->verticalScrollBar()->value() / displayer->verticalScrollBar()->maximum();

    displayer->increaseSize();

    displayer->verticalScrollBar()->setValue(displayer->verticalScrollBar()->maximum() * p);
}

void MobileApp::on_ZoomOutBTN_clicked()
{
    float p = (float) displayer->verticalScrollBar()->value() / displayer->verticalScrollBar()->maximum();

    displayer->decreaseSize();

    displayer->verticalScrollBar()->setValue(displayer->verticalScrollBar()->maximum() * p);
}

void MobileApp::on_toMainMenuBTN_clicked()
{
    ui->stackedWidget->setCurrentIndex(MAIN_PAGE);
}


void MobileApp::displayKukaytaMessage()
{
    qDebug()<<"is kukayta installed ?";
    qDebug()<<"?: " <<isKukaytaInstalled();
    if (! isKukaytaInstalled())
        return;

    bool firstTimeAfterInstalledKukayta = false;

    QSettings settings("Orayta", "SingleUser");
    settings.beginGroup("Confs");
    firstTimeAfterInstalledKukayta = !settings.contains("kukaytaInstalled");


    if (firstTimeAfterInstalledKukayta)
    {
        ui->stackedWidget->setCurrentIndex(KUKAYTA_PAGE);
        ui->kukaytaArea->setCurrentIndex(1);
    }

    settings.setValue("kukaytaInstalled", true);

    settings.endGroup();
}


void MobileApp::on_helpBTN_clicked()
{
    on_gtoHelp_clicked();
}

void MobileApp::on_findBookBTN_clicked()
{
//    ui->stackedWidget->setCurrentWidget(bookFindDialog);
    ui->stackedWidget->setCurrentIndex(BOOKFIND_PAGE);
    bookFindDialog->Reset();
}

void MobileApp::on_customFontRDBTN_toggled(bool checked)
{ 
    ui->saveConf->setEnabled(true);
    applySettingsPreview();
}

void MobileApp::on_autoResumeCKBX_stateChanged(int arg1)
{ ui->saveConf->setEnabled(true);}

void MobileApp::on_NightModeCKBX_clicked(bool checked)
{ 
    // Enable save button since settings changed
    ui->saveConf->setEnabled(true);
    
    // Apply preview immediately
    applySettingsPreview();
    
    qDebug() << "Night mode preview changed to:" << checked;
}

void MobileApp::updateFontPreview()
{
    // Update font preview with current font settings and night mode colors
    QString fontFamily = ui->fontComboBox->currentFont().family();
    int fontSize = ui->fonSizeSpinBox->value();
    
    // Set font directly on the widget
    QFont previewFont(fontFamily, fontSize);
    ui->fontPreview->setFont(previewFont);
    
    // Apply night mode colors AND font size in stylesheet to ensure it's not overridden
    QString styleSheet;
    if (ui->NightModeCKBX->isChecked()) {
        styleSheet = QString("QLabel { font-family: '%1'; font-size: %2pt; color: #7faf70; background-color: black; }")
            .arg(fontFamily).arg(fontSize);
    } else {
        styleSheet = QString("QLabel { font-family: '%1'; font-size: %2pt; color: black; background-color: white; }")
            .arg(fontFamily).arg(fontSize);
    }
    ui->fontPreview->setStyleSheet(styleSheet);
    
    qDebug() << "Font preview updated - Family:" << fontFamily << "Size:" << fontSize;
}

void MobileApp::on_dlKukaytaBooksBTN_clicked()
{
    autoInstallKukBooksFlag=true;
    ui->stackedWidget->setCurrentIndex(GET_BOOKS_PAGE);
    downloadBookList();
}


void MobileApp::on_installKukaytaBTN_clicked()
{
    #ifdef KOOKITA
        installKukayta();
    #endif
}


void MobileApp::on_dlKukaytaBooksBTN__clicked()
{
    goBack();
}

void MobileApp::on_settingsBTN_clicked()
{
    ui->stackedWidget->setCurrentIndex(SETTINGS_PAGE);
}



void MobileApp::on_pushButton_clicked()
{
    QString link = "https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=P8RH8U6ABNJ38";
    QDesktopServices::openUrl(QUrl(link));
}

// gets called when qApp-applicationStateChanged() is sent (android lifecycle state changed)
void MobileApp::stateChanged()
{
    //qDebug() << qApp->applicationState();
    switch (qApp->applicationState()) {
    case Qt::ApplicationInactive:
        saveSettings();
        break;
    default:
        break;
    }

}

void MobileApp::on_menuBTN_clicked()
{
    showMenu();
}


#ifdef Q_OS_ANDROID
// JNI function called from Java
extern "C" JNIEXPORT jboolean JNICALL
Java_org_Orayta_OraytaActivity_handleBackButton(JNIEnv *env, jobject obj)
{
    if (g_mobileAppInstance) {
        return g_mobileAppInstance->handleAndroidBackButton();
    }
    return false;  // Default behavior if no instance
}
#endif
