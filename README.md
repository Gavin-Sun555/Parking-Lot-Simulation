# Parking-Lot-Simulation Program  BETA 0.4.1  (2018-08-07) 

## About
This is my first C++ course project of the course VE101 as a freshman student at [University of Michigan-Shanghai Jiao Tong University Joint Institute](http://umji.sjtu.edu.cn/). It is a course designed for new engineers to design simple algorithms and understand the main concepts of programming. To be honest, the code style of this project is not good and there are countless bugs and perhaps some memory leaks in this project. It is posted to record the starting point of my coding career. Future modifications may be made. Following is the original readme wrote at that time. I transfer the original txt file into markdown to make it easier to read. There are many redundent descrptions, which I will fix later.

## Compiling and Quick Star
### Install OpenGL
Due to the project requirement, this outdated version of OpenGL is used. Please kindly follow the instruction to install it.
#### Windows (with MinGW)
Download FreeGlut For [MinGW](https://www.transmissionzero.co.uk/files/software/development/GLUT/freeglut-MinGW.zip) and you can get the following directory structure:
```
freeglut 
  bin
    freeglut.dll 
    x64
      freeglut.dll 
  Copying.txt
  include 
    GL
      freeglut_ext.h
      freeglut.h
      freeglut_std.h
      glut.h
  lib
    libfreeglut_static.a x64
    libfreeglut.a
    x64
      libfreeglut_static.a
      libfreeglut.a
  Readme.txt

6 directories, 12 files
```
Then you should copy some of these files into the corresponding MinGW directory (typically like this).
```
mingw64
  bin
  etc
  include
  lib
  libexec
  share
  ssl
  x86_64-w64-mingw32

8 directories, 0 files
```
Note:  
Make sure your mingw is 64-bit(mingw64) or 32-bit (mingw32) since compilers of different bits have different binaries.  
The procedure for mingw64 is:  
i) copy the directory freeglut/include/GL into mingw64/include   
ii) copy the file freeglut/bin/x64/freeglut.dll into mingw64/bin  
iii) copythefilesfreeglut/lib/x64/libfreeglut.aandfreeglut/lib/x64/libfreeglut_static.a into mingw64/lib   
The procedure for mingw32 is:  
i) copy the directory freeglut/include/GL into mingw32/include  
ii) copy the file freeglut/bin/freeglut.dll into mingw32/bin   
iii) copythefilesfreeglut/lib/libfreeglut.aandfreeglut/lib/libfreeglut_static.aintomingw32/lib   
#### Linux (Debian/Ubuntu)
Run the following commands and then OpenGL will be installed automatically.
```
$ sudo apt-get install build-essential
$ sudo apt-get install libgl1-mesa-dev libglu1-mesa-dev freeglut3-dev
```
#### MacOS (with LLVM/Clang)
Since Gult is included in Mac OS X. There’s nothing you need to do for environment setup.

### Setup Build Environment
#### Build with CMake
Download [CMakeLists.txt](https://github.com/Gavin-Sun555/Parking-Lot-Simulation/blob/master/P3_part_2/CMakeLists.txt)


#### Build with Command Line 
Windows
```
g++ -Wall -Werror -pedantic -Wno-unused-result -Wno-deprecated-declarations -std=c++14-oOpenGLmain.cpp-lglu32-lfreeglut-lopengl32
```
Linux
```
g++ -Wall -Werror -pedantic -Wno-unused-result -Wno-deprecated-declarations -std=c++14-oOpenGLmain.cpp-lglut-lGL-lGLU
```
MacOS
```
g++ -Wall -Werror -pedantic -Wno-unused-result -Wno-deprecated-declarations -std=c++14-oOpenGLmain.cpp-frameworkOpenGL-frameworkGLUT
```

## Basic Information
### Part 1
This is a database of a parking lot. To use this program, user should follow the instructions and type in the correct information.Then an Arrival ticket or a departure ticket will appear.
The main function is responsible for collecting information and give these information to the corresponding function. In parking_lot.cpp, the class and structures are implemented. We used a class for the whole parking lot, and used structure for each parking space.

### Part 2
1. As Suggested in the project manual, we define the class Vec and the operator of this class. This makes our life easier in the following step;
2. We define the class figure with colored figure and group as its sub-class. Rectangle and other basic figures are defined as sub-class in the class colored figure. UFO and other objects are defined as sub-class of group;
3. By adjust two Vec, postion and achor, we are able to define the method of the group;
4. Finally, with the use of variable flag, and rand(); we are able to accomplish the task;
5. We use timefunc to make it refresh 25 times per second;

## Contact and Copywrite
If you have any adivices or feedbacks, please email to <yansu@umich.edu> with \[ github_feedback \]. I will reply emails within two bussiness days. Since it is a open course project written on my own, there is no copywrites restriction on use or modify the code.

