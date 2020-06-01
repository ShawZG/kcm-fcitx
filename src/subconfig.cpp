/***************************************************************************
 *   Copyright (C) 2011~2011 by CSSlayer                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include <QFileInfo>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>

#include <fcitx-utils/utils.h>
#include <fcitx-config/xdg.h>

// self
#include "subconfig.h"
#include "subconfigpattern.h"
#include "global.h"

namespace Fcitx
{


QStringList getFilesByPattern(QDir& currentdir, const QStringList& filePatternList, int index)
{
    QStringList result;
    if (!currentdir.exists())
        return result;

    const QString& filter = filePatternList.at(index);
    QStringList filters;
    filters << filter;
    QDir::Filters filterflag;

    if (index + 1 == filePatternList.size()) {
        filterflag = QDir::Files;
    } else {
        filterflag = QDir::Dirs | QDir::NoDotAndDotDot;
    }

    QStringList list = currentdir.entryList(filters, filterflag);
    if (index + 1 == filePatternList.size()) {
        Q_FOREACH(const QString & item, list) {
            result << currentdir.absoluteFilePath(item);
        }
    } else {
        Q_FOREACH(const QString & item, list) {
            QDir dir(currentdir.absoluteFilePath(item));
            result << getFilesByPattern(dir, filePatternList, index + 1);
        }
    }
    return result;
}

QSet<QString> getFiles(const QStringList& filePatternList, bool user)
{
    size_t size;
    char** xdgpath;

    if (user)
        xdgpath = FcitxXDGGetPathUserWithPrefix(&size, "");
    else
        xdgpath = FcitxXDGGetPathWithPrefix(&size, "");

    QSet<QString> result;
    for (size_t i = 0; i < size; i ++) {
        QDir dir(xdgpath[i]);
        QStringList list = getFilesByPattern(dir, filePatternList, 0);
        Q_FOREACH(const QString & str, list) {
            result.insert(
                dir.relativeFilePath(str));
        }
    }

    FcitxXDGFreePath(xdgpath);

    return result;
}

void SubConfig::parseConfigFileSubConfig(const SubConfigPattern* pattern)
{
    m_fileList = getFiles(pattern->filePatternList(), false);
    m_configdesc = pattern->configdesc();
}

void SubConfig::parseNativeFileSubConfig(const SubConfigPattern* pattern)
{
    m_mimetype = pattern->mimetype();
    m_nativepath = pattern->nativepath();
    m_filePatternList = pattern->filePatternList();
    updateFileList();
}

void SubConfig::parsePluginSubConfig(const SubConfigPattern* pattern)
{
    m_nativepath = pattern->nativepath();
}

void SubConfig::updateFileList()
{
    if (m_type == SC_NativeFile) {
        m_fileList = getFiles(m_filePatternList, false);
        m_userFileList = getFiles(m_filePatternList, true);
    }
}


void SubConfig::parseProgramSubConfig(const SubConfigPattern* pattern)
{
    QString program = pattern->program();

    if (pattern->program()[0] != '/') {
        program =  QStandardPaths::findExecutable(pattern->program());
        if (program.isEmpty()) {
            char* path = fcitx_utils_get_fcitx_path_with_filename("bindir", program.toUtf8().constData());
            if (path) {
                program = path;
                free(path);
            }
        }
    }
    else {
        program = pattern->program();
    }
    QFileInfo info(program);
    if (!info.isExecutable())
        program = QString();

    m_progam = program;
}

SubConfig::SubConfig(const QString& name, SubConfigPattern* pattern) :
    m_name(name),
    m_type(pattern->type())
{
    switch (pattern->type()) {
    case SC_ConfigFile:
        parseConfigFileSubConfig(pattern);
        break;
    case SC_NativeFile:
        parseNativeFileSubConfig(pattern);
        break;
    case SC_Program:
        parseProgramSubConfig(pattern);
        break;
    case SC_Plugin:
        parsePluginSubConfig(pattern);
        break;
    default:
        break;
    }
}

SubConfigType SubConfig::type()
{
    return m_type;
}

const QString& SubConfig::name() const
{
    return m_name;
}

const QString& SubConfig::configdesc() const
{
    return m_configdesc;
}

const QString& SubConfig::nativepath() const
{
    return m_nativepath;
}

const QString& SubConfig::program() const
{
    return m_progam;
}

QSet< QString >& SubConfig::fileList()
{
    return m_fileList;
}

QSet< QString >& SubConfig::userFileList()
{
    return m_userFileList;
}

const QString& SubConfig::mimetype() const
{
    return m_mimetype;
}

bool SubConfig::isValid() const
{
    switch (m_type) {
    case SC_ConfigFile:
        return true;
    case SC_NativeFile:
        return true;
    case SC_Program:
        return !m_progam.isEmpty();
    case SC_Plugin:
        return !Global::instance()->testWrapper(m_nativepath).isEmpty();
    default:
        break;
    }
    return false;
}

}
