/*
 * Copyright (c) 2015-2018 Andre de Oliveira <deoliveirambx@googlemail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>
#include <QRegExp>

#include "brokerd.h"
#include "engine.h"

struct payload {
	QList<QTcpSocket *>	sessions;
	QVariant		value;
};

struct brokerd_engine_private {
	QTcpServer			*server;
	QList<QTcpSocket *>		 conns;
	QHash<QString, struct payload>	 data;
	QHash<QString, QTcpSocket *>	 sessions;
	QString				 filename;

	void	pub(char *, QTcpSocket *);
	void	sub(char *, QTcpSocket *);
	void	set(char *, QTcpSocket *);
	void	get(char *, QTcpSocket *);
	void	del(char *, QTcpSocket *);
	void	inc(char *, QTcpSocket *);
	void	add(char *, QTcpSocket *);
	void	exists(char *, QTcpSocket *);
	void	save(const char *, QTcpSocket *);
	void	list(const char *, QTcpSocket *);
	void	reset(QTcpSocket *);
};

brokerd_engine::brokerd_engine() :
    QObject(),
    d(new brokerd_engine_private)
{
	d->server = new QTcpServer;
	connect(d->server, SIGNAL(newConnection()), SLOT(accept()),
	    Qt::QueuedConnection);
}

brokerd_engine::~brokerd_engine()
{
	delete d->server;
	delete d;
}

void
brokerd_engine::unload()
{
	QList<QTcpSocket *>	tmpsks;

	foreach (const QString &k, d->data.keys()) {
		foreach (QTcpSocket *se, d->data[k].sessions) {
			if (!tmpsks.size() || !tmpsks.contains(se))
				tmpsks.append(se);
		}
		d->data[k].sessions.clear();
		d->data[k].value.clear();
	}

	foreach (QTcpSocket *se, tmpsks) {
		se->write("unload\n");
		se->close();
		if (d->conns.contains(se))
			d->conns.removeOne(se);
		se->deleteLater();
	}

	foreach (QTcpSocket *c, d->conns) {
		c->flush();
		c->close();
		c->deleteLater();
	}

	d->data.clear();
	d->conns.clear();
}

void
brokerd_engine::load(const QString &filename)
{
	char		line[1024], *key, *value;
	QFile		file(filename);

	if (filename.isEmpty())
		return;

	if (!file.open(QIODevice::ReadOnly))
		log_fatal("failed to open file: %s",
		    filename.toAscii().data());

	while (!file.atEnd()) {
		if (file.readLine(line, sizeof(line)) == -1)
			continue;
		line[strcspn(line, " \t\r\n")] = '\0';

		key = line;
		if (line[0] == '#' || (value = strchr(key, '=')) == NULL)
			continue;

		*value++ = '\0';
		value += strspn(value, " \t\r\n");
		d->data[key].value = value;
	}
	file.close();
	d->filename = filename;
}

int
brokerd_engine::listen(const QHostAddress &addr, int port)
{
	return d->server->listen(addr, port);
}

void
brokerd_engine::accept()
{
	QTcpSocket *s = d->server->nextPendingConnection();

	connect(s, SIGNAL(readyRead()), SLOT(read()), Qt::QueuedConnection);
	connect(s, SIGNAL(disconnected()), SLOT(close()),
	    Qt::QueuedConnection);

	if (!d->conns.size() || !d->conns.contains(s))
		d->conns.append(s);
}

void
brokerd_engine::close()
{
	QTcpSocket					*s;
	QHash<QString, struct payload>::iterator	 it;
	QString						 key;

	s = (QTcpSocket *)sender();
	for (it = d->data.begin(); it != d->data.end(); ++it) {
		if (it->sessions.contains(s))
			it->sessions.removeOne(s);
	}

	if (d->conns.contains(s))
		d->conns.removeOne(s);

	if (d->sessions.values().contains(s)) {
		key = d->sessions.key(s);
		d->sessions.remove(key);
	}

	s->deleteLater();
}

void
brokerd_engine::read()
{
	QTcpSocket	*s = (QTcpSocket *)sender();
	char		 line[1024], *method, *args;

	while (s->canReadLine() && s->readLine(line, sizeof(line))) {
		method = line;
		line[strcspn(line, "\r\n")] = '\0';

		if ((args = strchr(method, ' ')) == NULL &&
		    strcmp(method, "list") < 0) {
			s->write("invalid format\n");
			s->flush();
			return;
		}

		if (args) {
			*args++ = '\0';
			args += strspn(args, " \t\r\n=");
		}

		if (!strcmp(method, "pub")) {
			d->pub(args, s);
		} else if (!strcmp(method, "sub")) {
			d->sub(args, s);
		} else if (!strcmp(method, "set")) {
			d->set(args, s);
		} else if (!strcmp(method, "get")) {
			d->get(args, s);
		} else if (!strcmp(method, "del")) {
			d->del(args, s);
		} else if (!strcmp(method, "inc")) {
			d->inc(args, s);
		} else if (!strcmp(method, "add")) {
			d->add(args, s);
		} else if (!strcmp(method, "exists")) {
			d->exists(args, s);
		} else if (!strcmp(method, "list")) {
			d->list(args, s);
		} else if (!strcmp(method, "reset")) {
			if (args) {
				s->write("invalid format\n");
				s->flush();
				continue;
			}
			d->reset(s);
		} else if (!strcmp(method, "save")) {
			d->save(args, s);
		} else {
			s->write("invalid method\n");
			s->flush();
		}
	}
}

void
brokerd_engine_private::pub(char *args, QTcpSocket *sk)
{
	char		*key, *value;

	key = args;
	if ((value = strchr(key, '=')) == NULL) {
		sk->write("invalid format\n");
		sk->flush();
		return;
	}
	*value++ = '\0';
	value += strspn(value, " \t\r\n=");

	if (!data.contains(key)) {
		sk->write(key);
		sk->write(": no such entry\n");
		sk->flush();
		return;
	}

	data[key].value = value;
	sk->write("OK\n");
	sk->flush();

	if (data[key].sessions.size() > 0) {
		foreach (QTcpSocket *se, data[key].sessions) {
			se->write(key);
			se->write("=");
			se->write(value);
			se->write("\n");
			se->flush();
		}
	}
}

void
brokerd_engine_private::sub(char *args, QTcpSocket *sk)
{
	QString		key(args);
	int		found = 0;
	QList<QString>	tmpkeys;
	QRegExp		re;

	key = key.trimmed();
	re.setPattern(key);
	if (re.isValid()) {
		foreach (const QString &k, data.keys()) {
			if (k.contains(re)) {
				tmpkeys.append(k);
				data[k].sessions.append(sk);
				found = 1;
			}
		}
		if (found) {
			qSort(tmpkeys.begin(), tmpkeys.end());
			foreach (const QString &k, tmpkeys) {
				sk->write(k.toAscii().data());
				sk->write("=");
				sk->write(data[k].value.toByteArray().data());
				sk->write("\n");
			}
			sessions[key] = sk;
			sk->flush();
			return;
		}
	}

	if (!data.contains(key)) {
		sk->write(key.toAscii().data());
		sk->write(": no such entry\n");
		sk->flush();
		return;
	}

	sk->write(key.toAscii().data());
	sk->write(data[key].value.toByteArray().data());
	sk->write("\n");
	sk->flush();
	data[key].sessions.append(sk);
	sessions[key] = sk;
}

void
brokerd_engine_private::inc(char *args, QTcpSocket *sk)
{
	QString		key(args);
	long long	val;
	bool		ok = false;

	key = key.trimmed();
	if (!data.contains(key)) {
		sk->write(QString("%1: no such entry\n")
		    .arg(key).toAscii().data());
		sk->flush();
		return;
	}

	val = data[key].value.toLongLong(&ok);
	if (!ok) {
		sk->write("no number format\n");
		sk->flush();
		return;
	}

	data[key].value = ++val;
	if (data[key].sessions.size() > 0) {
		foreach (QTcpSocket *se, data[key].sessions) {
			se->write(key.toAscii().data());
			se->write("=");
			se->write(data[key].value.toByteArray().data());
			se->write("\n");
		}
	}

	sk->write(key.toAscii().data());
	sk->write("=");
	sk->write(data[key].value.toString().toAscii().data());
	sk->write("\n");
	sk->flush();

	return;
}

void
brokerd_engine_private::add(char *args, QTcpSocket *sk)
{
	long long	tmpval, amount;
	bool		ok = false;
	char		*key, *value;

	key = args;
	if ((value = strchr(key, ' ')) == NULL) {
		sk->write("invalid format\n");
		sk->flush();
		return;
	}
	*value++ = '\0';
	value += strspn(value, " \t\r\n=");

	if (!data.contains(key)) {
		sk->write(key);
		sk->write(": no such entry\n");
		sk->flush();
		return;
	}

	tmpval = data[key].value.toLongLong(&ok);
	if (!ok) {
		sk->write("no number format\n");
		sk->flush();
		return;
	}

	ok = false;
	amount = QString(value).toLongLong(&ok);
	if (!ok) {
		sk->write("no number format for given amount\n");
		sk->flush();
		return;
	}

	data[key].value = (tmpval + amount);
	if (data[key].sessions.size() > 0) {
		foreach (QTcpSocket *se, data[key].sessions) {
			se->write(key);
			se->write("=");
			se->write(data[key].value.toByteArray().data());
			se->write("\n");
		}
	}

	sk->write(key);
	sk->write("=");
	sk->write(data[key].value.toString().toAscii().data());
	sk->write("\n");
	sk->flush();

	return;
}

void
brokerd_engine_private::exists(char *args, QTcpSocket *sk)
{
	QString		key(args);

	key = key.trimmed();
	if (!data.contains(key)) {
		sk->write(QString("%1: no such entry\n")
		    .arg(key).toAscii().data());
		sk->flush();
		return;
	}
	sk->write("OK\n");
	sk->flush();
	return;
}

void
brokerd_engine_private::set(char *args, QTcpSocket *sk)
{
	QTcpSocket	*sock;
	QString		 ks;
	char		*key, *value;

	key = args;
	if ((value = strchr(key, '=')) == NULL) {
		sk->write("invalid format\n");
		sk->flush();
		return;
	}
	*value++ = '\0';
	value += strspn(value, " \t\r\n");
	data[key].value = value;
	ks = key;

	/* look for existent subscribers sessions */
	foreach (const QString &kre, sessions.keys()) {
		if (ks.contains(QRegExp(kre))) {
			sock = sessions[kre];
			if (!data[ks].sessions.contains(sock))
				data[ks].sessions.append(sock);
		}
	}
	sk->write("OK\n");
	sk->flush();
}

