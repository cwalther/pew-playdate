from _pew import VfsPD
import os, vfs

def init():
	vfs.mount(VfsPD(), '/root')
	try:
		os.stat('/root/Files')
	except OSError:
		print(
			'Welcome!\n'
			'This appears to be the first time you are\n'
			'starting PewPew, so I am copying some example\n'
			'games to the MicroPython filesystem. Find them at\n'
			'Data/ch.kolleegium.pewpew/Files/ after rebooting\n'
			'your Playdate in Data Disk mode (press Left+Lock+\n'
			'Menu). Feel free to modify them! If you delete\n'
			'one, it will be restored to its original state at\n'
			'the next start.'
		)
		os.mkdir('/root/Files')

	for filename in os.listdir('/root/initfiles'):
		outfilename = '/root/Files/' + filename
		try:
			os.stat(outfilename)
		except OSError:
			print('  restoring', filename)
			with open('/root/initfiles/' + filename, 'rb') as inf:
				with open(outfilename, 'wb') as outf:
					while True:
						d = inf.read(256)
						if len(d) == 0:
							break
						outf.write(d)
						# flushing appears necessary to avoid "I/O Error"
						# https://devforum.play.date/t/i-o-error-when-writing-files-from-c-coroutine/24137
						outf.flush()

	vfs.mount(VfsPD('Files'), '/')
	os.umount('/root')

init()
del VfsPD, os, vfs, init
