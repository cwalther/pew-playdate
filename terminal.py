#!/usr/bin/env python3

import os
import sys
import base64
import serial
sys.path.append(os.path.join(os.path.dirname(__file__), 'micropython', 'tools', 'mpremote'))
from mpremote.console import Console

with serial.Serial(sys.argv[1]) as ser:
	ser.write(b'echo off\n')
	console = Console()
	try:
		console.enter()
		while True:
			console.waitchar(ser)
			c = console.readchar()
			if c:
				if c in (b"\x1d", b"\x18"):  # ctrl-] or ctrl-x, quit
					break
				else:
					ser.write(b'msg !')
					ser.write(base64.b64encode(c))
					ser.write(b'\n')

			try:
				n = ser.inWaiting()
			except OSError as er:
				if er.args[0] == 5:  # IO error, device disappeared
					print("device disconnected")
					break
			if n > 0:
				console.write(ser.read(n))
	finally:
		console.exit()
