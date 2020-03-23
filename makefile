ODIR=./objs
IDIR=./src
SDIR=./src

CC=gcc
CFLAGS=-Og -I$(IDIR)
LIBS=-lX11 -lpthread -lXtst -lportaudio

_OBJ=piano.o
OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))

_DEPS=
DEPS=$(patsubst %,$(IDIR)/%,$(_DEPS))

nopiano: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS) $(ODIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(ODIR):
	mkdir -p $(ODIR)

clean:
	rm -f $(ODIR)/*.o

cleanall:
	rm -f $(ODIR)/*.o ro

