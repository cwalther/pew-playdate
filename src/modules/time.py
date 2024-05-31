import sys
_path = sys.path
sys.path = ()
try:
	from time import *
finally:
	sys.path = _path
	del _path

_last_monotonic = 0.0
_last_ticks = ticks_ms()
def monotonic():
	global _last_monotonic, _last_ticks
	ticks = ticks_ms()
	_last_monotonic += ticks_diff(ticks, _last_ticks)*0.001
	_last_ticks = ticks
	return _last_monotonic
