# Process_battle
Two programs written in C launching multiple processes battling for survival.
## Specifications
The program padre.c makes use of the IPC mechanisms to communicate with a number of child processes determined by the first execution parameter. These child processes execute the program hijo.c receiving parameters from the parent in order to establish themselves the IPC mechanisms that allow them to coordinate the execution.
Subsequently and randomly, the created child processes will install either the defensa() function or the indefenso() function in the SIGUSR1 signal, this signal will be sent by attacking processes, resulting in the termination of the receiving process if it had previously installed the indefenso() function.
The result of the contention will be communicated by each process to the parent process through a message queue, the parent process itself being the one that terminates the child processes using the SIGTERM signal. The combat continues until one or no child processes remain alive, the result being displayed on the screen in each round of combat.