void
brokerd_engine_private::get(char *args, QTcpSocket *sk)
{
	QString		key(args);
	QList<QString>	tmpkeys;
	QRegExp		re;
	int		found = 0;

	/*
	 * first process a regexp that will match multiple keys
	 */
	key = key.trimmed();
	re.setPattern(key);
	if (re.isValid()) {
		foreach (const QString &k, data.keys()) {
			if (k.contains(re)) {
				tmpkeys.append(k);
				found = 1;
			}
		}
		if (found) {
			if (tmpkeys.size() > 1)
				qSort(tmpkeys.begin(), tmpkeys.end());
			foreach (const QString &k, tmpkeys) {
				sk->write(k.toAscii().data());
				sk->write("=");
				sk->write(data[k].value.toByteArray().data());
				sk->write("\n");
			}
			sk->flush();
			return;
		}
	}

	if (!data.contains(key)) {
		sk->write(QString("%1: no such entry\n")
		    .arg(key).toAscii().data());
		sk->flush();
		return;
	}

	sk->write(key.toAscii().data());
	sk->write(data[key].value.toByteArray().data());
	sk->write("\n");
	sk->flush();
}

void
brokerd_engine_private::list(const char *args, QTcpSocket *sk)
{
	QList<QString> ks;

	/*
	 * default: list keys
	 */
	if (!args) {
		if (!data.size()) {
			sk->write("empty\n");
			sk->flush();
			return;
		}

		ks = data.keys();
		qSort(ks.begin(), ks.end());
		foreach (const QString &k, ks) {
			sk->write(k.toAscii().data());
			sk->write("\n");
		}
		sk->flush();
		return;
	}

	if (!strcmp(args, "sessions")) {
		if (sessions.size() <= 0) {
			sk->write("empty\n");
			sk->flush();
			return;
		}
		foreach (const QString &k, sessions.keys()) {
			QTcpSocket *se = sessions[k];
			sk->write(se->peerAddress().toString().toAscii().data());
			sk->write(":");
			sk->write(QString::number(se->peerPort()).toAscii().data());
			sk->write(":");
			sk->write(k.toAscii().data());
			sk->write("\n");
		}
	} else if (!strcmp(args, "conns")) {
		foreach (const QTcpSocket *c, conns) {
			sk->write(c->peerAddress().toString().toAscii().data());
			sk->write(":");
			sk->write(QString::number(c->peerPort()).toAscii().data());
			sk->write("\n");
		}
	} else if (!strcmp(args, "size")) {
		sk->write(QString::number(data.size()).toAscii().data());
		sk->write("\n");
	}
	sk->flush();
}

