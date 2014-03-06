PRGNAME=sido
PRGTITLE=Sido
VERSION=0.13
PREFIX=/usr/local
OBJ=sido.o controller.o menu.o about.o callbacks.o
MO=fr_FR.mo
CFLAGS=`pkg-config --cflags glib-2.0` `pkg-config --cflags gtk+-2.0`
LDFLAGS=`pkg-config --libs glib-2.0` `pkg-config --libs gtk+-2.0`

all: configure $(OBJ) $(MO)
	$(CC) -g -Wall -Wextra $(OBJ) -o $(PRGNAME) $(CFLAGS) $(LDFLAGS)

%.o: %.c
	$(CC) -g -Wall -Wextra -c -o $@ $< $(CFLAGS)

%.mo: %.po
	msgfmt -c -o $@ $<

configure:	
	sed -e "s#__VERSION__#$(VERSION)#" -e "s#__PRGTITLE__#$(PRGTITLE)#" -e 's#__PRGLOCALEPATH__#$(PREFIX)/share/locale#' version.h.in > version.h
	sed -e "s#__VERSION__#$(VERSION)#" -e "s#__PRGNAME__#$(PRGNAME)#" PKGBUILD.in > PKGBUILD
preinstall:
	sed -e "s#__PREFIX__#$(PREFIX)#" -e "s#__PRGTITLE__#$(PRGTITLE)#" -e "s#__PRGNAME__#$(PRGNAME)#" $(PRGNAME).desktop.in > $(PRGNAME).desktop

pot:
	xgettext -k_ -d $(PRGNAME) -s -o $(PRGNAME).pot *.c

%.po: $(PRGNAME).pot
	msgmerge -s -U $@ $(PRGNAME).pot

install: preinstall
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	mkdir -p $(DESTDIR)$(PREFIX)/share/icons
	mkdir -p $(DESTDIR)$(PREFIX)/share/locale/fr_FR/LC_MESSAGES

	install $(PRGNAME) $(DESTDIR)$(PREFIX)/bin/
	install $(PRGNAME).desktop $(DESTDIR)$(PREFIX)/share/applications/
	cp $(PRGNAME).png $(DESTDIR)$(PREFIX)/share/icons/
	cp fr_FR.mo $(DESTDIR)$(PREFIX)/share/locale/fr_FR/LC_MESSAGES/$(PRGNAME).mo

uninstall:
	$(RM) $(DESTDIR)$(PREFIX)/bin/$(PRGNAME) 
	$(RM) $(DESTDIR)$(PREFIX)/share/icons/$(PRGNAME).png 
	$(RM) $(DESTDIR)$(PREFIX)/share/applications/$(PRGNAME).desktop
	$(RM) $(DESTDIR)$(PREFIX)/share/locale/fr_FR/LC_MESSAGES/$(PRGNAME).mo

dist: clean
	cd ..; cp -a $(PRGNAME) $(PRGNAME)-$(VERSION); $(RM) -r $(PRGNAME)-$(VERSION)/.git; find $(PRGNAME)-$(VERSION) -name .svn | xargs rm -rf; tar zcvf $(PRGNAME)-$(VERSION).tar.gz $(PRGNAME)-$(VERSION); $(RM) -r $(PRGNAME)-$(VERSION); cd -

clean:
	$(RM) $(PRGNAME) $(OBJ) $(MO) $(PRGNAME).desktop version.h PKGBUILD *~
