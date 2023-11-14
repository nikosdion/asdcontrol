# Apple Display Brightness Control

A simple program to control the brightness of Apple thunderbolt dipsplays (Pro Display XDR, Studio Display) on Linux.

Based on acdcontrol by Pavel Gurevich.

## Compiling

Make sure you have installed the necessary packages in your distribution, e.g. `sudo apt install build-essential` on Ubuntu.

Run `make`. A new file `asdcontrol` should appear in the same directory.

Use `sudo make install` to install the compiled program in `/usr/local/bin/asdcontrol`.

## Usage

  ./asdcontrol [--silent|-s] [--brief|-b] [--help|-h] [--about|-a] [--detect|-d] [--list-all|-l] <hid device(s)> [<brightness>]

### Parameters

`-s, --silent`

Suppress non-functional program output

`-b, --brief`

Only output the brightness level when no absolute or relative brightness is provided in the command line.

`-h, --help`

Display the built-in help message

`-a, --about`

Display the copyright and license information.

`-d, --detect`

Detect the Apple Display HID device. You **must** also provide `/dev/usb/hiddev*` (or `/dev/hiddev*`, depending on your Linux distribution) in the command line for the detection to do anything useful.

`-l, --list-all`

Lists all supported monitor models and quits.

`<brightness>`

When this option is not provided, the program will read and report the current brightness level of the monitor.

When this option is set, the program will set the brightness to the option's value. You can use a relative brightness level by prefixing the option value with `+` to increase the brightness by this much, or `-` to decrease the brightness by this much. Please note that you must place `--` before a negative value. See the examples.

The brightness is an integer (whole number). For the Apple Studio Display (2022) this can be between 400 and 60000. Not all values result in a visible backlight power change. It appears that the granularity is around 2980 (for a total of 20 brightness steps from lowest to highest brightness) for the 2022 Apple Studio Display.

Alternatively, you can use a percentage e.g. 20% to set the brightness to this monitor-specific level. You can of course prefix by + or - to increase or decrease, respectively, the brightness by this amount.

### Examples

Assuming that your monitor's device is `/usb/dev/hiddev0` here are some usage examples:

`asdcontrol --help`

Show the built-in help message.

`asdcontrol --about`

Show the copyright and license information.

`asdcontrol --detect /dev/usb/hiddev*`

Detect the device which corresponds to your Apple Display.

`asdcontrol /usb/dev/hiddev0`

Read and report the current brightness level.

`asdcontrol /usb/dev/hiddev0 10000`

Set the brightness to 30000; that's about 50% brightness. The range of values for Apple Studio Display is 400 to 60000.
The range of values for Apple XDR Display is still untested.

`asdcontrol /dev/hiddev0 +5960`

Increment current brightness by 5960 (that's a 10% brightness increase).

`asdcontrol /dev/hiddev0 -- -5960`

Decrement current brightness by 5960 (that's a 10% brightness decreate). Please note the `--` before the negative number. Without the double dash, a single dash (‘tack’) is understood as setting an option, therefore it won't work.

## Troubleshooting

### Cannot detect the display

This program only supports Apple Displays (Pro Display XDR and Studio Display).

To make sure that the Apple Display is recognised correctly by the kernel's USB subsystem run `lsusb  | grep -E '05ac:(1114|9243)'`. It should reply with something like the following (the bus and device may be different on your system):

```
Bus 005 Device 003: ID 05ac:1114 Apple, Inc. Studio Display
```

The stuff next to the ID is the USB vendor identifier, followed by a colon, followed by the USB product identifier, followed by a space and the product description as returned by the connected device. This program looks for the vendor identifier 05ac (Apple, Inc) and the product ID 1114 or 9243 (Apple Studio Display, Pro Display XDR).

If you want to control an older Apple monitor please look at [https://github.com/yhaenggi/acdcontrol](acdcontrol).

### This device is not a USB monitor!

This program checks that the HID device you provided in the command line implements the USB Monitor Control interface. If it doesn't, it returns the error “This device is not a USB monitor”.

Try using a different USB HID device. The one you are using does not correspond to your Apple Display.

### Permission denied

If you get `Permission denied` trying to use this program you have a permissions issue. You need write permissions to the Apple Display's HID device to be able to control its brightness if you are not root.

The _correct_ way to address this is to create a udev rule:

#### For Apple XDR Display
Create a file `/etc/udev/rules.d/50-apple-xdr.rules` with the contents:
```
KERNEL=="hiddev*", ATTRS{idVendor}=="05ac", ATTRS{idProduct}=="9243", GROUP="users", OWNER="root", MODE="0660"
```

#### For Apple Studio Display
Create a file `/etc/udev/rules.d/50-apple-studio.rules` with the contents:
```
KERNEL=="hiddev*", ATTRS{idVendor}=="05ac", ATTRS{idProduct}=="1114", GROUP="users", OWNER="root", MODE="0660"
```

Afterwards, reload the udev permissions with `sudo udevadm control --reload-rules`.

The program checks if the HID device you have provided is a known model (based on the USB vendor and product IDs reported by the device) and whether it supports HID Monitor Control. As a result it should be safe to use against the wrong HID device; it will simply tell you something like “This device is not a USB monitor!”.

This also means that if you are not sure which HID device is your Apple Display you can run this program against `/dev/usb/hiddev*` (or `/dev/hiddev*`, depending on your Linux distribution), i.e. tell it to go through _all_ known HID devices. The program will operate only against the HID devices which correspond to an Apple Display.

## Known Limitations

Currently, the display detection process is not fully automated as you need to specify a HID device path. You can, of course, use `/dev/usb/hiddev*` (or `/dev/hiddev*`, depending on your Linux distribution) to have the program go through the entire list of HID devices connected to the computer. This is generally safe, albeit a tad slow. If you have more than one Apple Display monitors it will also apply the brightness controls to both monitors which might not be what you intended to do.

This program only controls the monitor brightness. Apple monitors do not expose any controls for contrast, saturation etc. Moreover, this program cannot be used to update the monitor's firmware or change the camera settings such as Center Stage; this is only possible through macOS (the monitor essentially runs a cut-down version of iOS).
