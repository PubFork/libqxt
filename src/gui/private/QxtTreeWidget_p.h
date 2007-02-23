/*******************************************************************
Qt extended Library 
Copyright (C) 2007 J-P Nurmi <jpnurmi@gmail.com>
released under the Terms of LGPL (see the LICENSE file)
*******************************************************************/

#ifndef QXTTREEWIDGET_P_H
#define QXTTREEWIDGET_P_H

#include <QxtTreeWidget.h>
#include <QxtCore/QxtPimpl>

class QxtItemDelegate;

class QxtTreeWidgetPrivate : public QObject, public QxtPrivate<QxtTreeWidget>
{
	Q_OBJECT
public:
	QXT_DECLARE_PUBLIC(QxtTreeWidget);
	QxtTreeWidgetPrivate();
	QxtItemDelegate* delegate() const;
	bool branches;
	
private slots:
	void informStartEditing(const QModelIndex& index);
	void informFinishEditing(const QModelIndex& index);
	void expandCollapse(QTreeWidgetItem* item);
};

#endif // QXTTREEWIDGET_P_H
