#
# 'make depend' uses makedepend to automatically generate dependencies 
#               (dependencies are added to end of Makefile)
# 'make'        build executable file 'mycc'
# 'make clean'  removes all .o and executable files
#

# define the C compiler to use
CC = gcc

# define any compile-time flags
# CFLAGS = -Wfatal-errors -mfpu=neon -mfloat-abi=softfp -mcpu=cortex-a8 -fomit-frame-pointer -O3 -fno-math-errno -fno-signed-zeros -fno-tree-vectorize -export-dynamic

# define any directories containing header files other than /usr/include
#
# INCLUDES = -I/usr/include/freetype2 -I/usr/include/libpng12

# define library paths in addition to /usr/lib
#   if I wanted to include libraries not in /usr/lib I'd specify
#   their path using -Lpath, something like:
# LFLAGS = -L/usr/lib -Xlinker -rpath -Xlinker /usr/lib -L/usr/local/lib  -I

# define any libraries to link into executable:
#   if I want to link in libraries (libx.so or libx.a) I use the -llibname 
#   option, something like (this will link in libmylib.so and libm.so:
LIBS = -lpthread -l:libgmodule-2.0.so.0 -l:libdl.so -l:libglib-2.0.so.0 -lm

LIBS = `pkg-config --libs gtk+-2.0` -lm

CFLAGS = `pkg-config --cflags gtk+-2.0`

# define the C source files
SRCS = egolfui.c can.c ewin.c tune.c snap.c buttons.c msg.c batt.c dial.c

# define the C object files 
#
# This uses Suffix Replacement within a macro:
#   $(name:string1=string2)
#         For each word in 'name' replace 'string1' with 'string2'
# Below we are replacing the suffix .c of all words in the macro SRCS
# with the .o suffix
#
OBJS = $(SRCS:.c=.o)

# define the executable file 
MAIN = egolfui

#
# The following part of the makefile is generic; it can be used to 
# build any executable just by changing the definitions above and by
# deleting dependencies appended to the file from 'make depend'
#

.PHONY: depend clean

all:    $(MAIN)
	@echo  egolfui built

$(MAIN): $(OBJS) 
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

# this is a suffix replacement rule for building .o's from .c's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .c file) and $@: the name of the target of the rule (a .o file) 
# (see the gnu make manual section about automatic variables)
.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN)

depend: $(SRCS)
	makedepend $(INCLUDES) $^

# DO NOT DELETE THIS LINE -- make depend needs it