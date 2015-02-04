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

#include <QHostAddress>
#include <QStringList>

#include "brokerd.h"
#include "engine.h"

brokerd::brokerd(int argc, char **argv) :
    QtService<QCoreApplication>(argc, argv, "brokerd"),
    engine(new brokerd_engine)
{ }

brokerd::~brokerd()
{
	delete engine;
}

void
brokerd::start()
{
	QStringList	args = qApp->arguments();
	QString		filename;
	int		idx;

	if (args.contains("-f")) {
		idx = args.indexOf("-f") + 1;
		if (idx < args.size())
			filename = args.at(idx);
	}

	engine->load(filename);

	if (!engine->listen(QHostAddress::Any, 8008))
		log_debug("fail to listen on 8008");
}

void
brokerd::stop()
{
	engine->unload();
}

int
main(int argc, char **argv)
{
	brokerd	bd(argc, argv);
	return bd.exec();
}
