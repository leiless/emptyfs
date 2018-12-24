#
# macOS generic kernel extension Makefile
#

include Makefile.inc

#
# Check mandatory vars
#
ifndef KEXTNAME
$(error KEXTNAME not defined)
endif

ifndef KEXTVERSION
$(error KEXTVERSION not defined)
endif

ifndef KEXTBUILD
# [assume] zero indicates no build number
KEXTBUILD:=	0
endif

ifndef BUNDLEDOMAIN
$(error BUNDLEDOMAIN not defined)
endif


# defaults
BUNDLEID?=	$(BUNDLEDOMAIN).kext.$(KEXTNAME)
KEXTBUNDLE?=	$(KEXTNAME).kext
KEXTMACHO?=	$(KEXTNAME).out
ARCH?=		x86_64
#ARCH?=		i386
PREFIX?=	/Library/Extensions

# Set default XCode SDK
SDKROOT?=	$(shell xcrun --sdk macosx --show-sdk-path)

SDKFLAGS=	-isysroot $(SDKROOT)
CC=		$(shell xcrun -find -sdk $(SDKROOT) cc)
#CXX=		$(shell xcrun -find -sdk $(SDKROOT) c++)
CODESIGN=	$(shell xcrun -find -sdk $(SDKROOT) codesign)

#
# Standard defines and includes for kernel extensions
#
# The __makefile__ macro used to compatible with XCode
# Since XCode use intermediate objects  which causes symbol duplicated
#
CPPFLAGS+=	-DKERNEL \
		-DKERNEL_PRIVATE \
		-DDRIVER_PRIVATE \
		-DAPPLE \
		-DNeXT \
		$(SDKFLAGS) \
		-I$(SDKROOT)/System/Library/Frameworks/Kernel.framework/Headers \
		-I$(SDKROOT)/System/Library/Frameworks/Kernel.framework/PrivateHeaders \
		-D__kext_makefile__

#
# Convenience defines
# BUNDLEID macro will be used in KMOD_EXPLICIT_DECL
#
CPPFLAGS+=	-DKEXTNAME_S=\"$(KEXTNAME)\"		\
		-DKEXTVERSION_S=\"$(KEXTVERSION)\"	\
		-DKEXTBUILD_S=\"$(KEXTBUILD)\"		\
		-DBUNDLEID_S=\"$(BUNDLEID)\"		\
		-DBUNDLEID=$(BUNDLEID)			\
		-D__TZ__=\"$(shell date +%z)\"

#
# C compiler flags
#
ifdef MACOSX_VERSION_MIN
CFLAGS+=	-mmacosx-version-min=$(MACOSX_VERSION_MIN)
else
CFLAGS+=	-mmacosx-version-min=10.4
endif
CFLAGS+=	-x c \
		-arch $(ARCH) \
		-std=c99 \
		-nostdinc \
		-fno-builtin \
		-fno-common \
		-mkernel

# warnings
CFLAGS+=	-Wall -Wextra -Os

# linker flags
ifdef MACOSX_VERSION_MIN
LDFLAGS+=	-mmacosx-version-min=$(MACOSX_VERSION_MIN)
else
LDFLAGS+=	-mmacosx-version-min=10.4
endif
LDFLAGS+=	-arch $(ARCH)
LDFLAGS+=	-nostdlib \
		-Xlinker -kext \
		-Xlinker -object_path_lto \
		-Xlinker -export_dynamic

# libraries
LIBS+=		-lkmod
#LIBS+=		-lkmodc++
LIBS+=		-lcc_kext

# kextlibs flags
KLFLAGS+=	-xml -c -unsupported -undef-symbols

# source, header, object and make files
SRCS:=		$(wildcard $(KEXTNAME)/*.c)
HDRS:=		$(wildcard $(KEXTNAME)/*.h)
OBJS:=		$(SRCS:.c=.o)
MKFS:=		$(wildcard Makefile)


# targets

all: debug

%.o: %.c $(HDRS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJS): $(MKFS)

$(KEXTMACHO): $(OBJS)
	$(CC) $(SDKFLAGS) $(LDFLAGS) $(LIBS) -o $@ $^
	otool -h $@
	otool -l $@ | grep uuid

Info.plist~: Info.plist.in
	sed \
		-e 's/__KEXTNAME__/$(KEXTNAME)/g' \
		-e 's/__KEXTMACHO__/$(KEXTNAME)/g' \
		-e 's/__KEXTVERSION__/$(KEXTVERSION)/g' \
		-e 's/__KEXTBUILD__/$(KEXTBUILD)/g' \
		-e 's/__BUNDLEID__/$(BUNDLEID)/g' \
		-e 's/__OSBUILD__/$(shell /usr/bin/sw_vers -buildVersion)/g' \
		-e 's/__CLANGVER__/$(shell $(CC) -v 2>&1 | grep version)/g' \
	$^ > $@

$(KEXTBUNDLE): $(KEXTMACHO) Info.plist~
	mkdir -p $@/Contents/MacOS
	mv $< $@/Contents/MacOS/$(KEXTNAME)

	# Clear placeholders(o.w. kextlibs cannot parse)
	sed 's/__KEXTLIBS__//g' Info.plist~ > $@/Contents/Info.plist
	awk '/__KEXTLIBS__/{system("kextlibs $(KLFLAGS) $@");next};1' Info.plist~ > $@/Contents/Info.plist~
	mv $@/Contents/Info.plist~ $@/Contents/Info.plist

ifdef COPYRIGHT
	/usr/libexec/PlistBuddy -c 'Add :NSHumanReadableCopyright string "$(COPYRIGHT)"' $@/Contents/Info.plist
endif

ifdef SIGNCERT
	$(CODESIGN) --force --timestamp=none --sign $(SIGNCERT) $@
	/usr/libexec/PlistBuddy -c 'Add :CFBundleSignature string ????' $@/Contents/Info.plist
endif

	# Empty-dependency kext cannot be load  so we add one manually
	/usr/libexec/PlistBuddy -c 'Print OSBundleLibraries' $@/Contents/Info.plist &> /dev/null || \
		/usr/libexec/PlistBuddy -c 'Add :OSBundleLibraries:com.apple.kpi.bsd string "8.0b1"' $@/Contents/Info.plist

	touch $@

	dsymutil -arch $(ARCH) -o $(KEXTNAME).kext.dSYM $@/Contents/MacOS/$(KEXTNAME)

release: $(KEXTBUNDLE)

# see: https://www.gnu.org/software/make/manual/html_node/Target_002dspecific.html
# Those two flags must present at the same time  o.w. debug symbol cannot be generated
debug: CPPFLAGS += -g -DDEBUG
debug: release

load: $(KEXTBUNDLE)
	sudo chown -R root:wheel $<
	sudo sync
	sudo kextutil $<
	# restore original owner:group
	sudo chown -R '$(USER):$(shell id -gn)' $<
	sudo dmesg | grep $(KEXTNAME) | tail -1

stat:
	kextstat | grep $(KEXTNAME)

unload:
	sudo kextunload $(KEXTBUNDLE)
	sudo dmesg | grep $(KEXTNAME) | tail -2

install: $(KEXTBUNDLE) uninstall
	test -d "$(PREFIX)"
	sudo cp -r $< "$(PREFIX)/$<"
	sudo chown -R root:wheel "$(PREFIX)/$<"

uninstall:
	test -d "$(PREFIX)"
	test -e "$(PREFIX)/$(KEXTBUNDLE)" && \
	sudo rm -rf "$(PREFIX)/$(KEXTBUNDLE)" || true

clean:
	rm -rf $(KEXTBUNDLE) $(KEXTBUNDLE).dSYM Info.plist~ $(OBJS) $(KEXTMACHO)

.PHONY: all debug release load stat unload intall uninstall clean

