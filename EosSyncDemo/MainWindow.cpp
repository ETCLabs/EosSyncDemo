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

#include "MainWindow.h"
#include "EosSyncLib.h"
#include "EosTimer.h"
#include "ShowDataGrid.h"
#include "EosTcp.h"
#include <time.h>

#ifdef WIN32
	#include <windows.h>
	#include "resource.h"
#endif

////////////////////////////////////////////////////////////////////////////////

#define APP_VERSION			"0.3"
#define SETTING_IP			"IP"
#define SETTING_PORT		"Port"
#define SETTING_SEND_TEXT	"SendText"
#define SETTING_LOG_DEPTH	"LogDepth"

////////////////////////////////////////////////////////////////////////////////

EosSyncLibThread::EosSyncLibThread()
	: m_Port(0)
	, m_Run(false)
{
}

////////////////////////////////////////////////////////////////////////////////

EosSyncLibThread::~EosSyncLibThread()
{
	Stop();
}

////////////////////////////////////////////////////////////////////////////////

void EosSyncLibThread::Start(const QString &ip, unsigned short port)
{
	Stop();

	m_Ip = ip;
	m_Port = port;
	m_Run = true;
	start();
}

////////////////////////////////////////////////////////////////////////////////

void EosSyncLibThread::Stop()
{
	m_Run = false;
	wait();
}

////////////////////////////////////////////////////////////////////////////////

EosSyncLib* EosSyncLibThread::LockEosSyncLib()
{
	m_Mutex.lock();
	return &m_EosSyncLib;
}

////////////////////////////////////////////////////////////////////////////////

void EosSyncLibThread::UnlockEosSyncLib()
{
	m_Mutex.unlock();
}

////////////////////////////////////////////////////////////////////////////////

void EosSyncLibThread::SendOscString(const std::string &str)
{
	OSCPacketWriter *packet = OSCPacketWriter::CreatePacketWriterForString( str.c_str() );
	if( packet )
	{
		m_Mutex.lock();
		m_EosSyncLib.Send(*packet, /*immediate*/true);
		m_Mutex.unlock();
		
		delete packet;
	}
}

////////////////////////////////////////////////////////////////////////////////

void EosSyncLibThread::run()
{
	// initialize
	m_Mutex.lock();
	if( !m_EosSyncLib.Initialize(m_Ip.toAscii().constData(), m_Port) )
		m_Run = false;
	m_Mutex.unlock();

	// run
	while( m_Run )
	{
		m_Mutex.lock();
		m_EosSyncLib.Tick();
		if( !m_EosSyncLib.IsRunning() )
			m_Run = false;
		m_Mutex.unlock();
		EosTimer::SleepMS(10);
	}

	// destroy
	m_Mutex.lock();
	m_EosSyncLib.Shutdown();
	m_Mutex.unlock();
}

////////////////////////////////////////////////////////////////////////////////

