How to run:
This was made to run on WSL.
First, add the program you want to run for this and go to the main.c folder
then, change line 16 to be the program name. Currently, it is set to "program_list.txt".

Second, cd to where this is located. //ignore this, is for me timtol@tlau:/mnt/c/Users/timto/CE_4348_Projects/Project2$
then, run this
gcc -o program2 main.c disk.c cpu.c memory.c scheduler.c
this will create the file called program2

Finally, use the command
./program2
this will run the program and display the output in the terminal