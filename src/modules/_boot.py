from _pew import VfsPD
import os, vfs

vfs.mount(VfsPD(), '/root')
try:
	os.stat('/root/Files')
except OSError:
	os.mkdir('/root/Files')
vfs.mount(VfsPD('Files'), '/')

os.umount('/root')
del VfsPD, os, vfs
