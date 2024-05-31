# Portions (c) Copyright 2019 by Radomir Dopieralski, https://github.com/pypewpew/
# Portions (c) Copyright 2024 by Christian Walther
#
# This work is licensed under a Creative Commons
# Attribution-ShareAlike 4.0 International (CC BY-SA 4.0) License
# (http://creativecommons.org/licenses/by-sa/4.0/)


from micropython import const
from _pew import show as _show, keys, tick


_FONT = (
	b'\xff\xff\xff\xff\xff\xff\xf3\xf3\xf7\xff\xf3\xff\xcc\xdd\xff\xff\xff\xff'
	b'\xdd\x80\xdd\x80\xdd\xff\xf7\xc1\xf4\xc7\xd0\xf7\xcc\xdb\xf3\xf9\xcc\xff'
	b'\xf1\xccc\xcca\xff\xf3\xf7\xff\xff\xff\xff\xf2\xfd\xfc\xfd\xf2\xff\xe3'
	b'\xdf\xcf\xdf\xe3\xff\xff\xd9\xe2\xd9\xff\xff\xff\xf3\xc0\xf3\xff\xff\xff'
	b'\xff\xff\xff\xf3\xf9\xff\xff\xc0\xff\xff\xff\xff\xff\xff\xff\xf3\xff\xcf'
	b'\xdb\xf3\xf9\xfc\xff\xd2\xcc\xc8\xcc\xe1\xff\xf3\xf1\xf3\xf3\xf3\xff\xe4'
	b'\xcf\xe2\xfd\xc0\xff\xd1\xcf\xe3\xcf\xd1\xff\xf3\xf9\xdc\xc0\xcf\xff\xc0'
	b'\xfc\xd0\xcf\xd0\xff\xd2\xfc\xd0\xcc\xd1\xff\xc0\xdb\xf3\xf9\xfc\xff\xd1'
	b'\xcc\xe2\xcc\xd1\xff\xd1\xcc\xc1\xcf\xe1\xff\xff\xf3\xff\xf3\xff\xff\xff'
	b'\xf3\xff\xf3\xf9\xff\xcf\xf3\xfc\xf3\xcf\xff\xff\xc0\xff\xc0\xff\xff\xfc'
	b'\xf3\xcf\xf3\xfc\xff\xe1\xcf\xe3\xff\xf3\xff\xd2\xcd\xcc\xfd\xc6\xff\xe2'
	b'\xdd\xcc\xc4\xcc\xff\xe0\xcc\xe0\xcc\xe0\xff\xc2\xfd\xfc\xfd\xc2\xff\xe4'
	b'\xdc\xcc\xdc\xe4\xff\xc0\xfc\xf0\xfc\xc0\xff\xc0\xfc\xf0\xfc\xfc\xff\xc2'
	b'\xfd\xfc\xcd\xc2\xff\xcc\xcc\xc0\xcc\xcc\xff\xe2\xf3\xf3\xf3\xe2\xff\xcf'
	b'\xcf\xcf\xcc\xd1\xff\xcc\xd8\xf4\xd8\xcc\xff\xfc\xfc\xfc\xfc\xc0\xff\xdd'
	b'\xc4\xc0\xc8\xcc\xff\xcd\xd8\xd1\xc9\xdc\xff\xe2\xdd\xcc\xdd\xe2\xff\xe4'
	b'\xcc\xcc\xe4\xfc\xff\xe2\xdd\xcc\xc9\xd2\xcf\xe4\xcc\xcc\xe4\xcc\xff\xc1'
	b'\xfc\xd1\xcf\xd0\xff\xc0\xf3\xf3\xf3\xf3\xff\xcc\xcc\xcc\xcd\xd6\xff\xcc'
	b'\xcc\xdd\xe6\xf3\xff\xcc\xc8\xc4\xc0\xd9\xff\xcc\xd9\xe6\xd9\xcc\xff\xcc'
	b'\xdd\xe6\xf3\xf3\xff\xc0\xdb\xf3\xf9\xc0\xff\xf0\xfc\xfc\xfc\xf0\xff\xfc'
	b'\xf9\xf3\xdb\xcf\xff\xc3\xcf\xcf\xcf\xc3\xff\xf3\xc8\xdd\xff\xff\xff\xff'
	b'\xff\xff\xff\xff\xc0\xfc\xf7\xff\xff\xff\xff\xff\xc2\xcd\xcc\xc6\xff\xfc'
	b'\xe4\xdc\xcc\xe4\xff\xff\xc2\xfd\xfc\xc6\xff\xcf\xc6\xcd\xcc\xc6\xff\xff'
	b'\xd2\xcd\xf4\xc2\xff\xcb\xf3\xd1\xf3\xf3\xf3\xff\xc6\xcd\xc6\xdf\xe1\xfc'
	b'\xe4\xdc\xcc\xcc\xff\xf3\xfb\xf2\xf3\xe7\xff\xcf\xef\xcb\xcf\xce\xe1\xfc'
	b'\xcc\xf4\xd8\xcc\xff\xf2\xf3\xf3\xf3\xdb\xff\xff\xe0\xc0\xc4\xcc\xff\xff'
	b'\xe4\xdc\xcc\xcc\xff\xff\xe2\xcd\xdc\xe2\xff\xff\xe4\xdc\xcc\xe4\xfc\xff'
	b'\xc6\xcd\xcc\xc6\xcf\xff\xc9\xf4\xfc\xfc\xff\xff\xc2\xf9\xdb\xe0\xff\xf3'
	b'\xd1\xf3\xf3\xdb\xff\xff\xcc\xcc\xcd\xd2\xff\xff\xcc\xdd\xe6\xf3\xff\xff'
	b'\xcc\xc8\xd1\xd9\xff\xff\xcc\xe6\xe6\xcc\xff\xff\xcc\xcc\xd2\xdf\xe1\xff'
	b'\xc0\xdb\xf9\xc0\xff\xc7\xf3\xf8\xf3\xc7\xff\xf3\xf3\xf3\xf3\xf3\xf3\xf4'
	b'\xf3\xcb\xf3\xf4\xff\xffr\x8d\xff\xff\xfff\x99f\x99f\x99'
)


