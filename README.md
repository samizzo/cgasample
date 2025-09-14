CGA Sample
----------

A simple program to demonstrate setting the three palettes plus high intensity variants on the CGA video card.

Use the following keys:

| Key     | Function                                                                   |
| ------- | -------------------------------------------------------------------------- |
| Escape  | quit                                                                       |
| 0       | select palette 0 green/red/brown (video mode 4)                            |
| 1       | select palette 1 cyan/magenta/light grey (video mode 4)                    |
| 2       | select palette 2 cyan/red/light grey (video mode 5)                        |
| I       | toggle intensity palette                                                   |
| Up/Down | cycle the background colour through the 16 colours of the full CGA palette |

This talks directly to the CGA colour select register. Enable the #define `VGA_COMPATIBLE` to use interrupts to set the palette instead, which will make the program work correctly on EGA/VGA hardware.

Note that video mode 5 (for palette 2) doesn't set the right palette on a VGA card; it will set the magenta palette. Set dosbox `machine` setting to `cga` to get accurate results.
