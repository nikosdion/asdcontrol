# Apple Studio Display Brightness Control

## Compiling

Run

    make

A new file `asdcontrol` should appear in the same directory.

If compiling failed, check if you have installed packages necessary for compiling (e.g. `build-essential`).

## Usage

  ./asdcontrol [--silent|-s] [--brief|-b] [--help|-h] [--about|-a] [--detect|-d] [--list-all|-l] <hid device(s)> [<brightness>]

NOTE: You must have write permissions to this device in order to control the display being a
user, not root. If you have no such permissions, you can either grant read/write permission to
the world::

    sudo chmod a+rw /dev/usb/hiddevX

or change the ownership::

    sudo chown <your user name>:users /dev/usb/hiddevX


NOTE: It should be safe to run the program against a device other than your Apple Studio Display as
the program checks whether the device is Apple display and warns about it.


### Parameters

-s, --silent
    Suppress non-functional program output

-b, --brief
    Print brightness value only when in query mode, otherwise ignored.

-h, --help
    Show short help message and quit.

-a, --about
    Show information about the program, some credits and thanks.

-d, --detect
    Run program in the detection mode. In this mode no writes are performed to device so you can
    use any number of files if you are not sure that your monitor(s) are supported.

-l, --list-all
    Lists all "officially" supported monitors and quits.

brightness
    When this option is specified, the operation is to set brightness, otherwise, the current
    brightness is retrieved. If brightness starts with `+` or `-`, the current brightness is
    increased or decreased by that value. (Note: You have to place `--` before the negative value!)

    Brightness is an integer (whole number) parameter usually between 0 and 65536.
    Note, that not every value toggles the backlight power; different Apple Studio Display models have
    different granularity.

    See also: `--brief` option and "Known Limitations" section.


## Usage examples

In the following examples I assume that your HID device is `/usb/dev/hiddev0`. You may have it as
`/dev/hiddevX` or `/dev/usb/hiddevX`.

asdcontrol --help
    Show long help message.

asdcontrol --detect /dev/hiddev*
    Perform detection, which HID device is actually your display to be controlled.

asdcontrol /dev/hiddev0
    Read current brightness parameter

asdcontrol /dev/hiddev0 160
    Set brightness to 160. Note, that brightness setting depends on your model. Generally, this
    parameter may get values in the range ``[0-255]``.

asdcontrol /dev/hiddev0 +10
    Increment current brightness by 10.

asdcontrol /dev/hiddev0 -- -10
    Decrement current brightness by 10. Please,note ``--``!


## Known Limitations

Currently, the display detection process is not fully automated as you need to specify a HID
device path.
