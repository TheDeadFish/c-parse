call egcc.bat
gcc %CCFLAGS2% c-parse.cc -c -g
gcc %CCFLAGS2% test.cc c-parse.o -lstdshit
copy /Y c-parse.h %PROGRAMS%\local\include
ar -rcs  %PROGRAMS%\local\lib32\libexshit.a c-parse.o
