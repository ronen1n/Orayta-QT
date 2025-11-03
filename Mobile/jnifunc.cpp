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
* Author: Avi Izar <izar00@gmail.com>
*/

/*
 * purpose: this file will hold usefull functions for comunicating with java part of the android app
 */

#include "jnifunc.h"
#include <QJniObject>
#include <QJniEnvironment>
#include <QCoreApplication>
// QNativeInterface is included via QCoreApplication in Qt6

QString pwd = "ElOB2wAJ!";

int initRequest()
{
    // Qt6: Use QNativeInterface to get Android context
    QJniObject context = QNativeInterface::QAndroidApplication::context();
    
    // Qt6: Updated package name qt5 -> qt
    jint success = QJniObject::callStaticMethod<jint>(
        "org/qtproject/qt/crypt/Crypter", 
        "init", 
        "(Landroid/content/Context;)I", 
        context.object<jobject>()
    );
    
    // Add JNI exception checking
    QJniEnvironment env;
    if (env.checkAndClearExceptions()) {
        qWarning() << "JNI exception occurred in initRequest";
        return -1;
    }
    
    return (int) success;
}

bool testIsKukaytaInstalled()
{
    // Qt6: QJniObject instead of QAndroidJniObject, qt5 -> qt
    jboolean installed = QJniObject::callStaticMethod<jboolean>("org/qtproject/qt/crypt/Crypter", "isKukaytaInstalled");

    return (bool) installed;
}

void installKukayta()
{
    // Qt6: QJniObject instead of QAndroidJniObject, qt5 -> qt
    QJniObject::callStaticMethod<void>("org/qtproject/qt/crypt/Crypter","installKukatya");
}

int zipDecrypt(QString zipFilename, QString internalFile, QString target)
{
    // Qt6: QJniObject instead of QAndroidJniObject
    QJniObject zipPath = QJniObject::fromString(zipFilename);
    QJniObject intFile = QJniObject::fromString(internalFile);
    QJniObject Target = QJniObject::fromString(target);
    QJniObject password = QJniObject::fromString(pwd);

    // Qt6: Updated package name qt5 -> qt
    jint res = QJniObject::callStaticMethod<jint>("org/qtproject/qt/crypt/Crypter", "zipDecrypt","(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I",
                                        zipPath.object<jstring>(), intFile.object<jstring>(), Target.object<jstring>(), password.object<jstring>());

    // Add JNI exception checking
    QJniEnvironment env;
    if (env.checkAndClearExceptions()) {
        qWarning() << "JNI exception occurred in zipDecrypt";
        return -1;
    }

    return (int) res;
}




