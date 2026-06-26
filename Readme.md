This code is the companion code to the book "Advanced Design and Implementation of Virtual Machines" \
(https://play.google.com/store/books/details/Xiao_Feng_Li_Advanced_Design_and_Implementation_of?id=zLbZDQAAQBAJ) \
and contains minimal implementation of working Java Virtual Machine (JVM) for educational goals only. \
It requires classes.zip folder from JDK 1.1 which could be downloaded from \
https://www.oracle.com/java/technologies/java-archive-downloads-javase11-downloads.html \
Required file is jdk-1_1_8_010-windows-i586.exe. \
Once the JDK 1.1 is fully installed or extracted, \
navigate to the \lib\ directory inside the installation folder. \
You will find classes.zip sitting inside \lib\classes.zip. \
Folder classes should be extracted and placed to the same directory with executable file of jvm korpvm.exe \
(which should be built according to the instruction below) and compiled Java samples. 

Build instruction: 

MSYS2 MinGw32: 

Run terminall, enter the following commands one by one: 

cd korp_src\buildmingw32 \
C:\msys64\msys2_shell.cmd -mingw32 -here \
cmake -G "MSYS Makefiles" .. (this step could be skipped because this repo already contains generated Makefile and you don't want to regenerate it) \
make 

Place into directory korp_src\buildmingw32 extracted folder classes mentioned above. \
Test running the command for example korpvm.exe -cp classpath Primes 500 \

Visual Studio 2026 Community\Professional editions: \

Run Visual Studio, open solution from korp_src\build\korp\korp.sln. \
Build it. \

JetBrains CLion: \
Run CLion IDE, open folder korp_src. \
Build it. \

Place into directory korp_src\cmake-build-debug extracted folder classes mentioned above and compiled Java samples. \
Test running the command for example korpvm.exe -cp classpath Primes 500 \
