qbrokerd
========

simplistic event-driven key/value broker backend in Qt.

build
=====

	% export QTDIR=/opt/qt
	% cd brokerd && gmake

running
=======

brokerd will listen to 8008 tcp port. try the following to test:

	% nc localhost 8008
	> list
	> set foo=bar
	OK
	> list
	foo
	> get foo
	foo=bar
	> sub foo
	foo=bar

from another client:

	% nc localhost 8008
	> pub foo=baz

the first client will receive "foo=bar".

coding
======

get/set

	#include "brokerc.h"

	brokerc_key	key;
	brokerc		bc;
	QVariant	value;

	/* sets key foo.bar with value foobar */
	key.add("foo");
	key.add("bar");
	bc.set(key, "foobar");

	/* key now is foo.* */
	key.readd("*");
	value = key.get("*");

pub/sub

	/* sub.cpp */
	#include "brokerc.h"

	class subscriber : public QObject
	{
		Q_OBJECT
	public slots:
		void keyEvent(brokerc_buf buf)
		{
			/* buf["foo.bar"] == QVariant(double, 23.33) */
			buf["foo.bar"].toDouble();
		};
	};

	void
	subscribe() {
		brokerc_key	 key;
		brokerc		 bc;
		subscriber	*sc = new subscriber;

		key.add("foo");
		key.add("*");
		bc.sub(key, &sc, SLOT(keyEvent(brokerc_buf)));
	}
	#include "pub.moc"

	/* pub.cpp */
	#include "brokerc.h"

	void
	publish() {
		brokerc_key	 key;
		QVariant	 value;
		brokerc		 bc;
		subscriber	*sc = new subscriber;

		key.add("foo");
		key.add("bar");
		value = 23.33;
		bc.set(key, value);
		bc.pub(key, value);
	}

License
=======

Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).

Commercial Usage
Licensees holding valid Qt Commercial licenses may use this file in
accordance with the Qt Solutions Commercial License Agreement provided
with the Software or, alternatively, in accordance with the terms
contained in a written agreement between you and Nokia.

GNU Lesser General Public License Usage
Alternatively, this file may be used under the terms of the GNU Lesser
General Public License version 2.1 as published by the Free Software
Foundation and appearing in the file LICENSE.LGPL included in the
packaging of this file.  Please review the following information to
ensure the GNU Lesser General Public License version 2.1 requirements
will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.

In addition, as a special exception, Nokia gives you certain
additional rights. These rights are described in the Nokia Qt LGPL
Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
package.

GNU General Public License Usage
Alternatively, this file may be used under the terms of the GNU
General Public License version 3.0 as published by the Free Software
Foundation and appearing in the file LICENSE.GPL included in the
packaging of this file.  Please review the following information to
ensure the GNU General Public License version 3.0 requirements will be
met: http://www.gnu.org/copyleft/gpl.html.