MainWindow::MainWindow(QWidget* parent/*=0*/, Qt::WindowFlags f/*=0*/)
	: QWidget(parent, f)
	, m_EosSyncLibThread(0)
	, m_Settings("ETC", "EosSyncDemo")
	, m_LogDepth(200)
	, m_LogFile("EosSyncDemo.XXXXXX.log.txt")
{
#ifdef WIN32
	HICON hIcon = static_cast<HICON>( LoadImage(GetModuleHandle(0),MAKEINTRESOURCE(IDI_ICON1),IMAGE_ICON,128,128,LR_LOADTRANSPARENT) );
	if( hIcon )
	{
		setWindowIcon( QIcon(QPixmap::fromWinHICON(hIcon)) );
		DestroyIcon(hIcon);
	}
#endif

	m_LogFile.setFileName( QDir(QDir::tempPath()).absoluteFilePath("EosSyncDemoLog.txt") );
	if( m_LogFile.open(QIODevice::WriteOnly|QIODevice::Truncate|QIODevice::Text) )
	{
		m_LogStream.setDevice( &m_LogFile );
		m_LogStream.setCodec("UTF-8");
		m_LogStream.setGenerateByteOrderMark(true);
	}

	m_EosSyncLibThread = new EosSyncLibThread();

	QGridLayout *layout = new QGridLayout(this);

	int row = 0;

	layout->addWidget(new QLabel("IP",this), row, 0);

	QString ip;
	GetDefaultIP(ip);
	ip = m_Settings.value(SETTING_IP,ip).toString();

	m_Ip = new QLineEdit(ip, this);
	layout->addWidget(m_Ip, row, 1);

	layout->addWidget(new QLabel("Port",this), row, 2);

	m_Port = new QSpinBox(this);
	m_Port->setMinimum(0);
	m_Port->setMaximum(0xffff);
	m_Port->setValue( m_Settings.value(SETTING_PORT,EosSyncLib::DEFAULT_PORT).toUInt() );
	layout->addWidget(m_Port, row, 3);

	m_StartStopButton = new QPushButton(this);
	connect(m_StartStopButton, SIGNAL(clicked(bool)), this, SLOT(onStartStopClicked(bool)));
	layout->addWidget(m_StartStopButton, row, 4);

	row++;
	
	QSplitter *splitter = new QSplitter(this);
	layout->addWidget(splitter, row, 0, 1, 5);

	QScrollArea *scrollArea = new QScrollArea(splitter);
	scrollArea->setWidgetResizable(true);
	splitter->addWidget(scrollArea);
	
	m_ShowDataGrid = new ShowDataGrid(scrollArea);
	scrollArea->setWidget(m_ShowDataGrid);
	
	QWidget *logBase = new QWidget(splitter);
	QGridLayout *logLayout = new QGridLayout(logBase);
	splitter->addWidget(logBase);
	
	m_LogDepth = m_Settings.value(SETTING_LOG_DEPTH, m_LogDepth).toInt();
	if(m_LogDepth < 1)
		m_LogDepth = 1;
	m_Settings.setValue(SETTING_LOG_DEPTH, m_LogDepth);

	m_Log = new QListWidget(logBase);
	QPalette logPal( m_Log->palette() );
	logPal.setColor(QPalette::Base, BG_COLOR);
	m_Log->setPalette(logPal);
	m_Log->setSelectionMode(QAbstractItemView::NoSelection);
	m_Log->setMovement(QListView::Static);
	QFont fnt("Monospace");
	fnt.setStyleHint(QFont::TypeWriter);
	m_Log->setFont(fnt);
	logLayout->addWidget(m_Log, 0, 0, 1, 2);

	QPushButton *button = new QPushButton("Clear Log", logBase);
	connect(button, SIGNAL(clicked(bool)), this, SLOT(onClearLogClicked(bool)));
	logLayout->addWidget(button, 1, 0);

	button = new QPushButton("Open Log", logBase);
	button->setEnabled( m_LogFile.isOpen() );
	connect(button, SIGNAL(clicked(bool)), this, SLOT(onOpenLogClicked(bool)));
	logLayout->addWidget(button, 1, 1);
	
	row++;
	
	QString sendText = m_Settings.value(SETTING_SEND_TEXT,"/eos/sub/1=0.75").toString();
	m_SendText = new QLineEdit(sendText, this);
	connect(m_SendText, SIGNAL(returnPressed()), this, SLOT(onSendReturnPressed()));
	layout->addWidget(m_SendText, row, 0, 1, 2);
	
	m_SendButton = new QPushButton("Send", this);
	connect(m_SendButton, SIGNAL(clicked(bool)), this, SLOT(onSendClicked(bool)));
	layout->addWidget(m_SendButton, row, 2, 1, 3);

	m_EosSyncLibThreadTimer = new QTimer(this);
	connect(m_EosSyncLibThreadTimer, SIGNAL(timeout()), this, SLOT(onTick()));	

	AddLogInfo( QString("Version %1").arg(APP_VERSION) );
	m_StartStopButton->setFocus();
	UpdateUI();
}

////////////////////////////////////////////////////////////////////////////////

