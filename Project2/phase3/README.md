 Extend the shell functionality to handle background processes and job control, allowing users to run commands in the background and manage them with built-in job control commands.

Before executing the program:
Input 'make' in bash to compile the program.


HOW TO Execute:
******JOB INDEX STARTS WITH '0' (NOT THE '1')*******

Background Process Management:

Append & to a command to run it in the background.
Use jobs to list all background jobs.

Use bg %<job_number> to continue a stopped job in the background.

Use fg %<job_number> to bring a job to the foreground.

Use kill %<job_number> to terminate a job.
Commands Supporting Background Execution:

Any command can be run in the background by appending &.

Example: 
'cat largefile.txt | grep "search string" &'

Exiting the shell:
Input "quit" or "exit" to quit myshell.