#.SUFFIXES: .o .c

CC = gcc
CFLAGS = -Wall
SHLIBS = -lz -lpng
STLIBS = -l:libpng.a -l:libz.a
LIBS = $(STLIBS)

OBJDIR = obj
BINDIR = out

HEADERS = colorutils.h readpng.h writepng.h

_GETPALOBJ = getpal.o colorutils.o readpng.o
GETPALOBJ = $(patsubst %,$(OBJDIR)/%,$(_GETPALOBJ))

_MAKEPALOBJ = makepal.o colorutils.o writepng.o
MAKEPALOBJ = $(patsubst %,$(OBJDIR)/%,$(_MAKEPALOBJ))

_GETCVALOBJ = getcolorvals.o colorutils.o
GETCVALOBJ = $(patsubst %,$(OBJDIR)/%,$(_GETCVALOBJ))

#rules start here
default: #default: do nothing
	$(info Please select a target (getcolorvals | getpal | makepal))

#debug rules
debug_getpal: CFLAGS += -g
debug_getpal: getpal

debug_makepal: CFLAGS += -g
debug_makepal: makepal

debug_getcval: CFLAGS += -g
debug_getcval: getcolorvals

#the '%' is special. must be including headers too, so if they change, the .c files will get recompiled.
$(OBJDIR)/%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

#with "make getpal", make will find this first. it'll understand that, to create getpal, it must create the object files. 
getpal: $(GETPALOBJ)
	$(CC) $(GETPALOBJ) -o $(BINDIR)/$@ $(LIBS) 

makepal: $(MAKEPALOBJ)
	$(CC) $(MAKEPALOBJ) -o $(BINDIR)/$@ $(LIBS)

getcolorvals: $(GETCVALOBJ)
	$(CC) $(GETCVALOBJ) -o $(BINDIR)/$@ $(LIBS)

#if a "clean" file exists, make shouldn't do anything with it
.PHONY: clean
clean:
	rm -f obj/* out/*
