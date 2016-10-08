
# AY/YM sound co-processor emulation library.

  This is the AY/YM sound chip emulator. These chips was used in such
computers as Sinclair ZX Spectrum 128Kb(AY8912), Atari and some others.

  This library allows you to use AY/YM sound effects and music in your own
programms, games, demos, etc.

Library has simple API which is documented with Doxygen.
You will also find VTX file description inside.

[Downloads](https://sourceforge.net/projects/libayemu/files/)

[Homepage](https://asashnov.github.io/libayemu.html)

There are thousands songs from games, magazines, demos
are available from on the Internet.



## Compiling the sources, testing

$ ./bootstrap

$ ./configure

$ make

$ aoss ./test/test

$ aoss ./apps/playvtx/playvtx music_sample/ritm-4.vtx



## Using the library

You can get some examples of using this library:

* custom register data set: test/test.c in source code;
* playvtx (console player)
* xmms-vtx (xmms input plugin)
* gstreamer-vtx (gstreamer plugin)
* DeadBeef music player


### How to build

Requires: automake-1.10, autoconf-2.61, libtool-1.5, cvs2cl

$ git clone git@github.com:asashnov/libayemu.git
$ cd libayemu
$ sh bootstrap
$ ./configure
$ make
$ sudo make install


### Links

[AY-3-8910 wiki page](https://ru.wikipedia.org/wiki/AY-3-8910)

[AY-3-8910, AY-3-8912, YM2149 Homepage](http://bulba.untergrund.net/)

[AyEmul from Bulba](http://bulba.untergrund.net/emulator_e.htm)

[AYFly](http://code.google.com/p/ayfly) - (unavailable now)
Available platforms: Android, Windows, MacOS_X, Linux, Raspberry_Pi.

[ZX Tune cross-platform player](http://zxtune.bitbucket.org/)
Available platforms: Android, Windows, MacOS_X, Linux, Raspberry_Pi.

[ZX-Spectrum sound kit](https://sourceforge.net/projects/zxssk/)
Project core is fast and accurate resampler, based on FIR-filter,
heavily optimized for piecewise-constant functions.
Above this is clock-precise AY-3-8910 (YM2149F) emulator.
Front-ends are win32 console app, Winamp and GSPlayer plugins.

[Music archive at Bulba's page](http://bulba.untergrund.net/music_e.htm)

[ZXTUNES — World's largest ZX Spectrum music collection](http://zxtunes.com/)
