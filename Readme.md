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
(which should be built according to the instruction below), \
and provided compiled Java samples. 

Build instruction: 

MSYS2 MinGw32: 

Run terminall, enter the following commands one by one: 
```
cd korp_src\buildmingw32 \
C:\msys64\msys2_shell.cmd -mingw32 -here \
```
cmake -G "MSYS Makefiles" .. (this step could be skipped because this repo already contains generated Makefile and you don't want to regenerate it) \
```
make 
```
Place into directory korp_src\buildmingw32 extracted folder classes mentioned above. \
Test running the command for example korpvm.exe -cp classpath Primes 500 

Visual Studio 2026 Community\Professional editions: 

Run Visual Studio, open solution from korp_src\build\korp\korp.sln. \
Build it. 

Place into directory korp_src\build\korp\Debug (if you built Debug configuration) \
extracted folder classes mentioned above and compiled Java samples. \
Test running the command for example korp.exe -cp classpath Primes 500

JetBrains CLion: \
Run CLion IDE, open folder korp_src. \
Build it. 

Place into directory korp_src\cmake-build-debug extracted folder classes mentioned above and compiled Java samples. \
Test running the command for example korpvm.exe -cp classpath Primes 500 

Folders description:

app_src/ --  folder for the Java source of the demo applications. \
korp_src/ -- folder for source code of KORP that builds the korp.exe 

Note: KORP is supposed for J2ME, so all demo classes are compiled for JRE 1.1.

Demo applications

HelloWorld -- Print "Hello World!" on screen. It actually compiles tens of Java methods in this simple application. \
Primes -- Compute and output all the prime numbers smaller than the input number; \
Factorial -- Compute the factorial of an input number. \
Fibonacci -- Compute the Fibonacci number of an input number. \
GCD - Compute the greates common divisor of two intergers. \
Josephus - Compute the last survivor wiht two input numbers. About Josephus problem description, please refer to wiki: http://en.wikipedia.org/wiki/Josephus_problem. 

Run the demo applications

Step 1. Open a command window (Run -> cmd). \
Step 2. Enter demo folder (cd C:\korp_demo). \
Step 3. Start demo Java apps as below: \
	korp -cp classpath app_name [arg1] [arg2]

Examples: 

(1) HelloWorld

    C:\korp_demo>korp -cp classpath HelloWorld 
    Hello World! 

(2) Primes

    C:\korp_demo>korp -cp classpath Primes 500 
     2 3 5 7 11 13 17 19 23 29 
     31 37 41 43 47 53 59 61 67 71 
     73 79 83 89 97 101 103 107 109 113 
     127 131 137 139 149 151 157 163 167 173 
     179 181 191 193 197 199 211 223 227 229 
     233 239 241 251 257 263 269 271 277 281 
     283 293 307 311 313 317 331 337 347 349 
     353 359 367 373 379 383 389 397 401 409 
     419 421 431 433 439 443 449 457 461 463 
     467 479 487 491 499

Note: For memory footprint consideration, the maximal input number for the Prime application is limited. In my test it is 5990. The limitation on input is similar with other demo applications.

(3) Factorial

    C:\korp_demo>korp -cp classpath Factorial 10 
    3628800

(4) Fibonacci

    C:\korp_demo>korp -cp classpath Fibonacci 15 
    610

(5) GCD

    C:\korp_demo>korp -cp classpath GCD 121 33 
    Greatest Common Divisor (GCD) is: 
    11

(6) Josephus

    C:\korp_demo>korp -cp classpath Josephus 100 5 
    The last survivor is 47

