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
	cp $(PROGRAM) $(DESTDIR)/bin/$(PROGRAM)

uninstall:
	$(RM) $(DESTDIR)/bin/$(PROGRAM)

upload:
	cd ../  && \
	tar zcf $(PROGRAM).tar.gz petitscheme && \
	scp $(PROGRAM).tar.gz shnya@shnya.jp:www/public/ && \
	rm $(PROGRAM).tar.gz && \
	cd petitscheme
