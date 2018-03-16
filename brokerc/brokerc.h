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

#ifndef BROKERC_H
#define BROKERC_H

#include <QObject>
#include <QVariant>
#include <QRegExp>
#include <QStringList>

class brokerc_key : public QString {
public:
		brokerc_key(const char *s) : QString(s) { }
		brokerc_key(const QString &s) : QString(s) { }
		brokerc_key() : QString() { }
	brokerc_key	add(const QString &s) {
		if (!isEmpty())
			append(".");
		append(s);
		return (*this);
	}
	brokerc_key	pop() {
		truncate(lastIndexOf('.'));
		return (*this);
	}
	/* rewinds and add */
	brokerc_key	readd(const QString &s) {
		pop();
		return add(s);
	}
	brokerc_key	readd(int i, const QString &s) {
		rewind(i);
		return add(s);
	}
	brokerc_key	rewind(int i) {
		int j = split(".").size();
		while (--j > i)
			pop();
		return (*this);
	}
};

#define	CONNECT_TIMEOUT	10000
#define	READ_TIMEOUT	10000
#define	MAXLINE		1024

class brokerc_buf : public QHash<QString, QVariant> { };
Q_DECLARE_METATYPE(brokerc_buf);

struct brokerc_private;
class brokerc : public QObject
{
	Q_OBJECT
public:
		 brokerc();
		~brokerc();

	void	 set_server_address(const QString &);
		/* commands */
	int	 set(const brokerc_key &, const QVariant &);
	QVariant get(const brokerc_key &);
	int	 pub(const brokerc_key &, const QVariant &);
	int	 sub(const brokerc_key &, QObject *, const char *);
	int	 unsub(const brokerc_key &, QObject *, const char *);
	int	 repub(const brokerc_key &);
	int	 inc(const brokerc_key &);
	int	 add(const brokerc_key &, const QVariant &);
	int	 exists(const brokerc_key &);
	int	 del(const brokerc_key &);
private slots:
	void	 sub_read();
	void	 sub_close();
	void	 pub_read();
private:
	brokerc_private	*d;
};

#endif /* BROKERC_H */
