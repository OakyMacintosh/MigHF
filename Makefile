CC := gcc
TARGET := mhf
PREFIX := /opt/mighf

all:
	$(CC) ./ISA2.0/mhfsoft.c -o $(TARGET)

tools:
	$(CC) ./toolchain/mighf-unknown-gnuabi-as.c -o mighf-unknown-gnuabi-as
	$(CC) ./toolchain/mighf-unknown-gnuabi-cc.c -o mighf-unknown-gnuabi-cc
	
install:
	cp -r ./mighf-unknown-gnuabi-* $(PREFIX)
	cp mhf $(PREFIX)

# .PHONY all toolchain install
