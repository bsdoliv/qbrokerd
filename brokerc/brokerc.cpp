/*
 * Copyright (c) 2015 Andre de Oliveira <deoliveirambx@googlemail.com>
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

#include <QMetaMethod>
#include <QTcpSocket>

#include "brokerc.h"

struct subscriber {
	QObject		*sc_object;
	QMetaMethod	 sc_slot;
	QTcpSocket	*sc_sock;
};

struct brokerc_private {
	QHash<brokerc_key, struct subscriber>	subscribers;
};

brokerc::brokerc() : QObject(),
    d(new brokerc_private)
{
	qRegisterMetaType<brokerc_buf>("brokerc_buf");
}

brokerc::~brokerc()
{
	delete d;
}

int
brokerc::set(const brokerc_key &key, const QVariant &value)
{
	QTcpSocket		*sk = new QTcpSocket;

	sk->connectToHost("localhost", 8008);
	if (!sk->waitForConnected(CONNECT_TIMEOUT))
		goto fail;

	sk->write("set ");
	sk->write(key.toAscii().data());
	sk->write("=");
	sk->write(value.toString().toAscii().data());
	sk->write("\n");
	sk->flush();

	if (!sk->waitForReadyRead(READ_TIMEOUT))
		goto fail;

	if (sk->readAll().trimmed() != "OK")
		goto fail;

	sk->close();
	sk->deleteLater();
	return (0);

 fail:
	sk->deleteLater();
	return (-1);
}

QVariant
brokerc::get(const brokerc_key &key)
{
	QTcpSocket		*sk = new QTcpSocket;
	QVariant		 val(QVariant::Invalid);

	sk->connectToHost("localhost", 8008);
	if (!sk->waitForConnected(CONNECT_TIMEOUT))
		goto fail;

	sk->write("get ");
	sk->write(key.toAscii().data());
	sk->write("\n");
	sk->flush();

	if (!sk->waitForReadyRead(READ_TIMEOUT))
		goto fail;

	val = sk->readAll().trimmed();
	sk->close();

 fail:
	sk->deleteLater();
	return (val);
}

int
brokerc::pub(const brokerc_key &, const QVariant &)
{
	return (0);
}

int
brokerc::sub(const brokerc_key &key, QObject *obj, const char *slot)
{
	QTcpSocket		*sk = new QTcpSocket;
	struct subscriber	 sc;
	int			 midx;
	QByteArray		 norm_slot;

	/* skip code */
	++slot;
	norm_slot = QMetaObject::normalizedSignature(slot);

	sk->connectToHost("localhost", 8008);
	if (!sk->waitForConnected(CONNECT_TIMEOUT))
		goto fail;

	connect(sk, SIGNAL(readyRead()), SLOT(sub_read()),
	    Qt::QueuedConnection);
	connect(sk, SIGNAL(disconnected()), SLOT(sub_close()),
	    Qt::QueuedConnection);

	if ((midx = obj->metaObject()->indexOfMethod(norm_slot)) == -1)
		goto fail;
	sc.sc_slot = obj->metaObject()->method(midx);
	sc.sc_object = obj;
	sc.sc_sock = sk;
	d->subscribers[key] = sc;

	sk->write("sub ");
	sk->write(key.toAscii().data());
	sk->write("\n");
	sk->flush();

	if (!sk->waitForReadyRead(READ_TIMEOUT))
		goto fail;

	return (0);
 fail:
	sk->deleteLater();
	return (-1);
}

int
brokerc::repub(const brokerc_key &key)
{
	Q_UNUSED(key);
	return (0);
}

void
brokerc::sub_read()
{
	QTcpSocket		*s = (QTcpSocket *)sender();
	char			 line[MAXLINE], *key, *value;
	brokerc_buf	 	 buf;
	struct subscriber	*sc = NULL;

	foreach (brokerc_key k, d->subscribers.keys()) {
		if (d->subscribers[k].sc_sock == s) {
			sc = &d->subscribers[k];
			break;
		}
	}

	if (sc == NULL)
		return;

	while (s->canReadLine() && s->readLine(line, sizeof(line))) {
		key = line;

		if ((value = strchr(line, '=')) == NULL &&
		    !strcmp(key, "reset"))
			continue;

		if (value) {
			*value++ = '\0';
			value += strspn(value, " \t\r\n=");
			value[strcspn(value, " \t\r\n")] = '\0';
			buf[key] = value;
		} else {
			line[strcspn(key, " \t\r\n=")] = '\0';
			buf[key] = key;
		}
	}

	if (buf.size() > 0)
		sc->sc_slot.invoke(sc->sc_object, Qt::QueuedConnection,
		    Q_ARG(brokerc_buf, buf));
}

void
brokerc::sub_close()
{
	QTcpSocket *s = (QTcpSocket *)sender();
	s->deleteLater();
}

void
brokerc::pub_read()
{
};
