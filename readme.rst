====================================
PlanarTools - Avisynth plugin
====================================

This plugin is a set of filters that offerd converting packed(interleaved)
formats to planar formats and vice versa.

Syntax:
=========

PackedToPlanar:
-----------------

Convert packed format to planar format without changing the pixel values.::

    PackedToPlanar(clip clip)

clip - RGB24, RGB32, or YUY2. output format is YV24(RGB24, RGB32) or YV16(YUY2).

On RGB clip, each color channels are mapped as follows.

    Green: Y
    Blue: U
    Red: V

Alpha channel will be dropped.

On YUY2 clip, this is same as ConvertToYV16.

ExtractPlane:
---------------

Extract a color channel from packed format, and output as a Y8 clip.::

    ExtractPlane(clip clip[, int plane])

clip - RGB24, RGB32, or YUY2.

plane - color channel to extract.

    0: Blue(RGB24/RGB32) or Y(YUY2).
    1: Green(RGB24/RGB32) or U(YUY2).
    2: Red(RGB24/RGB32) or V(YUY2).
    3: Alpha(RGB32)

    This is same as ShowGreen/ShowBlue/ShowRed/ShowAlpha(on RGB) or
    ConvertToY8/UToY8/VToY8(on YUY2), but faster.

    default is 0(RGB24/YUY2) or 3(RGB32).

PlanarToPacked:
-------------------

Convert planar format clip(s) to RGB24 or YUY2 without changing the pixel values.::

    PlanarToPacked(clip clip)
    PlanarToPacked(clip clip0, clip clip1, clip clip2)

clip - YV24(output is RGB24),  or YV16(same as ConvertToYUY2).


clip0, clip1, clip2 - planar formats.

    Make a RGB24/YUY2 clip from three Y-planes. Each Y-planes are mapped as follows.

    clip0: Green or Y.
    clip1: Blue or U.
    clip2: Red or V.

PlanarToRGB32:
------------------

Make a RGB32 clip from multiple planar format clips without changing the pixel values.::

    PlanarToRGB32(clip base, clip alpha)
    PlanarToRGB32(clip green, clip blue, clip red, clip alpha)

base - YV24 only. Each planes are mapped as follows.

    Y: Green
    U: Blue
    V: Red

green, blue, red - planar formats clips. Only Y-planes are used.
alpha - planar format. only Y-plane is used. must be the same resolution of others.

RGBToRGB:
-------------

Convert RGB24 clip to RGB32, and vice versa.::

    RGBToRGB(clip clip)

clip - RGB24 or RGB32.


Note:
=========
- All filters are SSE2 optimized. Thus these will cause crash on No SSE2 machine.
- RGBToRGB requires SSSE3. If SSSE3 is not available on your machine, it invoke internal ConvertToRGB24/ConvertToRGB32 filter instead.

License:
==========
GPL v2 or later.

Source code:
==============
https://github.com/chikuzen/PlanarTools/
