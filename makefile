SRCS = 	sql.cpp 		\
	Config.cpp	 	\
	WeatherData.cpp 	\
	WeatherSource.cpp  	\
	WeatherSink.cpp		\
	units.c			\
	utils.c			\
	Database.cpp		\
	VantagePro.cpp		\
	Debug.cpp		\
	Wind.cpp		\
	Rain.cpp		\
	Now.cpp			\
	testReader.cpp		\
	Hysteresis.cpp		\
	Barometer.cpp		\
	wunderground.cpp	\
	weatherbug.cpp	        \
	pwsweather.cpp	        \
	CWOP.cpp		\
	gethostbyname_r.c     	\
	httpserver.cpp		\
	XML.cpp		        \
	Almanac.cpp	        \
	Alarms.cpp		\
        socket.cpp


LD_OPTS=-lsqlite3 -lncurses -lcurl

DEBUG=-g

CFLAGS=$(DEBUG) -Wall
LDFLAGS=$(DEBUG)

CC = gcc
CPP = g++

OBJDIR  = .objs
DEPEND	= -MD
DEPDIR 	= .deps
df 	= $(DEPDIR)/$(*F)

OBJS:=$(addprefix $(OBJDIR)/,$(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(SRCS))))

EXES = wxprod 

ifdef WINDOWS_STYLE_MAKE
__CYGWINPATH_TO_WINDOWS := 's@/cygdrive/\([A-z]\)@\1:@g'
else
__CYGWINPATH_TO_WINDOWS := ''
endif

define makedeps
	sed -e 's@\([A-z]\):/@/cygdrive/\1/@' $*.d >> $*.d.trans; \
	cp $*.d.trans $(df).P.tmp; \
	sed 	-e 's/#.*//' \
		-e 's/^[^:]*: *//' \
		-e 's/ *\\$$//' \
		-e '/^$$/ d' \
		-e 's/$$/ :/' \
	       < $*.d.trans >> $(df).P.tmp; \
	sed -e $(__CYGWINPATH_TO_WINDOWS) < $(df).P.tmp > $(df).P; \
	rm -f $*.d $*.d.trans $(df).P.tmp
endef

all: prepare-0 $(EXES)

weatherlink: FORCE
	gcc -Wall  -g -I/usr/local/include weatherlink.c  ./curl-7.22.0/lib/.libs/libcurl.a /usr/lib/libidn.a /usr/lib/i386-linux-gnu/librt.a /usr/lib/i386-linux-gnu/libc.a /usr/lib/i386-linux-gnu/libz.a /usr/lib/libtidy.a -o weatherlink


prepare-0:: FORCE
	@-mkdir -p $(DEPDIR) $(OBJDIR) > /dev/null

%.o:../%.c
	@echo " CC      $(subst $(OBJDIR)/../,,$<)"
	@$(CPP) $(DEFS) $(DEPEND) $(CFLAGS) -c -o $@ $<
	@$(call makedeps)

%.o:../%.cpp
	@echo " CPP     $(subst $(OBJDIR)/../,,$<)"
	@$(CPP) $(DEFS) $(DEPEND) $(CFLAGS) -c -o $@ $<
	@$(call makedeps)


wxprod: $(OBJS) main.o
	@echo " LD      $@"
	@$(CPP) $(DEFS) $(LDFLAGS) -o $@ $^ $(LD_OPTS)

wxpro: .objs/testWriter.o
	$(CPP) $(DEFS) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LD_OPTS)

test: .objs/Debug.o .objs/Almanac.o .objs/units.o .objs/utils.o .objs/Config.o test.cpp
	$(CPP) $(DEFS) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LD_OPTS)

t: client server

server: .objs/server.o .objs/gethostbyname_r.o
	$(CPP) $(DEFS) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LD_OPTS)

client: .objs/client.o .objs/gethostbyname_r.o
	$(CPP) $(DEFS) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LD_OPTS)

clean: FORCE
	@rm -f $(OBJS) wxprod

realclean: FORCE
	@rm -f $(OBJS) wxprod
	@rm -f $(DEPS) wxprod

deps: $(SRCS)
	$(CPP) -MM -E $^ > /dev/null
	mv $(DEPS) .deps/

FORCE: ;

.PHONY: clean

-include $(addprefix $(DEPDIR)/,$(patsubst %.cxx,%.P, $(patsubst %.cpp,%.P, $(patsubst %.c,%.P,$(subst /,-,$(SRCS))))))