MainWindow::~MainWindow()
{
	if( m_LogFile.isOpen() )
	{
		m_LogStream.flush();
		m_LogFile.close();
	}

	if( m_EosSyncLibThread )
	{
		delete m_EosSyncLibThread;
		m_EosSyncLibThread = 0;
	}
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::UpdateUI()
{
	bool running = m_EosSyncLibThread->isRunning();
	m_StartStopButton->setText(running ? "Disconnect" : "Sync");
	m_StartStopButton->setPalette( palette() );
	m_Ip->setEnabled( !running );
	m_Port->setEnabled( !running );
	m_SendText->setEnabled(false);
	m_SendButton->setEnabled(false);
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::AddLogInfo(const QString &text)
{
	EosLog::sLogMsg logMsg;
	logMsg.type = EosLog::LOG_MSG_TYPE_INFO;
	logMsg.timestamp = time(0);
	logMsg.text = text.toStdString();

	EosLog::LOG_Q logQ;
	logQ.push_back(logMsg);
	AddLogQ(logQ);
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::AddLogQ(EosLog::LOG_Q &logQ)
{
	if( !logQ.empty() )
	{
		m_Log->setUpdatesEnabled(false);

		QScrollBar *vs = m_Log->verticalScrollBar();
		bool dontAutoScroll = (vs && vs->isEnabled() && vs->isVisible() && vs->value()<vs->maximum());

		for(EosLog::LOG_Q::const_iterator i=logQ.begin(); i!=logQ.end(); i++)
		{
			const EosLog::sLogMsg logMsg = *i;

			tm *t = localtime( &logMsg.timestamp );

			QString msgText;
			if( logMsg.text.c_str() )
				msgText = QString::fromUtf8( logMsg.text.c_str() );

			QString itemText = QString("[ %1:%2:%3 ]  %4")
				.arg(t->tm_hour, 2)
				.arg(t->tm_min, 2, 10, QChar('0'))
				.arg(t->tm_sec, 2, 10, QChar('0'))
				.arg( msgText );

			if( m_LogFile.isOpen() )
			{
				m_LogStream << itemText;
				m_LogStream << "\n";
			}

			QListWidgetItem *item = new QListWidgetItem(itemText);
			switch( logMsg.type )
			{
				case EosLog::LOG_MSG_TYPE_DEBUG:
					item->setForeground(MUTED_COLOR);
					break;

				case EosLog::LOG_MSG_TYPE_WARNING:
					item->setForeground(WARNING_COLOR);
					break;

				case EosLog::LOG_MSG_TYPE_ERROR:
					item->setForeground(ERROR_COLOR);
					break;
			}

			m_Log->addItem(item);

			while(m_Log->count() > m_LogDepth)
			{
				QListWidgetItem *item = m_Log->takeItem(0);
				if( item )
					delete item;
			}
		}		

		m_Log->setUpdatesEnabled(true);
	
		if(	!dontAutoScroll )
			m_Log->setCurrentRow(m_Log->count() - 1);
	}
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::GetDefaultIP(QString &ip)
{
	QHostAddress localHostAddr(QHostAddress::LocalHost);

	QList<QNetworkInterface> nics = QNetworkInterface::allInterfaces();
	for(QList<QNetworkInterface>::const_iterator i=nics.begin(); i!=nics.end(); i++)
	{
		const QNetworkInterface &net = *i;
		if( net.isValid() &&
			net.flags().testFlag(QNetworkInterface::IsUp) &&
			net.flags().testFlag(QNetworkInterface::IsRunning) )
		{
			QList<QNetworkAddressEntry>	addrs = net.addressEntries();
			for(QList<QNetworkAddressEntry>::const_iterator j=addrs.begin(); j!=addrs.end(); j++)
			{
				QHostAddress addr = j->ip();
				if(	!addr.isNull() &&
					addr.protocol()==QAbstractSocket::IPv4Protocol &&
					addr!=localHostAddr )
				{
					ip = addr.toString();
					return;
				}
			}
		}
	}
	
	ip = localHostAddr.toString();
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::SendText()
{
	if(m_EosSyncLibThread && m_EosSyncLibThread->isRunning())
	{
		QString str( m_SendText->text() );
		m_Settings.setValue(SETTING_SEND_TEXT, str);
		if( !str.isEmpty() )
			m_EosSyncLibThread->SendOscString( str.toStdString() );
	}
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onTick()
{
	EosSyncLib *eosSyncLib = m_EosSyncLibThread->LockEosSyncLib();
	if( eosSyncLib )
	{
		// update UI
		if( eosSyncLib->IsConnected() )
		{
			m_SendText->setEnabled(true);
			m_SendButton->setEnabled(true);
		}
		m_ShowDataGrid->Update( *eosSyncLib );
		const EosSyncStatus &status = eosSyncLib->GetData().GetStatus();
		if( status.GetDirty() )
		{
			QPalette pal( palette() );
			pal.setColor(QPalette::ButtonText, Qt::white);
			pal.setColor(QPalette::Button, (status.GetValue()==EosSyncStatus::SYNC_STATUS_COMPLETE) ? SUCCESS_COLOR : ERROR_COLOR);
			m_StartStopButton->setPalette(pal);
		}
		eosSyncLib->ClearDirty();

		// flush log messages
		EosLog::LOG_Q logQ;
		eosSyncLib->GetLog().Flush(logQ);
		AddLogQ(logQ);

		m_EosSyncLibThread->UnlockEosSyncLib();
	}

	if( !m_EosSyncLibThread->isRunning() )
	{
		m_EosSyncLibThreadTimer->stop();
		UpdateUI();
	}
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onStartStopClicked(bool /*checked*/)
{
	if( m_EosSyncLibThread->isRunning() )
	{
		m_EosSyncLibThreadTimer->stop();
		m_EosSyncLibThread->Stop();
	}
	else
	{
		QString ip( m_Ip->text() );
		unsigned short port = static_cast<unsigned short>( m_Port->value() );
		m_Settings.setValue(SETTING_IP, ip);
		m_Settings.setValue(SETTING_PORT, port);

		m_EosSyncLibThread = new EosSyncLibThread();
		m_EosSyncLibThread->Start(ip, port);
		m_EosSyncLibThreadTimer->start(60);
	}

	UpdateUI();
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onClearLogClicked(bool /*checked*/)
{
	m_Log->clear();
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onOpenLogClicked(bool /*checked*/)
{
	if( m_LogFile.isOpen() )
		m_LogStream.flush();
	QDesktopServices::openUrl( QUrl::fromLocalFile(m_LogFile.fileName()) );
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onSendClicked(bool /*checked*/)
{
	SendText();
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::onSendReturnPressed()
{
	SendText();
}

////////////////////////////////////////////////////////////////////////////////
