# SeaShell
A command interpreter with REPL loops, dispatch tables, packet building and etc written in C


This is CShell. A simple learning project of mine. It is a command interpreter that includes many commands (20):
Here are the commands of CShell
```
Here are the commands of CShell
[1]  hello    Greets you - usage: hello
[2]  goodbye  Says goodbye - usage: goodbye
[3]  clean    Wipes the shell - usage: clean
[4]  clear    Wipes the shell - usage: clear
[5]  cls      Wipes the shell - usage: cls
[6]  echo     Echoes a message - usage: echo <message>
[7]  cwd      Display the directory you're at - usage: cwd
[8]  mkdir    Creates a directory - usage: mkdir <directory_name>
[9]  mkfile   Creates a file - usage: mkfile <file_name.format>
[10] chdir    Changes directory - usage: chdir <directory>
[11] read     Reads file content - usage: read <file_name>
[12] ldir     Lists files/folders in a directory - usage: ldir [directory_name]
[13] calc     Built-in calculator, e.g. calc 2+3*6/4 - usage: calc <expression>
[14] help     Shows this list - usage: help
[15] mist     Enter 'mist help' for more - usage: mist <arg>
[16] netfo    Gives you info about your internet connection - usage: netfo
[17] ping     Pings the destination - usage: ping <address>
[18] get      Pulls files from a GitHub repository link - usage: get <GITHUB_link>
[19] games    Play CShell games - usage: games
[20] sudo     sudo help for more - usage: sudo <action>
```

Though, I did not want to resort to just `system("command")` so.. i wrote everything from scratch
You can contribute to this project and write some new code if you like.
