#
# All-in-one Makefile
#

MAKE=make
MV=mv
RM=rm
MKDIR=mkdir
OUT=bin

all:
	$(RM) -rf $(OUT)/emptyfs.kext* $(OUT)/mount_emptyfs
	$(MKDIR) -p $(OUT)
	$(MAKE) -C kext
	$(MAKE) -C mount_emptyfs
	$(MV) kext/emptyfs.kext kext/emptyfs.kext.dSYM $(OUT)
	$(MV) mount_emptyfs/mount_emptyfs $(OUT)

clean:
	$(RM) -rf $(OUT)/emptyfs.kext* $(OUT)/mount_emptyfs
	$(MAKE) -C kext clean
	$(MAKE) -C mount_emptyfs clean

.PHONY: all clean

