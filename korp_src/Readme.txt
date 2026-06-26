Build instruction:

MSYS2 MinGw32:

Run terminall, enter the following commands one by one:

cd buildmingw32
C:\msys64\msys2_shell.cmd -mingw32 -here
cmake -G "MSYS Makefiles" ..
make

Visual Studio 2026 Community\Professional editions:

Run Visual Studio, open solution from korp_src\build\korp\korp.sln.
Build it.