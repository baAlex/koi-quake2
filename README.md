Quake 2 Me
==========

This is a mod/addon that changes Quake 2 gameplay mechanics just for
fun and experimentation. Regarding the name it doesn't mean anything (yet).

While compatible with any Quake II client that uses the original unaltered
mod API, the "Yamagi Quake 2 Client" is highly recommended to play this
addon. For more information visit http://www.yamagi.org/quake2.


Installation for FreeBSD, Linux and OpenBSD:
--------------------------------------------
1. Type `make` or `gmake` to compile the 'game.so' file.
2. Create a subdirectory 'q2me' in your Quake 2 directory.
3. Copy 'release/game.so' to 'q2me'.
4. Start the game with `./quake2 +set game q2me`

Installation for OS X:
----------------------
1. Create a subdirectory 'q2me' in your Quake 2 directory.
2. Copy 'game.dynlib' from the zip-archive to 'q2me'.
3. Start the game with `quake2 +set game q2me`

If you want to compile 'q2me' for OS X from source, please take a
look at the "Installation" section of the README of the Yamagi Quake 2
client. In the same file the integration into an app-bundle is
explained.

Installation for Windows:
-------------------------
1. Create a subdirectory 'q2me' in your Quake 2 directory.
2. Copy 'game.dll' from the zip-archive to 'q2me'.
3. Start the game with `quake2.exe +set game q2me`

If you want to compile 'q2me' for Windows from source, please take a
look at the "Installation" section of the README of the Yamagi Quake 2
client. There's descripted how to setup the build environment.
