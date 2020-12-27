PNAME = morph
PLIBNAME = lib$(PNAME)
PLIBVER = 1
SRCP = src
CC = gcc
CFLAGS = -Wall -I$(SRCP)/include -DREAL=double -DRGBA_MESH_WARP
LDFLAGS = -lm -s
SRCL = $(SRCP)/lib
SRCM = $(SRCP)/cli
OBJS = $(SRCL)/mesh.o $(SRCL)/my_malloc.o $(SRCL)/relax.o $(SRCL)/resample.o $(SRCL)/rgbaimage.o $(SRCL)/spl-array.o $(SRCL)/spline.o $(SRCL)/tga.o $(SRCL)/warp2.o $(SRCL)/warp.o
POBJS = $(SRCM)/main.o
SONAME = $(PLIBNAME).so.$(PLIBVER)

all: $(SONAME) $(PNAME)

$(SONAME): $(OBJS)
	$(CC) -shared $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(PNAME): $(POBJS) $(SONAME)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(PNAME) $(SONAME) $(OBJS) $(POBJS)