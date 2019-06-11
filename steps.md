* to start gdb
```
cd lab
make gdb
make qemu-nox-gdb
```

* gdb cmd
```
info reg
```

* misc
```
make grade
using make run-x or make run-x-nox. For instance, make run-hello-nox runs the hello user program.
```

* GDB cmd
```
b proc.c:297
bt // check stack trace
symbol-file _uthread
```