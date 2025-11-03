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
* Author: Moshe Wagner. <moshe.wagner@gmail.com>
*/

#include "mobileapp.h"
#include "ui_mobileapp.h"
#include <QDebug>
#include <QDir>

#ifdef Q_OS_ANDROID
    #include "permissionutils.h"
    #include <QMessageBox>
#endif

/*
  This file includes all functions of the mobileapp class that deal with downloading books -
  The booklist file abd the books themselves.
 */


// start download of the selected books.
void MobileApp::on_downloadBTN_clicked()
{
    downloadStart();
}

// stop the current download
void MobileApp::on_stopDownloadBTN_clicked()
{
    qDebug() << "Stop download button clicked";
    
    // Abort all active downloads
    for (FileDownloader* downloader : activeDownloaders) {
        if (downloader) {
            disconnect(downloader, nullptr, this, nullptr);
            downloader->abort();
            downloader->deleteLater();
        }
    }
    activeDownloaders.clear();
    
    // Clear the download queue
    downloadsList.clear();
    hashs.clear();
    
    // Reset download counters for next download session
    int downloadedCount = completedDownloads;
    completedDownloads = 0;
    failedDownloads = 0;
    
    // Reload the book tree to show any books that were successfully downloaded
    qDebug() << "Reloading book list after download stop...";
    reloadBooklist();
    qDebug() << "Book list reloaded. Total books:" << bookList.size();
    
    // Refresh the download list window
    ui->downloadListWidget->clear();
    
    // Update UI with helpful message
    if (downloadedCount > 0) {
        ui->downloadInfo->setText(tr("Download stopped. %1 file(s) downloaded successfully.").arg(downloadedCount));
    } else {
        ui->downloadInfo->setText(tr("Download stopped by user"));
    }
    ui->downloadPrgBar->hide();
    ui->downloadListWidget->setEnabled(true);
    ui->downloadBTN->setEnabled(true);
    ui->stopDownloadBTN->hide();
    
    // Switch to book list page to show the downloaded books
    if (ui->stackedWidget->currentIndex() == GET_BOOKS_PAGE && downloadedCount > 0) {
        ui->stackedWidget->setCurrentIndex(LIST_PAGE);
    }
    
    qDebug() << "Download stopped - completed:" << completedDownloads << "failed:" << failedDownloads;
}

