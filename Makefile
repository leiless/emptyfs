#
# All-in-one Makefile
#

MAKE=make
MV=mv
RM=rm
MKDIR=mkdir
OUT=bin

all: debug

debug:
	$(RM) -rf $(OUT)/emptyfs.kext* $(OUT)/mount_emptyfs*
	$(MAKE) -C kext $(TARGET)
	$(MAKE) -C mount_emptyfs $(TARGET)
	$(MKDIR) -p $(OUT)
	$(MV) kext/emptyfs.kext kext/emptyfs.kext.dSYM $(OUT)
	$(MV) mount_emptyfs/mount_emptyfs $(OUT)
	$(MV) mount_emptyfs/mount_emptyfs.dSYM $(OUT) 2> /dev/null || true

release: TARGET=release
release: debug

clean:
	$(RM) -rf $(OUT)/emptyfs.kext* $(OUT)/mount_emptyfs
	$(MAKE) -C kext clean
	$(MAKE) -C mount_emptyfs clean

.PHONY: all clean

