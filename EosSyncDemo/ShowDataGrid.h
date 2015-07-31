// Copyright (c) 2015 Electronic Theatre Controls, Inc., http://www.etcconnect.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once
#ifndef SHOW_DATA_GRID_H
#define SHOW_DATA_GRID_H

#ifndef EOS_SYNC_LIB_H
#include "EosSyncLib.h"
#endif

#ifndef QT_INCLUDE_H
#include "QtInclude.h"
#endif

#include <time.h>

////////////////////////////////////////////////////////////////////////////////

class ShowDataDetails
	: public QWidget
{
public:
	ShowDataDetails(QWidget *parent);

	virtual void Clear();
	virtual bool GetDirty() const {return m_Dirty;}
	virtual void SetDirty() {Clear(); m_Dirty=true;}
	virtual unsigned int GetTargetType() const {return m_TargetType;}
	virtual void SetTargetType(unsigned int targetType);
	virtual void Update(const EosSyncData::TARGETLIST_DATA &targetListData);

	virtual QSize sizeHint() const {return QSize(600,480);}

protected:
	QTextEdit		*m_Text;
	bool			m_Dirty;
	unsigned int	m_TargetType;

	virtual void resizeEvent(QResizeEvent *e);
};

////////////////////////////////////////////////////////////////////////////////

class TargetButton
	: public QPushButton
{
	Q_OBJECT

public:
	TargetButton(unsigned int targetType, const QString &text, QWidget *parent);

signals:
	void targetClicked(unsigned int targetType);

private slots:
	void onClicked(bool checked);

protected:
	unsigned int	m_TargetType;
};

////////////////////////////////////////////////////////////////////////////////

class ShowDataGrid
	: public QWidget
{
	Q_OBJECT

public:
	ShowDataGrid(QWidget *parent);

	virtual void Update(EosSyncLib &eosSyncLib);

	static void TimestampToStr(const time_t &timestamp, QString &str);

private slots:
	void onTargetClicked(unsigned int targetType);

protected:
	struct sWidgetGroup
	{
		QLabel			*label;
		QProgressBar	*progress;
		QLabel			*count;
		QLabel			*timestamp;
		TargetButton	*button;
	};

	sWidgetGroup	m_WidgetGroups[EosTarget::EOS_TARGET_COUNT];
	ShowDataDetails	*m_Details;
};

////////////////////////////////////////////////////////////////////////////////

#endif
