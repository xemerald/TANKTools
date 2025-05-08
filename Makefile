#
#
#
CFLAG = /usr/bin/gcc -Wall -O3 -flto -g -I./include
SRC = ./src
INSTALL_DIR = /usr/local/bin

#
PROGS = \
	tnk_cut \
	tnk_remux \
	tnk_extract \
	tnk_sniff

all: $(PROGS)

tnk_cut: $(SRC)/tnk_cut.o $(SRC)/scan.o $(SRC)/swap.o $(SRC)/progbar.o
	$(CFLAG) -o $@ $(SRC)/tnk_cut.o $(SRC)/scan.o $(SRC)/swap.o $(SRC)/progbar.o -lm

tnk_remux: $(SRC)/tnk_remux.o $(SRC)/scan.o $(SRC)/swap.o
	$(CFLAG) -o $@ $(SRC)/tnk_remux.o $(SRC)/scan.o $(SRC)/swap.o $(SRC)/progbar.o -lm

tnk_extract: $(SRC)/tnk_extract.o $(SRC)/scan.o $(SRC)/swap.o
	$(CFLAG) -o $@ $(SRC)/tnk_extract.o $(SRC)/scan.o $(SRC)/swap.o $(SRC)/progbar.o -lm

tnk_sniff: $(SRC)/tnk_sniff.o $(SRC)/scan.o $(SRC)/swap.o
	$(CFLAG) -o $@ $(SRC)/tnk_sniff.o $(SRC)/scan.o $(SRC)/swap.o $(SRC)/progbar.o -lm


# Compile rule for Object
%.o:%.c
	$(CFLAG) -c $< -o $@

#
install:
	@echo Installing to $(INSTALL_DIR)...
	@for x in $(PROGS) ; \
	do \
		cp ./$$x $(INSTALL_DIR); \
	done
	@echo Finish installing of all programs!

# Clean-up rules
clean:
	(cd $(SRC); rm -f *.o *.obj *% *~; cd -)

clean_bin:
	rm -f $(BIN_NAME)