void
brokerd_engine_private::reset(QTcpSocket *sk)
{
	QList<QTcpSocket *>	tmpsks;

	foreach (const QString &k, data.keys()) {
		foreach (QTcpSocket *se, data[k].sessions) {
			if (!tmpsks.size() || !tmpsks.contains(se))
				tmpsks.append(se);
		}
		data[k].sessions.clear();
		data[k].value.clear();
	}

	foreach (QTcpSocket *se, tmpsks)
		se->write("reset\n");

	data.clear();
	sk->write("OK\n");
	sk->flush();
}

void
brokerd_engine_private::del(char *args, QTcpSocket *sk)
{
	QString			key(args);
	QList<QTcpSocket *>	tmpsks;
	int			removed = 0;
	QRegExp			re;

	key = key.trimmed();
	if (data.contains(key)) {
		foreach (QTcpSocket *se, data[key].sessions) {
			se->write("deleted ");
			se->write(key.toAscii().data());
			se->write("\n");
		}

		data[key].sessions.clear();
		data.remove(key);
		removed = 1;
	} else {
		re.setPattern(key);
		if (!re.isValid()) {
			sk->write("invalid regular expression:");
			sk->write(key.toAscii().data());
			sk->write("\n");
			sk->flush();
			return;
		}
		foreach (const QString &k, data.keys()) {
			if (k.contains(QRegExp(key))) {
				foreach (QTcpSocket *se, data[k].sessions) {
					if (!tmpsks.size() ||
					    !tmpsks.contains(se))
						tmpsks.append(se);
				}

				data[k].sessions.clear();
				data.remove(k);
				removed = 1;
			}
		}
		foreach (QTcpSocket *se, tmpsks) {
			se->write("deleted ");
			se->write(key.toAscii().data());
			se->write("\n");
		}
	}
	if (removed) {
		sk->write("OK\n");
	} else {
		sk->write(key.toAscii().data());
		sk->write(": no such entry\n");
	}
	sk->flush();
}

void
brokerd_engine_private::save(const char *args, QTcpSocket *sk)
{
	QFile		file(filename);

	if (!args && filename.isEmpty()) {
		sk->write("invalid format\n");
		sk->flush();
		return;
	}

	if (args)
		file.setFileName(args);

	if (!file.open(QIODevice::WriteOnly) ||
	    !file.resize(0)) {
		sk->write("fail to open file:");
		sk->write(file.fileName().toAscii());
		sk->write("\n");
		sk->flush();
		return;
	}
	foreach (const QString &k, data.keys()) {
		file.write(k.toAscii());
		file.write("=");
		file.write(data[k].value.toString().toAscii());
		file.write("\n");
	}
	file.close();
	sk->write("OK\n");
	sk->flush();
}
