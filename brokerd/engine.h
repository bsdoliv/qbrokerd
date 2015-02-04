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

#ifndef BROKERDENGINE_H
#define BROKERDENGINE_H

#include <QObject>

class QHostAddress;

class brokerd_handler;
struct brokerd_engine_private;

class brokerd_engine : public QObject
{
	Q_OBJECT
public:
		 brokerd_engine();
		~brokerd_engine();
	void	 load(const QString &filename = QString::null);
	int	 save(const QString &filename);
	void	 unload();
	int	 listen(const QHostAddress &, int);

private slots:
	void	accept();
	void	read();
	void	close();

private:
	brokerd_engine_private *d;
};

#endif /* BROKERDENGINE_H */
