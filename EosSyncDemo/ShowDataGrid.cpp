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

#include "ShowDataGrid.h"

////////////////////////////////////////////////////////////////////////////////

ShowDataDetails::ShowDataDetails(QWidget *parent)
	: QWidget(parent, Qt::Window)
	, m_TargetType(EosTarget::EOS_TARGET_COUNT)
	, m_Dirty(true)
{
	m_Text = new QTextEdit(this);
	m_Text->setAcceptRichText(false);
	m_Text->setReadOnly(true);
	m_Text->setWordWrapMode(QTextOption::NoWrap);
	m_Text->setTabStopWidth(20);
	QFont fnt("Monospace");
	fnt.setStyleHint(QFont::TypeWriter);
	m_Text->setFont(fnt);
}

////////////////////////////////////////////////////////////////////////////////

void ShowDataDetails::Clear()
{
	m_Text->clear();
}

////////////////////////////////////////////////////////////////////////////////

void ShowDataDetails::SetTargetType(unsigned int targetType)
{
	if(m_TargetType != targetType)
	{
		m_TargetType = targetType;
		EosTarget::EnumEosTargetType type = static_cast<EosTarget::EnumEosTargetType>(m_TargetType);
		setWindowTitle( EosTarget::GetNameForTargetType(type) );
		SetDirty();
	}
}

////////////////////////////////////////////////////////////////////////////////

void ShowDataDetails::Update(const EosSyncData::TARGETLIST_DATA &targetListData)
{
	if( !m_Dirty )
	{
		bool anyTargetListDirty = false;
		for(EosSyncData::TARGETLIST_DATA::const_iterator i=targetListData.begin(); i!=targetListData.end(); i++)
		{
			if( i->second->GetStatus().GetDirty() )
			{
				anyTargetListDirty = true;
				break;
			}
		}

		if( !anyTargetListDirty )
			return;
	}

	QString text;
	QString qStr;
	std::string stdStr;
	for(EosSyncData::TARGETLIST_DATA::const_iterator i=targetListData.begin(); i!=targetListData.end(); i++)
	{
		EosTarget::EnumEosTargetType type = static_cast<EosTarget::EnumEosTargetType>(m_TargetType);

		int listId = i->first;
		if(type==EosTarget::EOS_TARGET_CUE && listId<=0 && targetListData.size()>1)
			continue;

		const EosTargetList *targetList = i->second;
		const EosTargetList::TARGETS &targets = targetList->GetTargets();		

		qStr = EosTarget::GetNameForTargetType(type);
		if(type==EosTarget::EOS_TARGET_CUE && listId>0)
			qStr.append( QString(" list %1").arg(listId) );
		if( !text.isEmpty() )
			text.append("\n");
		text.append(qStr);
		ShowDataGrid::TimestampToStr(targetList->GetStatus().GetTimestamp(), qStr);
		text.append( QString(" (%1)").arg(qStr) );
		text.append("\n\n");

		// for all targets
		for(EosTargetList::TARGETS::const_iterator j=targets.begin(); j!=targets.end(); j++)
		{
			// for all parts
			const EosTarget::sDecimalNumber &targetNumber = j->first;
			const EosTargetList::PARTS &parts = j->second.list;

			for(EosTargetList::PARTS::const_iterator k=parts.begin(); k!=parts.end(); k++)
			{
				// for all property groups
				int partNumber = k->first;
				const EosTarget *target = k->second;
				const EosTarget::PROP_GROUPS &propGroups = target->GetPropGroups();

				EosTarget::GetStringFromNumber(targetNumber, stdStr);
				if( stdStr.c_str() )
					qStr = QString::fromUtf8( stdStr.c_str() );
				else
					qStr.clear();
				if(partNumber > 0)
					qStr.append( QString("/%1").arg(partNumber) );
				text.append( QString("[ %1 ]").arg(qStr) );
				ShowDataGrid::TimestampToStr(target->GetStatus().GetTimestamp(), qStr);
				text.append( QString(" (%1)\n").arg(qStr) );

				for(EosTarget::PROP_GROUPS::const_iterator k=propGroups.begin(); k!=propGroups.end(); k++)
				{
					// for all properties
					const std::string &propGroupName = k->first;
					const EosTarget::PROPS &props = k->second.props;

					bool subGroup = !propGroupName.empty();
					if( subGroup )
					{
						if( propGroupName.c_str() )
							qStr = QString::fromUtf8( propGroupName.c_str() );
						else
							qStr.clear();
						text.append( QString("\t[ %1 ]\n").arg(qStr) );
					}

					qStr.clear();
					for(EosTarget::PROPS::const_iterator l=props.begin(); l!=props.end(); l++)
					{
						const std::string &value = l->value;
						if( !qStr.isEmpty() )
							qStr.append(", ");
						QString strVal;
						if( value.c_str() )
							strVal = QString::fromUtf8( value.c_str() );
						qStr.append( QString("\"%1\"").arg(strVal) );
					}

					if( !qStr.isEmpty() )
					{
						text.append("\t");
						if( subGroup )
							text.append("\t");
						text.append(qStr);
						text.append("\n");
					}
				}
			}
		}
	}

	m_Text->setPlainText(text);

	m_Dirty = false;
}