void MobileApp::on_downloadListWidget_itemClicked(QListWidgetItem *item)
{
    //This is a little hack to prevent double events on some android machines and emulators.
    //If the function is called again in less than 2 ms, the second time is ignored.
    qint64 miliSec = timer.restart();
    if (miliSec < 200) return;

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

void MobileApp::downloadBookList()
{
    //show aprorpriate widgets
    ui->downloadSTKWidget->setCurrentIndex(0);
    
    // Reset horizontal scroll position to show the content (not the left edge)
    ui->downloadListWidget->horizontalScrollBar()->setValue(0);

    qDebug() << "downloadBookList called, list count:" << ui->downloadListWidget->count();

    //If we already created a download list widget, we are done.
    // In any other case, we should download the list.
    if (ui->downloadListWidget->count() < 1)
    {
        qDebug() << "Starting new download...";
        
        // Clean up existing downloader if any
        if (listdownload) {
            disconnect(listdownload, nullptr, this, nullptr);
            listdownload->deleteLater();
        }
        
        listdownload = new FileDownloader();
        connect(listdownload, SIGNAL(done()), this, SLOT(listDownloadDone()));
        connect(listdownload, SIGNAL(downloadError()), this, SLOT(downloadError()));
        listdownload->Download(BOOKLISTURL, SAVEDBOOKLIST, true);
    }
    else 
    {
        qDebug() << "List already exists, showing it";
        ui->downloadSTKWidget->setCurrentIndex(1);
    }
}

void MobileApp::downloadDailyLimudFiles()
{
    FileDownloader * limudDownloader = new FileDownloader();
    // Connect to done signal to clean up after download
    connect(limudDownloader, &FileDownloader::done, limudDownloader, &FileDownloader::deleteLater);
    connect(limudDownloader, &FileDownloader::downloadError, limudDownloader, &FileDownloader::deleteLater);
    limudDownloader->Download(LIMUD_YOMI_URL, LIMUD_YOMI_FILE, true);
}

void MobileApp::updateDownloadableList()
{
    QList <QString> dl;
    groups.clear();

    ReadFileToList(SAVEDBOOKLIST, dl, "UTF-8");
    parseDLFile(dl);

    //show aprorpriate widgets
    ui->downloadSTKWidget->setCurrentIndex(1);


    //Clear the old list
    ui->downloadListWidget->clear();

    //Build the new list
    for (int i=0; i<groups.size(); i++)
    {
        if (groups[i].downloadState != 0)
        {
            QListWidgetItem *lwi;
            lwi= new QListWidgetItem(groups[i].name + " (" + QString::number(groups[i].downloadSize)
                                     + /* "/" + QString::number(groups[i].fullSize) + */ " MB)");
            if (autoInstallKukBooksFlag && groups[i].name.contains("הרחבה"))
                lwi->setCheckState(Qt::Checked);
            else
                lwi->setCheckState(Qt::Unchecked);
            lwi->setWhatsThis(stringify(i));
            lwi->setToolTip("False");
            if(groups[i].hidden)
                lwi->setForeground(QBrush(QColor("gray")));

            ui->downloadListWidget->addItem(lwi);
            ui->downloadListWidget->setEnabled(true);
        }
    }

    if(autoInstallKukBooksFlag)
        downloadStart();

}

// Parse the booklist file
void MobileApp::parseDLFile(QList <QString> dl)
{
    for (int i=0; i<dl.size(); i++)
    {
        //Comment, ignore
        if (dl[i].startsWith("#")) {}
        //Group name
        else if (dl[i].startsWith("@"))
        {
            //Create new group
            DownloadbleBookGroup g;
            g.name = dl[i].mid(2);
            g.hidden=false;

            groups.append(g);
        }

        else if (dl[i].startsWith("./"))
        {
            if (groups.size() < 1)
            {
                qDebug() << "Error! Book with no group... Aborting download list";
                return ;
            }

            DownloadbleBookObject bo;
            bo.hash = "";
            QStringList sl = dl[i].split(", ");
            if (sl.size() < 3)
            {
                qDebug() << "Error! Invalid book entry! Skipping.";
                break ;
            }

            QString t = sl[0];
            bo.URL = t.replace("./", "https://raw.githubusercontent.com/MosheWagner/Orayta-Books/master/books/");
            bo.UnpackPath = sl[0].replace("./", BOOKPATH);
            int n; if (ToNum(sl[1], &n)) bo.fileSize = n / 1000000.0;

            bo.dateModified = QDate::fromString(sl[2].simplified(), "dd/MM/yy");
            //Because QT thinks '12' is 1912 and not 2012...
            bo.dateModified.setDate(100 + bo.dateModified.year(), bo.dateModified.month(), bo.dateModified.day());

            //If a hash is present for this line
            if(sl.size() > 3)
            {
                bo.hash = sl[3];
            }

            groups.last().books.append(bo);
        }
    }


    for (int i=0; i<groups.size(); i++)
    {
        //qDebug() << "#" << groups[i].name << groups[i].groupSize;

        bool hasAll = true;
        bool hasNone = true;

        for (int j=0; j<groups[i].books.size(); j++)
        {
            Book * b = bookList.FindBookByPath(groups[i].books[j].UnpackPath);

            bool needToDownload;

            if (b)
            {
                if (b->modificationDate() >= QDateTime(groups[i].books[j].dateModified, QTime()))
                {
                    needToDownload = false;
                }
                else needToDownload = true;
            }
            else needToDownload = true;

            if (!needToDownload)
            {
                    groups[i].books[j].needToDownload = false;
                    hasNone = false;
            }
            else
            {
                groups[i].books[j].needToDownload = true;
                hasAll = false;
            }
        }

        //Calculate size of download
        double fs = 0, ds = 0;
        for (int k=0; k < groups[i].books.size(); k++)
        {
            if (groups[i].books[k].needToDownload) ds += groups[i].books[k].fileSize;
            fs += groups[i].books[k].fileSize;
        }
        groups[i].fullSize = int (fs * 10) / 10.0;
        groups[i].downloadSize = int (ds * 10) / 10.0;

        if (hasAll) groups[i].downloadState = 0; //All installed
        else if (hasNone) groups[i].downloadState = 2; //None installed
        else groups[i].downloadState = 1; //Needs update

        //qDebug() << groups[i].name <<  groups[i].downloadState << groups[i].fullSize << groups[i].downloadSize;
   }
}


void MobileApp::listDownloadDone()
{
    qDebug() << "List download completed";
    //If all is ok
    if (listdownload)
    {
        if (listdownload->getFileName().contains("Orayta"))
        {
            updateDownloadableList();
            // Clean up the downloader
            listdownload->deleteLater();
            listdownload = NULL;
        }
        else
        {
            qDebug() << "Downloaded file doesn't contain 'Orayta', retrying...";
            listdownload->deleteLater();
            listdownload = NULL;
            //Retry the download
            downloadBookList();
        }

    }
    else
    {
        qDebug() << "listdownload is null, recreating...";
        // this shouldn't happen, but if so we should recreate everything.
        downloadBookList();
    }
}

void MobileApp::downloadStart()
{
    downloadsList.clear();
    hashs.clear();
    completedDownloads = 0;
    failedDownloads = 0;

    for (int i=0; i<ui->downloadListWidget->count(); i++)
    {
        QListWidgetItem *item = ui->downloadListWidget->item(i);
        if (item->checkState() == Qt::Checked)
        {
            //Generate download urls
            int n;
            if (ToNum(item->whatsThis(), &n))
            {
                if (groups.size() > n)
                {
                    for (int j=0; j<groups[n].books.size(); j++)
                    {
                        if (groups[n].books[j].needToDownload)
                        {
                            QString url = groups[n].books[j].URL;
                            downloadsList << url;
                            hashs << groups[n].books[j].hash;
                        }
                    }
                }
            }
        }
    }
    downloadNum = downloadsList.size();

    ui->downloadInfo->toolTip() = "";
    ui->downloadInfo->setText(tr("Starting download of %1 file(s)...").arg(downloadNum));

    ui->downloadListWidget->setEnabled(false);
    ui->downloadBTN->setEnabled(false);
    ui->stopDownloadBTN->show();
    ui->downloadPrgBar->show();
    ui->downloadPrgBar->setValue(0);

    // Start parallel downloads
    for (int i = 0; i < maxParallelDownloads && !downloadsList.isEmpty(); i++) {
        downloadNext();
    }
}

// download the next file in downloadsList.
void MobileApp::downloadNext()
{
    if (!downloadsList.isEmpty())
    {
#ifdef Q_OS_ANDROID
        // Check if we have write permissions before downloading (only check once)
        if (activeDownloaders.isEmpty() && !PermissionUtils::canWriteExternalStorage()) {
            qDebug() << "No write permission for downloading books, requesting permission...";
            PermissionUtils::requestStoragePermissions([this](bool granted) {
                if (granted) {
                    qDebug() << "Permission granted, continuing download";
                    // Restart parallel downloads
                    for (int i = 0; i < maxParallelDownloads && !downloadsList.isEmpty(); i++) {
                        performDownloadNext();
                    }
                } else {
                    qWarning() << "Storage permission denied, cannot download books";
                    ui->downloadInfo->setText(tr("Download failed: Storage permission required"));
                    ui->downloadInfo->setToolTip("Error");
                    ui->downloadPrgBar->hide();
                    ui->downloadListWidget->setEnabled(true);
                    ui->downloadBTN->setEnabled(true);
                    ui->stopDownloadBTN->hide();
                    
                    QMessageBox::warning(this, tr("Permission Required"), 
                        tr("Storage permission is required to download books. "
                           "Please grant permission to continue downloading."));
                }
            });
            return;
        }
#endif
        
        performDownloadNext();
    }
    //No more books to download and all active downloads finished
    else if (activeDownloaders.isEmpty())
    {
        //display download information:
        QString statusMsg;
        if (failedDownloads > 0) {
            statusMsg = tr("Download complete! %1 succeeded, %2 failed.")
                .arg(completedDownloads).arg(failedDownloads);
            ui->downloadInfo->setToolTip("Error");
        } else {
            statusMsg = tr("Download complete! %1 file(s) downloaded successfully.")
                .arg(completedDownloads);
        }
        ui->downloadInfo->setText(statusMsg);

        //reload the book tree
        reloadBooklist();

        //Refresh the download list window
        ui->downloadListWidget->clear();

        //reset the download page
        ui->downloadPrgBar->hide();
        ui->downloadListWidget->setEnabled(true);
        ui->downloadBTN->setEnabled(true);
        ui->stopDownloadBTN->hide();

        if (failedDownloads == 0)
        {
            //Switch view to book tree to see the new books
            if (ui->stackedWidget->currentIndex() == GET_BOOKS_PAGE)
                ui->stackedWidget->setCurrentIndex(LIST_PAGE);
        }

        autoInstallKukBooksFlag = false;
    }
}


void MobileApp::downloadProgress(int val) 
{ 
    // Calculate overall progress based on completed + active downloads
    int totalProgress = (completedDownloads * 100 + val) / downloadNum;
    ui->downloadPrgBar->setValue(totalProgress);
}

#include <QMessageBox>

void MobileApp::downloadError()
{
    FileDownloader* downloader = qobject_cast<FileDownloader*>(sender());
    if (downloader) {
        qDebug() << "Error downloading: " + downloader->getFileName();
        activeDownloaders.removeOne(downloader);
        downloader->deleteLater();
        failedDownloads++;
    } else {
        qDebug() << "Download error but downloader is null";
    }

    ui->downloadInfo->setToolTip("Error");
    
    // Update progress display
    int totalCompleted = completedDownloads + failedDownloads;
    ui->downloadInfo->setText(tr("Downloading... %1 of %2 (%3 failed)")
        .arg(totalCompleted).arg(downloadNum).arg(failedDownloads));

    //this file has finished, start the next one
    downloadNext();
}

//called when book package has done downloading.
void MobileApp::downloadDone()
{
    FileDownloader* downloader = qobject_cast<FileDownloader*>(sender());
    if (downloader)
    {
        qDebug() << "Book download completed: " << downloader->getFileName();
        activeDownloaders.removeOne(downloader);
        downloader->deleteLater();
        completedDownloads++;
        
        // Update progress display
        int totalCompleted = completedDownloads + failedDownloads;
        ui->downloadInfo->setText(tr("Downloading... %1 of %2")
            .arg(totalCompleted).arg(downloadNum));

        //this file has finished downloading, get the next file.
        downloadNext();
    }
    else
    {
        qDebug() << "downloadDone called but downloader is null";
    }
}

void MobileApp::performDownloadNext()
{
    // Safety check to prevent infinite recursion
    if (downloadsList.isEmpty()) {
        qDebug() << "performDownloadNext called with empty download list";
        return;
    }
    
    // Safety check for list size mismatch
    if (hashs.size() != downloadsList.size()) {
        qWarning() << "Hash list size mismatch! hashs:" << hashs.size() << "downloads:" << downloadsList.size();
        // Pad with empty strings if needed
        while (hashs.size() < downloadsList.size()) {
            hashs.append("");
        }
    }

    QString url = downloadsList.first();
    QString hash = hashs.isEmpty() ? "" : hashs.first();
    QString name = url.mid(url.lastIndexOf("/") + 1);
    //Generate download target
    QString target = QString(url).replace("https://raw.githubusercontent.com/MosheWagner/Orayta-Books/master/books/", BOOKPATH);
    QString p = target.left(target.length() - name.length());
    QDir().mkpath(p);

    // Remove from list BEFORE starting download to prevent recursion issues
    downloadsList.removeFirst();
    if (!hashs.isEmpty()) {
        hashs.removeFirst();
    }
    
    // Create a new downloader for this file
    FileDownloader* newDownloader = new FileDownloader();
    activeDownloaders.append(newDownloader);
    
    // Connect signals
    connect(newDownloader, &FileDownloader::done, this, &MobileApp::downloadDone);
    connect(newDownloader, &FileDownloader::downloadError, this, &MobileApp::downloadError);
    connect(newDownloader, SIGNAL(downloadProgress(int)), this, SLOT(downloadProgress(int)));
    
    // Start download
    newDownloader->Download(url, target, false, hash);
    
    qDebug() << "Started download" << (downloadNum - downloadsList.size()) << "of" << downloadNum << ":" << name;
}

