
SRC_C2P = cue2pops.c
SRC_P2C = pops2cue.c

CC = gcc
CFLAGS = -Wall -Wextra

TARGET_C2P = cue2pops
TARGET_P2C = pops2cue

INSTALL_DIR = /usr/local/bin

RM = rm
CP = cp

all: cue2pops pops2cue

cue2pops:
	$(CC) $(CFLAGS) $(SRC_C2P) -o $(TARGET_C2P)

pops2cue:
	$(CC) $(CFLAGS) $(SRC_P2C) -o $(TARGET_P2C)

install: cue2pops pops2cue
	$(CP) $(TARGET_C2P) $(INSTALL_DIR)
	$(CP) $(TARGET_P2C) $(INSTALL_DIR)

clean:
	$(RM) $(TARGET_C2P) $(TARGET_P2C)
