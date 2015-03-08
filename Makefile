CC=			gcc
CFLAGS=		-g -Wall -O2 -Wno-unused-function
CPPFLAGS=
INCLUDES=	
OBJS=		sdg.o
PROG=		sdg
LIBS=		-lm -lz

.SUFFIXES:.c .o

.c.o:
		$(CC) -c $(CFLAGS) $(CPPFLAGS) $(INCLUDES) $< -o $@

all:sdg.o

clean:
		rm -fr gmon.out *.o ext/*.o a.out $(PROG) *~ *.a *.dSYM session*

depend:
		(LC_ALL=C; export LC_ALL; makedepend -Y -- $(CFLAGS) $(DFLAGS) -- *.c)

# DO NOT DELETE

sdg.o: sdg.h kbtree.h khash.h
