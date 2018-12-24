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

After installation, the kext will be loaded automatically in each boot.

### Use of `emptyfs`

**TODO**

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

