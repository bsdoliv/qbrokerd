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

#include <QCoreApplication>
#include <QSocketNotifier>
#include <QDebug>
#include <QFile>

#include "brokerc.h"

class brokerclient : public QCoreApplication
{
	Q_OBJECT
public:
	brokerclient(int argc, char **argv) :
	    QCoreApplication(argc, argv),
	    stdin_notifier(new QSocketNotifier(STDIN_FILENO,
		QSocketNotifier::Read, this)),
	    stdin_file(this)
	{ 
		if (!stdin_file.open(STDIN_FILENO, QIODevice::ReadOnly)) {
			qFatal("failed to open stdin");
			qApp->quit();
		}
		connect(stdin_notifier, SIGNAL(activated(int)),
		    SLOT(read_stdin(int)), Qt::QueuedConnection);
		stdin_notifier->setEnabled(true);
		printf("> ");
		fflush(stdout);
	}

	~brokerclient()
	{
		delete stdin_notifier;
		stdin_file.close();
	}

private slots:
	void read_stdin(int)
	{
		char		line[1024], *method, *args, *value;
		brokerc_key	key;
		QVariant	val;

		memset(line, 0, sizeof(line));
		if (stdin_file.readLine(line, sizeof(line)) == -1 ||
		    !line[0]) {
			printf("\n");
			fflush(stdout);
			qApp->exit(0);
			return;
		}

		method = line;
		if ((args = strchr(method, ' ')) != NULL) {
			*args++ = '\0';
			args += strspn(args, " \t\r\n=");
			if ((value = strchr(args, '=')) != NULL) {
				*value++ = '\0';
				value += strspn(args, " \t\r\n=");
				value[strcspn(value, " \t\r\n")] = '\0';
			} else
				args[strcspn(args, " \t\r\n=")] = '\0';

			key.add(args);
		} else
			line[strcspn(method, " \t\r\n=")] = '\0';


		if (!strcmp(method, "pub")) {
			broker_client.pub(key, value);
		} else if (!strcmp(method, "sub")) {
			broker_client.sub(key, this,
			    SLOT(sub_read(brokerc_buf)));
			return;
		} else if (!strcmp(method, "set")) {
			broker_client.set(key, value);
		} else if (!strcmp(method, "get")) {
			val = broker_client.get(key);
			if (!val.isValid())
				printf("failed to retrieve value\n");
			printf("%s\n", val.toString().toAscii().data());
#if 0
		} else if (!strcmp(method, "del")) {
			broker_client.del(key);
		} else if (!strcmp(method, "list")) {
			broker_client.list(args);
		} else if (!strcmp(method, "reset")) {
			broker_client.reset(args);
		} else if (!strcmp(method, "save")) {
			args[strcspn(args, " \t\r\n=")] = '\0';
			broker_client.save(args);
#endif
		} else
			printf("invalid method\n");

		printf("> ");
		fflush(stdout);
	};

	void sub_read(brokerc_buf buf)
	{
		QString v;

		foreach (QString k, buf.keys()) {
			v = buf[k].toString();
			printf("\n%s=%s\n", k.toAscii().data(),
			    v.toAscii().data());
		}
		printf("> ");
		fflush(stdout);
	}

private:
	QSocketNotifier	*stdin_notifier;
	QFile		 stdin_file;
	brokerc		 broker_client;
};

int
main(int argc, char **argv)
{
	brokerclient app(argc, argv);
	return app.exec();
}

#include "brokerclient.moc"
