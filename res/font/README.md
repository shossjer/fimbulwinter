# Linux

## X11

In order to be able to read fonts with X11 we need to create additional information. This is perhaps most easily done by

    mkfontscale /path/to/res/font/
    mkfontdir /path/to/res/font/

The generated files are small and should probably be added to the repository. Better yet might be to include them in the configure script if possible, since we only need them for X11.

### Naming Convention XLFD

Copied from [here](http://www.netsweng.com/~anderson/talks/xclass/fonts.html), cudos to whomever wrote that.

Starting with X11 Release 3, fonts have been named using the following convention:

    -foundry-family-weight-slant-setwidth-addstyle-pixels-points-horiz-vert-spacing-avgwidth-rgstry-encoding

This naming convention is known as the "X Logical Font Description", or XLFD. The fields are defined as follows:

* foundry	The name of the company that digitized the font.
* family	The typeface family - Courier, Times Roman, Helvetica, etc.
* weight	The "blackness" of the font - bold, medium, demibold, etc.
* slant		The "posture" of the font. "R" for upright, "I" for italic, etc.
* setwidth	Typographic proportionate width of the font.
* addstyle	Additional typographic style information.
* pixels	Size of the font in device-dependent pixels
* points	Size of the font in device-independent points
* horiz		Horizontal dots-per-inch for which the font was designed.
* vert		Vertical dots-per-inch for which the font was designed
* spacing	The spacing class of the font.  "P" for proportional, "M" for monospaced, etc
* avgwidth	Average width of all glyphs in the font
* rgstry	Tells the language the characters conform to. (iso8859 = Latin characters)
* encoding	Further language encoding information
