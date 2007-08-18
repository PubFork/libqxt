/****************************************************************************
 **
 ** Copyright (C) Qxt Foundation. Some rights reserved.
 **
 ** This file is part of the QxtCore module of the Qt eXTension library
 **
 ** This library is free software; you can redistribute it and/or
 ** modify it under the terms of the GNU Lesser General Public
 ** License as published by the Free Software Foundation; either
 ** version 2.1 of the License, or any later version.
 **
 ** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 ** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 **
 ** There is aditional information in the LICENSE file of libqxt.
 ** If you did not receive a copy of the file try to download it or
 ** contact the libqxt Management
 ** 
 ** <http://libqxt.sourceforge.net>  <aep@exys.org>  <coda@bobandgeorge.com>
 **
 ****************************************************************************/

#include "qxtsqlpackage.h"
#include <QBuffer>
#include <QDataStream>

QxtSqlPackage::QxtSqlPackage(QObject *parent) : QObject(parent) 
{
    record = -1;
}

QxtSqlPackage::QxtSqlPackage(const QxtSqlPackage & other, QObject *parent) : QObject(parent)
{
    record = -1;

    setData(other.data());
}

bool QxtSqlPackage::isValid()
{
    if ((record >= 0) && (record < map.count()))
        return true;
    else
        return false;
}

int QxtSqlPackage::at()
{
    return record;
}

bool QxtSqlPackage::next()
{
    record++;
    if (record > (map.count()-1))
    {
        last();
        return false;
    }

    return true;
}

bool QxtSqlPackage::last()
{
    record=map.count()-1;
    if (record >= 0)
        return true;
    else
        return false;
}

bool QxtSqlPackage::first()
{
    if (map.count()) {
        record=0;
        return true;
    } else {
        record=-1;
        return false;
    }
}

QString QxtSqlPackage::value(const QString& key)
{
    if ((record<0) || !map.count()) return QString();

    return map.at(record).value(key);
}



void QxtSqlPackage::insert(QSqlQuery query)
{
    map.clear();
    record=-1;

    /*query will be invalid if next is not called first*/
    if(!query.isValid())
        query.next();

    QSqlRecord infoRecord = query.record();
    int iNumCols = infoRecord.count();
    QVector<QString> tableMap = QVector<QString>(iNumCols);

    /*first create a map of index->colname pairs*/
    for(int iLoop = 0; iLoop < iNumCols; iLoop++) {
        tableMap[iLoop] = infoRecord.fieldName(iLoop);
    }

    /*now use this created map to get column names
     *this should be faster than querying the QSqlRecord every time
     *but that depends on the databasetype and size of the table (number of rows and cols)
     */
    do {
        QHash<QString,QString> hash;
        for(int iColLoop = 0; iColLoop < iNumCols; iColLoop++) {
            hash[tableMap[iColLoop]] = query.value(iColLoop).toString();
        }
        map.append(hash);

    } while(query.next());
}


int QxtSqlPackage::count() const
{
    return map.count();
}


QByteArray QxtSqlPackage::data() const
{
    QBuffer buff;
    buff.open(QBuffer::WriteOnly);
    QDataStream stream(&buff);

    stream<<count();
    for (int i=0; i < count();i++)
        stream << map.at(i);

    buff.close();
    return buff.data();
}

void QxtSqlPackage::setData(const QByteArray& data)
{
    map.clear();
    record=-1;

    QBuffer buff;
    buff.setData(data);
    buff.open(QBuffer::ReadOnly);
    QDataStream stream(&buff);

    int c;
    stream >> c;

    for (int i=0; i<c;i++) {
        QHash<QString,QString> hash;
        stream >> hash;
        map.append(hash);
    }
}

QHash<QString,QString> QxtSqlPackage::hash(int index)
{
    if (index > count()) return QHash<QString,QString>();
    return map.at(index);
}


QHash<QString,QString> QxtSqlPackage::hash()
{
    qDebug()<<record;
    return map.at(record);
}


QxtSqlPackage& QxtSqlPackage::operator= ( const QxtSqlPackage & other ) 
{
    setData(other.data());
    return *this;
}
