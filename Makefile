# Makefile

PROGRAM = petitsch
OBJS = scheme.o

CXX = g++
CXXFLAGS = -g -Wall
DESTDIR = /usr/local

.PHONY: all clean install uninstall upload
all : $(PROGRAM)

$(PROGRAM) : $(OBJS)
	$(CXX) -o $(PROGRAM) $^

.cc.o:
	$(CXX) $(CXXFLAGS) -c $<

clean:
	$(RM) $(PROGRAM) $(OBJS)

install: $(PROGRAM)
	[ ! -e $(DESTDIR)/bin ] && mkdir -p $(DESTDIR)/bin; \
	cp $(PROGRAM) $(DESTDIR)/bin/$(PROGRAM)

uninstall:
	$(RM) $(DESTDIR)/bin/$(PROGRAM)

upload:
	tar zcvf ../$(PROGRAM).tar.gz -C ../ $(PROGRAM) 
	scp ../$(PROGRAM).tar.gz shnya@shnya.jp:www/public/
	rm ../$(PROGRAM).tar.gz
