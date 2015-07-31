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
#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#ifndef EOS_SYNC_LIB_H
#include "EosSyncLib.h"
#endif

#ifndef QT_INCLUDE_H
#include "QtInclude.h"
#endif

class ShowDataGrid;

////////////////////////////////////////////////////////////////////////////////

class EosSyncLibThread
	: public QThread
{
public:
	EosSyncLibThread();
	virtual ~EosSyncLibThread();

	virtual void Start(const QString &ip, unsigned short port);
	virtual void Stop();
	virtual EosSyncLib* LockEosSyncLib();
	virtual void UnlockEosSyncLib();
	virtual void SendOscString(const std::string &str);

protected:
	QString			m_Ip;
	unsigned short	m_Port;
	bool			m_Run;
	EosSyncLib		m_EosSyncLib;
	QMutex			m_Mutex;

	virtual void run();
};

////////////////////////////////////////////////////////////////////////////////

class MainWindow
	: public QWidget
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent=0, Qt::WindowFlags f=0);
	virtual ~MainWindow();
	
	virtual QSize sizeHint() const {return QSize(640,480);}
	virtual void AddLogInfo(const QString &text);
	virtual void AddLogQ(EosLog::LOG_Q &logQ);

private slots:
	void onTick();
	void onStartStopClicked(bool checked);
	void onClearLogClicked(bool checked);
	void onOpenLogClicked(bool checked);
	void onSendClicked(bool checked);
	void onSendReturnPressed();

private:
	QLineEdit			*m_Ip;
	QSpinBox			*m_Port;
	QPushButton			*m_StartStopButton;
	ShowDataGrid		*m_ShowDataGrid;
	QListWidget			*m_Log;
	QLineEdit			*m_SendText;
	QPushButton			*m_SendButton;
	EosSyncLibThread	*m_EosSyncLibThread;
	QTimer				*m_EosSyncLibThreadTimer;
	QSettings			m_Settings;
	int					m_LogDepth;
	QFile				m_LogFile;
	QTextStream			m_LogStream;

	virtual void UpdateUI();
	virtual void SendText();

	static void GetDefaultIP(QString &ip);
};

////////////////////////////////////////////////////////////////////////////////

#endif
