SUBDIRS = src skins

EXTRA_DIST = AUTHORS COPYING ChangeLog INSTALL README TODO

messages: rc.cpp
	$(EXTRACTRC) `find . -name \*.ui -o -name \*.rc` > rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/yakuake.pot