K_LEFT = const(0x01)
K_RIGHT = const(0x02)
K_UP = const(0x04)
K_DOWN = const(0x08)
K_X = const(0x10)
K_O = const(0x20)


colordepth = (2,)


def brightness(level):
	pass


def show(pix):
	_show(pix.buffer, pix.width)


class GameOver(SystemExit):
	__slots__ = ()


class Pix:
	__slots__ = ('buffer', 'width', 'height')

	def __init__(self, width=8, height=8, buffer=None):
		if buffer is None:
			buffer = bytearray(width * height)
		self.buffer = buffer
		self.width = width
		self.height = height

	@classmethod
	def from_text(cls, string, color=None, bgcolor=0, colors=None):
		pix = cls(4 * len(string), 6)
		font = memoryview(_FONT)
		if colors is None:
			if color is None:
				colors = [3, 2, 1, 0]
				if bgcolor != 0:
					colors[2] = colors[3] = bgcolor
			else:
				colors = (color, color, bgcolor, bgcolor)
		x = 0
		for c in string:
			index = ord(c) - 0x20
			if not 0 <= index <= 95:
				continue
			row = 0
			for byte in font[index * 6:index * 6 + 6]:
				for col in range(4):
					pix.pixel(x + col, row, colors[byte & 0x03])
					byte >>= 2
				row += 1
			x += 4
		return pix

	@classmethod
	def from_iter(cls, lines):
		pix = cls(len(lines[0]), len(lines))
		y = 0
		for line in lines:
			x = 0
			for pixel in line:
				pix.pixel(x, y, pixel)
				x += 1
			y += 1
		return pix

	def pixel(self, x, y, color=None):
		if not 0 <= x < self.width or not 0 <= y < self.height:
			return 0
		if color is None:
			return self.buffer[x + y * self.width]
		self.buffer[x + y * self.width] = color

	def box(self, color, x=0, y=0, width=None, height=None):
		x = min(max(x, 0), self.width - 1)
		y = min(max(y, 0), self.height - 1)
		width = max(0, min(width or self.width, self.width - x))
		height = max(0, min(height or self.height, self.height - y))
		for y in range(y, y + height):
			xx = y * self.width + x
			for i in range(width):
				self.buffer[xx] = color
				xx += 1

	def blit(self, source, dx=0, dy=0, x=0, y=0,
			 width=None, height=None, key=None):
		if dx < 0:
			x -= dx
			dx = 0
		if x < 0:
			dx -= x
			x = 0
		if dy < 0:
			y -= dy
			dy = 0
		if y < 0:
			dy -= y
			y = 0
		width = min(min(width or source.width, source.width - x),
					self.width - dx)
		height = min(min(height or source.height, source.height - y),
					 self.height - dy)
		source_buffer = memoryview(source.buffer)
		self_buffer = self.buffer
		if key is None:
			for row in range(height):
				xx = y * source.width + x
				dxx = dy * self.width + dx
				self_buffer[dxx:dxx + width] = source_buffer[xx:xx + width]
				y += 1
				dy += 1
		else:
			for row in range(height):
				xx = y * source.width + x
				dxx = dy * self.width + dx
				for col in range(width):
					color = source_buffer[xx]
					if color != key:
						self_buffer[dxx] = color
					dxx += 1
					xx += 1
				y += 1
				dy += 1

	def __str__(self):
		return "\n".join(
			"".join(
				('.', '+', '*', '@')[self.pixel(x, y)]
				for x in range(self.width)
			)
			for y in range(self.height)
		)


def init():
	pass
