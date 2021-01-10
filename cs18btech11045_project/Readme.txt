PCP Project:

Files included :

* mlock.cpp
* multilock.cpp
* gtlock.cpp
* readme.txt
* Report.pdf


Execution:

Place the cpp source file in the directory same as the input file "inp-params.txt".

Open terminal and change the directory to the directory of this source file.

Execute the command for all programs:


1. To compile the program, we run the following command in the terminal
            g++ <filename>.cpp -lpthread -latomic

2. To run the program, 
            ./a.out

3. For all the programs input of the program will be taken from "inp-params.txt"

** Each lock is implemented in seperate cpp file of it's own **

4. Output of the program,

  a. Avg entry time, Worst entry time, Avg exit time, Worst exit time will be printed in the terminal

  b. Logs about Entry Request, Entry, Exit Request, Exit will be printed in an output file "output-<lock-name>.txt"

Log contents:
* Entry request time into CS by every thread.
* Entry time into CS by every thread.
* Exit request time from CS by every thread.
* Exit time from CS by every thread.
