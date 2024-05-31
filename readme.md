# PewPew for Playdate

Work in progress on running [PewPew](https://pewpew.readthedocs.io/) games on the [Playdate](https://play.date/).

## How to build

1. Clone this repository.
   ```sh
   git clone https://github.com/cwalther/pew-playdate.git pewpew
   cd pewpew
   ```

3. Initialize submodules – this pulls in MicroPython, among others.
   (Don't use `--recursive`, MicroPython has a lot of submodules and most are not needed!)
   ```sh
   git submodule update --init
   ```

4. Initialize the required ones among MicroPython’s submodules.
   ```sh
   make -f micropython_embed.mk submodules
   ```

5. Build the _embed_ port of MicroPython, including `mpy-cross`.
   ```sh
   make -f micropython_embed.mk
   ```

6. Build PewPew.
   ```sh
   make
   ```

7. Run.
   ```sh
   open pewpew.pdx
   ```

## How to use the REPL

Using the REPL (interactive interpreter) over the USB serial connection is not as straightforward as for a hardware MicroPython board, because a Playdate application does not get full access to the serial port, but is still possible using a protocol built on the `msg` command of the Playdate serial interface (which cannot transmit arbitrary binary content, but only a selection of printable ASCII characters):

* Messages that do not start with `!` are taken as-is, followed by CRLF. That allows conveniently entering simple one-liners from any terminal program or the simulator console, but not line editing or anything else that needs partial lines or bytes outside of the allowed set.

  Example:
  ```
  msg print('Hello')
  ```

* Messages that start with `!` contain arbitrary binary content encoded in Base64. That allows transmitting anything, at the cost of being inconvenient to do by hand.

  Some useful examples (the `==` padding at the end of the Base64 encoding is redundant and can be omitted):

  Backspace:
  ```
  msg !CA
  ```

  ctrl-C:
  ```
  msg !Aw
  ```

You have the following options:

* Send these commands manually either using a serial terminal program (device only) or using the _Console_ window of the Playdate simulator (device or simulator, it talks to the device if an unlocked one is connected or to the simulator otherwise).
  In the simulator console, you need to prefix the command with `!` to escape from Lua mode into command mode, e.g. `!msg print('Hello')`.

* (Device only) Use the included _terminal.py_, which implements this protocol internally to connect the terminal it is running in directly to the MicroPython terminal on the Playdate. It requires PySerial (`pip3 install pyserial`). Pass the serial port device (e.g. `/dev/cu.usbmodemPDU1_Y0…` on macOS) as a single command line argument.

  Output currently only appears in whole lines and with some extra empty lines, this is due to shortcomings of the Playdate SDK. Better look on the Playdate screen to see what you are doing.

  Press ctrl-X to exit.

Make sure that the simulator is closed while you are connecting using something else, otherwise they will compete for access to the serial port and things will not work.

## How to install games

Run the application once to let it create its data folder, then put Python files into _Data/ch.kolleegium.pewpew/Files/_, which is found under _PlaydateSDK/Disk/_ for the simulator and by connecting the Playdate in [data disk mode](https://help.play.date/games/sideloading/#data-disk-mode) for the device.

_boot.py_ and _main.py_ are run automatically at startup as usual, you may want to install the [PewPew menu](https://github.com/pypewpew/game-menu) as _main.py_.