////////////////////////////////////////////////////////////////////////////////

void ShowDataDetails::resizeEvent(QResizeEvent *e)
{
	m_Text->setGeometry(0, 0, width(), height());
	QWidget::resizeEvent(e);
}

////////////////////////////////////////////////////////////////////////////////

TargetButton::TargetButton(unsigned int targetType, const QString &text, QWidget *parent)
	: QPushButton(text, parent)
	, m_TargetType(targetType)
{
	connect(this, SIGNAL(clicked(bool)), this, SLOT(onClicked(bool)));
}

////////////////////////////////////////////////////////////////////////////////

void TargetButton::onClicked(bool /*checked*/)
{
	emit targetClicked(m_TargetType);
}

////////////////////////////////////////////////////////////////////////////////

ShowDataGrid::ShowDataGrid(QWidget *parent)
	: QWidget(parent)
	, m_Details(0)
{
	QGridLayout *layout = new QGridLayout(this);
	layout->addItem(new QSpacerItem(1,1,QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding), 0, 0);

	for(unsigned int i=0; i<EosTarget::EOS_TARGET_COUNT; i++)
	{
		int row = (i+1);
		sWidgetGroup &group = m_WidgetGroups[i];
		EosTarget::EnumEosTargetType type = static_cast<EosTarget::EnumEosTargetType>(i);
		QString name( EosTarget::GetNameForTargetType(type) );
		name.prepend("/");

		group.label = new QLabel(name, this);
		QPalette labelPal( group.label->palette() );
		labelPal.setColor(QPalette::WindowText, MUTED_COLOR);
		group.label->setPalette(labelPal);
		layout->addWidget(group.label, row, 1);

		group.progress = new QProgressBar(this);
		group.progress->setMinimum(0);
		group.progress->setMaximum(100);
		group.progress->setFixedSize(60, 16);
		group.progress->setValue(0);
		group.progress->setTextVisible(false);
		layout->addWidget(group.progress, row, 2);

		group.count = new QLabel(this);
		group.count->setPalette(labelPal);
		layout->addWidget(group.count, row, 3);

		group.timestamp = new QLabel(this);
		QPalette timestampPal( group.label->palette() );
		timestampPal.setColor(QPalette::WindowText, TIME_COLOR);
		group.timestamp->setPalette(timestampPal);
		layout->addWidget(group.timestamp, row, 4);

		group.button = new TargetButton(i, "+", this);
		QSize buttonSize( group.button->sizeHint() );
		group.button->setFixedSize(buttonSize.height(), buttonSize.height());		
		connect(group.button, SIGNAL(targetClicked(unsigned int)), this, SLOT(onTargetClicked(unsigned int)));
		layout->addWidget(group.button, row, 5);
	}

	layout->addItem(new QSpacerItem(1,1,QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding), EosTarget::EOS_TARGET_COUNT+1, 6);
}

////////////////////////////////////////////////////////////////////////////////

