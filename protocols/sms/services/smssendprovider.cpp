#include "smssendprovider.h"
#include "smsglobal.h"
#include "smscontact.h"

#include <kdeversion.h>
#if KDE_VERSION > 305
#include <kprocio.h>
#else
#include <kprocess.h>
#endif
#include <qregexp.h>
#include <klistview.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>

SMSSendProvider::SMSSendProvider(QString providerName, QString prefixValue, SMSContact* contact, QObject* parent, const char *name)
	: QObject( parent, name )
{

	provider = providerName;
	prefix = prefixValue;
	m_contact = contact;

	optionsLoaded = false;

#if KDE_VERSION > 305
	KProcIO* p = new KProcIO;
	p->setUseShell(true);
#else
	KShellProcess* p = new KShellProcess;
#endif

	*p << QString("%1/bin/smssend").arg(prefix) << provider << "-help";
	connect( p, SIGNAL(processExited(KProcess *)), this, SLOT(slotOptionsFinished(KProcess*)));
	connect( p, SIGNAL(receivedStdout(KProcess*, char*, int)),
		this, SLOT(slotReceivedOutput(KProcess*, char*, int)));
	connect( p, SIGNAL(receivedStderr(KProcess*, char*, int)),
		this, SLOT(slotReceivedOutput(KProcess*, char*, int)));
	output.clear();
	p->start(KProcess::Block);
}

SMSSendProvider::~SMSSendProvider()
{

}

QListViewItem* SMSSendProvider::listItem(KListView* parent, int pos)
{
	while (optionsLoaded == false)
		;

	return new KListViewItem(parent, names[pos], values[pos]);
}

void SMSSendProvider::save(KListView* data)
{
	QListViewItem* p;
	QString group = QString("SMSSend-%1").arg(provider);

	for (int i=0; i < data->childCount(); i++)
	{
		p = data->itemAtIndex(i);
		if (p->text(1) == "")
			SMSGlobal::deleteConfig(group, p->text(0), m_contact);
		else
			SMSGlobal::writeConfig(group, p->text(0), m_contact, p->text(1));
	}
}

void SMSSendProvider::showDescription(QString name)
{
	int pos = names.findIndex(name);
	if (pos > -1)
		KMessageBox::information(0L, descriptions[pos], name);
}

int SMSSendProvider::count()
{
	return names.count();
}

void SMSSendProvider::send(const KopeteMessage& msg)
{
	kdDebug() << "SMSSendProvider::send()" << endl;
	while (optionsLoaded == false)
		;

	m_msg = msg;

	QString message = msg.plainBody();
	QString nr = msg.to().first()->id();

	if (canSend = false)
		return;

	int pos = names.findIndex(QString("Message"));
	values[pos] = message;
	pos = names.findIndex(QString("Tel"));
	values[pos] = nr;

	QString args = "\"" + values.join("\" \"") + "\"";

#if KDE_VERSION > 305
	KProcIO* p = new KProcIO;
	p->setUseShell(true);
#else
	KShellProcess* p = new KShellProcess;
#endif

	*p << QString("%1/bin/smssend").arg(prefix) << provider << args;
	output.clear();
	connect( p, SIGNAL(processExited(KProcess *)), this, SLOT(slotSendFinished(KProcess*)));
	connect( p, SIGNAL(receivedStdout(KProcess*, char*, int)),
		this, SLOT(slotReceivedOutput(KProcess*, char*, int)));
	connect( p, SIGNAL(receivedStderr(KProcess*, char*, int)),
		this, SLOT(slotReceivedOutput(KProcess*, char*, int)));

	p->start(KProcess::Block);
}

void SMSSendProvider::slotSendFinished(KProcess* p)
{
	kdDebug() << "SMSSendProvider::slotSendFinished()" << endl;
	if (p->exitStatus() == 0)
	{
		KMessageBox::information(0L, i18n("Message sent"), output.join("\n"), i18n("Message sent"));

		emit messageSent(m_msg);
	}
	else
	{
		KMessageBox::detailedError(0L, i18n("Something went wrong when sending message"), output.join("\n"),
				i18n("Could not send message"));
	}
}

void SMSSendProvider::slotOptionsFinished(KProcess* p)
{
	bool nameFound = false;
	bool nrFound = false;

	QString n = "  ([^ ]*) ";
	QString valueInfo = ".*"; // Should be changed later to match "(info abut the format)"
	QString valueDesc = "(/\\* )(.*)( \\*/)";

	QRegExp r = n + valueInfo + valueDesc;
	QString group = QString("SMSSend-%1").arg(provider);

	for (unsigned i=0; i < output.count(); i++)
	{
		QString tmp = output[i];

		int pos = r.search(tmp);
		if (pos > -1)
		{
			names.append(r.cap(1));
			
			if (r.cap(1) == "Message")
				nameFound = true;
			if (r.cap(1) == "Tel")
				nrFound = true;

			descriptions.append(r.cap(3));
			rules.append("");
			values.append(SMSGlobal::readConfig(group, r.cap(1), m_contact));
		}
	}

	optionsLoaded = true;

	if ( !nameFound )
	{
		canSend = false;
		KMessageBox::error(0L, i18n("Could not determine which argument which should contain the message"),
			i18n("Could not send message"));
		return;
	}
	if ( !nrFound )
	{
		canSend = false;

		KMessageBox::error(0L, i18n("Could not determine which argument which should contain the number"),
			i18n("Could not send message"));
		return;
	}

	canSend = true;
	kdDebug() << "SMSSendProvider::slotOptionsFinished()" << endl;

	delete p;
}

void SMSSendProvider::slotReceivedOutput(KProcess*, char  *buffer, int  buflen)
{
	kdDebug() << "SMSSendProvider::slotReceivedOutput()" << endl;
	QStringList lines = QStringList::split("\n", QString::fromLocal8Bit(buffer, buflen));
	for (QStringList::Iterator it = lines.begin(); it != lines.end(); ++it)
		output.append(*it);
}

#include "smssendprovider.moc"
/*
 * Local variables:
 * c-indentation-style: k&r
 * c-basic-offset: 8
 * indent-tabs-mode: t
 * End:
 */
// vim: set noet ts=4 sts=4 sw=4:

