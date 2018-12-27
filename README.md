## `emptyfs` - A trivial macOS VFS plug-in implementation

### Summary

`emptyfs` is a trivial VFS plug-in(kernel extension), its volumes are completely empty(except for the root directory).

This simple VFS plug-in is a minimal implementation, you can use it to explore VFS behaviours, or as a template as your own VFS plug-in.

### Build

You must install Apple's [Command Line Tools](https://developer.apple.com/download/more) as a minimal build environment, or Xcode as a full build environment in App Store.

```shell
$ make            # Debug build
$ make release    # Release build
```

Theoretically, this kext can be compile and run in macOS 10.4+, yet currently only from 10.10 up to 10.14 are tested.

If you compile in macOS >= 10.13 and wants the kext to stay compatible with macOS <= 10.12, you should specify `-DKASAN` or `-D_FORTIFY_SOURCE=0` to `CPPFLAGS`, for more info, please refer to [issues#1](https://github.com/lynnlx/emptyfs/issues/1)

### Debugging

```
# Under kext/ directory
$ make load			# Load emptyfs kext
$ make stat			# Print status of emptyfs kext
$ make unload			# Unload emptyfs kext
```

**TODO**: add info about kext logging

### Install

```shell
# Under kext/ directory
$ make install
$ make uninstall		# In case you want to uninstall
```

You can specify `PREFIX` variable to make for install location, the default is `/Library/Extensions`

**TODO:** add docs about HOWTO load at boot-up time

After installation, the kext will be loaded automatically in each boot.

### Use of `emptyfs`

To test the VFS plug-in, you must specify what device node to mounted on. Simplest approach is to create a disk image:

```shell
# Create a disk image (name: test.dmg size: 1MB type: EMPTYFS)
$ hdiutil create -size 1m -partitionType EMPTYFS test
created: /Users/lynnl/test.dmg

$ file test.dmg
test.dmg: Apple Driver Map, blocksize 512, blockcount 2048, devtype 0, devid 0, driver count 0, contains[@0x200]: Apple Partition Map, map block count 2, start block 1, block count 63, name Apple, type Apple_partition_map, valid, allocated, contains[@0x400]: Apple Partition Map, map block count 2, start block 64, block count 1984, name disk image, type EMPTYFS, valid, allocated, readable, writable, mount at startup
```

Then attach the disk image with `-nomount` flag(.: the system won't automatically mount the volumes on the image):

```shell
# The first two volumes are image scheme and map partition
#  We cares about the last one(i.e. EMPTYFS)
$ hdiutil attach -nomount test.dmg
/dev/disk2          	Apple_partition_scheme
/dev/disk2s1        	Apple_partition_map
/dev/disk2s2        	EMPTYFS
```

After you load emptyfs kext, you can create a mount point and mount the file system:

```shell
$ sudo kextload emptyfs.kext
$ mkdir emptyfs_mp
$ ./mount_emptyfs /dev/disk2s2 emptyfs_mp
```

Use [mount(8)](x-man-page://8/mount) to check mount info and explore  the file system:

```shell
$ mount | grep emptyfs
/dev/disk2s2 on /Users/lynnl/emptyfs_mp (emptyfs, local, nodev, noexec, nosuid, read-only, noowners, mounted by lynnl)

$ ls -la emptyfs_mp
total 8
dr-xr-xr-x   2 lynnl  staff  528 Dec 27 22:00 .
drwxr-xr-x+ 19 lynnl  staff  608 Dec 27 21:59 ..

$ stat emptyfs_mp
16777226 2 dr-xr-xr-x 2 lynnl staff 0 528 "Dec 27 22:00:25 2018" "Dec 27 22:00:25 2018" "Dec 27 22:00:25 2018" "Dec 27 22:00:25 2018" 4096 8 0 emptyfs_mp
```

When you explore the file system thoroughly, you can first [umount(8)](x-man-page://8/umount) the file system and then unload the kext:

```shell
$ unmount emptyfs_mp
$ sudo kextunload emptyfs.kext
```

---

### Unranked references

[EmptyFS - A very simple VFS plug-in that mounts a volume that is completely empty](https://developer.apple.com/library/archive/samplecode/EmptyFS/Introduction/Intro.html)

[MFSLives - Sample VFS plug-in for the Macintosh File System (MFS) volume format, as used on 400KB floppies](https://developer.apple.com/library/archive/samplecode/MFSLives/Introduction/Intro.html)

[mac9p - 9P for Mac](https://github.com/benavento/mac9p)

[antekone's cross-reference search site](http://xr.anadoxin.org/source/xref)

[FreeBSD and Linux Kernel Cross-Reference](http://fxr.watson.org)

[Darwin kernel miscfs](https://opensource.apple.com/source/xnu/xnu-4570.71.2/bsd/miscfs)

[Darwin kernel NFS](https://opensource.apple.com/source/xnu/xnu-4570.71.2/bsd/nfs)

[Darwin kernel NTFS](https://opensource.apple.com/source/ntfs)

[Darwin kernel msdosfs](https://opensource.apple.com/source/msdosfs)

[Darwin kernel autofs](https://opensource.apple.com/source/autofs)

[Darwin kernel cddafs](https://opensource.apple.com/source/cddafs/cddafs)

[Darwin kernel hfs](https://opensource.apple.com/source/hfs)

[Darwin kernel webdavfs](https://opensource.apple.com/source/webdavfs/webdavfs)