void ShowDataGrid::Update(EosSyncLib &eosSyncLib)
{
	const EosSyncData &syncData = eosSyncLib.GetData();

	bool refresh = syncData.GetStatus().GetDirty();

	if(!refresh && m_Details && m_Details->isVisible() && m_Details->GetDirty())
		refresh = true;	// details window needs refresh

	if( refresh )
	{
		const EosSyncData::SHOW_DATA &showData = syncData.GetShowData();
		for(int i=0; i<EosTarget::EOS_TARGET_COUNT; i++)
		{
			sWidgetGroup &group = m_WidgetGroups[i];
			QPalette labelPal( group.label->palette() );
			QPalette progressPal( group.progress->palette() );
			EosTarget::EnumEosTargetType type = static_cast<EosTarget::EnumEosTargetType>(i);

			bool targetTypeRunning = false;
			bool targetTypeComplete = false;
			bool initialSyncComplete = true;
			size_t initialSyncTotal = 0;
			size_t initialSyncTotalCompleted = 0;
			size_t totalTargets = 0;
			bool gotMostRecentTimestamp = false;
			time_t mostRecentTimestamp = 0;

			EosSyncData::SHOW_DATA::const_iterator j = showData.find(type);
			if(j != showData.end())
			{
				// check status of each list of this target type				
				const EosSyncData::TARGETLIST_DATA &targetListData = j->second;
				for(EosSyncData::TARGETLIST_DATA::const_iterator k=targetListData.begin(); k!=targetListData.end(); k++)
				{
					EosTargetList *targetList = k->second;

					totalTargets += targetList->GetNumTargets();
					const EosTargetList::sInitialSyncInfo &initialSyncInfo = targetList->GetInitialSync();
					if( !initialSyncInfo.complete )
						initialSyncComplete = false;
					initialSyncTotal += initialSyncInfo.count;
					initialSyncTotalCompleted += (initialSyncInfo.complete ? initialSyncInfo.count : targetList->GetNumTargets());

					const EosSyncStatus &status = targetList->GetStatus();
					status.GetTimestamp();
					switch( status.GetValue() )
					{
						case EosSyncStatus::SYNC_STATUS_RUNNING:
							targetTypeRunning = true;
							break;

						case EosSyncStatus::SYNC_STATUS_COMPLETE:
							targetTypeComplete = true;
							break;
					}

					if(status.GetValue() != EosSyncStatus::SYNC_STATUS_UNINTIALIZED)
					{
						if(!gotMostRecentTimestamp || status.GetTimestamp()>mostRecentTimestamp)
							mostRecentTimestamp = status.GetTimestamp();
						gotMostRecentTimestamp = true;
					}
				}

				if(	m_Details &&
					m_Details->isVisible() &&
					m_Details->GetTargetType()==static_cast<unsigned int>(type) )
				{
					m_Details->Update(targetListData);
				}
			}
			else
				initialSyncComplete = false;

			// determine color
			if( targetTypeRunning )
			{
				labelPal.setColor(QPalette::WindowText, WARNING_COLOR);
				progressPal.setColor(QPalette::Highlight, WARNING_COLOR);
			}
			else if( targetTypeComplete )
			{
				labelPal.setColor(QPalette::WindowText, SUCCESS_COLOR);
				progressPal.setColor(QPalette::Highlight, SUCCESS_COLOR);
			}
			else
			{
				labelPal.setColor(QPalette::WindowText, MUTED_COLOR);
				progressPal.setColor(QPalette::Highlight, MUTED_COLOR);
			}

			// label
			group.label->setPalette(labelPal);

			// progress
			double t = 0;
			if( initialSyncComplete )
				t = 1.0;
			else if(initialSyncTotal != 0)
				t = (initialSyncTotalCompleted/static_cast<double>(initialSyncTotal));
			group.progress->setValue( qRound(t*100) );
			group.progress->setPalette(progressPal);

			// count
			group.count->setPalette(labelPal);
			group.count->setText( QString::number(totalTargets) );

			// timestamp
			if( gotMostRecentTimestamp )
			{
				QString str;
				TimestampToStr(mostRecentTimestamp, str);
				group.timestamp->setText(str);
			}
			else
				group.timestamp->setText("-");
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void ShowDataGrid::onTargetClicked(unsigned int targetType)
{
	if( !m_Details )
		m_Details = new ShowDataDetails(this);

	m_Details->SetTargetType(targetType);

	if( !m_Details->isVisible() )
	{
		m_Details->SetDirty();
		m_Details->show();
	}
}

////////////////////////////////////////////////////////////////////////////////

void ShowDataGrid::TimestampToStr(const time_t &timestamp, QString &str)
{
	tm *t = localtime( &timestamp );
	str = QString("%1:%2:%3")
		.arg(t->tm_hour, 2)
		.arg(t->tm_min, 2, 10, QChar('0'))
		.arg(t->tm_sec, 2, 10, QChar('0'));
}

////////////////////////////////////////////////////////////////////////////////
