To run: use run.sh

The shell takes care of built-in commands (cd, pwd, fg [job], jobs, exit), output redirection and pipes as a parent process, i.e. [cd ~/Desktop &] is not dealt with by the code.

The signal handler:
- Ctrl + C exits the shell completely (not sure if it's correct)
- Ctrl + Z is ignored, so you'll have to press enter again for the prompt to come up, again not sure if it's correct

fg:
- brings the JobNumber parsed by the command line prompt (1,2,3,...) to foreground, and when doing jobs(), the job bein brought to foreground will still be printed, but its status is changed from BG to FG

jobs:
- prints all the jobs running in the background, and those that were once in the background, ie fg has been called on them

rest of the builtin commands follow the instructions given by the professor

