# todo
* https://pdos.csail.mit.edu/6.828/2018/homework/xv6-shell.html chanllenge questions
  * regarding sub shell, I think I can in parseexec(), first check if any parenthesis '(', if there is one, find the corresponding and use the substr within ['(', ')'] to do parseline() ( since parseline would regarding cmd base on start ptr and end ptr), done done parseline, that returned cmd is either a pipecmd/scm/other cmd
  * below failure for hw2 need to continue:
    ```
    6.828$ ls|(du;cat)
    [s:0x603180, e:0x60318c:ls|(du;cat)
    ]
    [debug subRight:0x603183, es:0x603186] (
    [debug subRight:0x603184, es:0x603186] d
    [debug subRight:0x603185, es:0x603186] u
    [s:0x603184, e:0x603186:du]
    116     .
    exec cat) failed
    6.828$ 
    ```
# steps
```
// turn on
ubuntu@ip-172-31-27-238:~/6.828/xv6-public$ make qemu-nox-gdb
// attach
ubuntu@ip-172-31-27-238:~/6.828/xv6-public$ gdb
```