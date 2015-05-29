Let's try the C demo first, compiling it with your favorite compiler:

```
$ gcc count.c -o count
```

With the demo compiled, we can now run it with DMTCP. Also, you can
feel free to try it without DMTCP first by running just `./count`.
We can tell DMTCP to checkpoint in intervals of X seconds by specifying
`-i X` as an argument. We'll try a relatively short interval of 5 seconds
for this example. Press `Ctr-C` sometime after the first 5 seconds of
running with DMTCP to kill the application.

```
$ dmtcp_launch -i 5 ./count
dmtcp_coordinator starting...
    Host: login4.stampede.tacc.utexas.edu (129.114.64.14)
    Port: 7779
    Checkpoint Interval: 5
    Exit on last client: 1
Backgrounding...
0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 ^C
```

The first thing you might notice is that a port is being used by 
the program `dmtcp_coordinator`; once more people use DMTCP on a
given node, you may find conflicts with the port. You can try again
with a different port by using the argument `-p <port number>`, and
try a random port with `-p 0`.

Note that you need to specify an absolute or relative path explicitly, i.e.,
./count not count, or you will get an error:

```
$ dmtcp_launch -i 5 count
*** ERROR:  Executable to run w/ DMTCP appears not to be readable,
***         or no such executable in path.
```

```
$ ls
ckpt_count_1a7dd72006bd7-40000-54714d2a.dmtcp  count  count.c  count.pl  
dmtcp_restart_script_1a7dd72006bd7-40000-54714d29.sh  dmtcp_restart_script.sh  README.md
```

Let's restart it:

```
$ ./dmtcp_restart_script.sh
dmtcp_coordinator starting...
    Host: login4.stampede.tacc.utexas.edu (129.114.64.14)
    Port: 7779
    Checkpoint Interval: 5
    Exit on last client: 1
Backgrounding...
17 18 19 ^C
```

The automatically generated `dmtcp_restart_script.sh` is a convenient wrapper for restarting from the last checkpoint in the current directory. You can also manually perform a restart:

```
$ dmtcp_restart ckpt_count_1a7dd72006bd7-40000-54714d2a.dmtcp 
dmtcp_coordinator starting...
    Host: login4.stampede.tacc.utexas.edu (129.114.64.14)
    Port: 7779
    Checkpoint Interval: disabled (checkpoint manually instead)
    Exit on last client: 1
Backgrounding...
17 18 19 20 21 22 23 24 25 ^C
```

**Note**, in production operations, you would also want to pass an an `-i X` option to `dmtcp_restart`,
otherwise further intervals checkpointing is disabled (note the warning above).

We can run the Perl demo very similarly to the C demo:

```
$ dmtcp_launch -i 5 perl count.pl 
dmtcp_coordinator starting...
    Host: login4.stampede.tacc.utexas.edu (129.114.64.14)
    Port: 7779
    Checkpoint Interval: 5
    Exit on last client: 1
Backgrounding...
0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 ^C
```

Again, there are caveats with the running syntax. Running an executable
script directly will not work:


```
$ dmtcp_launch -i 5 ./count.pl 
dmtcp_coordinator starting...
    Host: login4.stampede.tacc.utexas.edu (129.114.64.14)
    Port: 7779
    Checkpoint Interval: 5
    Exit on last client: 1
Backgrounding...
./count.pl: 3: ./count.pl: =: not found
./count.pl: 3: ./count.pl: $: not found
./count.pl: 5: ./count.pl: =: not found
./count.pl: 6: ./count.pl: Syntax error: "{" unexpected (expecting "do")
```

Checking for checkpoint files, we now see a Perl process file:

```
$ ls *.dmtcp
ckpt_count_1a7dd72006bd7-40000-54714d2a.dmtcp  ckpt_perl_1a7dd72006bd7-40000-54714f4f.dmtcp
```

```
$ dmtcp_restart ckpt_perl_1a7dd72006bd7-40000-54714f4f.dmtcp 
dmtcp_coordinator starting...
    Host: login4.stampede.tacc.utexas.edu (129.114.64.14)
    Port: 7779
    Checkpoint Interval: disabled (checkpoint manually instead)
    Exit on last client: 1
Backgrounding...
17 18 19 20 21 ^C
```

It will be interesting to compare the file sizes of the two checkpoint files, since
one is using an interpreter, and the other is a small program.

```
$ du -h *.dmtcp
2.9M    ckpt_count_1a7dd72006bd7-40000-54714d2a.dmtcp
4.0M    ckpt_perl_1a7dd72006bd7-40000-54714f4f.dmtcp
```

As can be seen, there is some overhead in the Perl program, but perhaps not as much as expected.



## Notes

DNS must be working properly for restart script:
```
$ ./dmtcp_restart_script_1a7dd72006bd7-40000-54714f4e.sh 
[13268] WARNING at jsocket.cpp:121 in JSockAddr; REASON='JWARNING(e == 0) failed'
     e = -5
     gai_strerror(e) = No address associated with hostname
     hostname = ubuntu
Message: No such host
Segmentation fault (core dumped)
user@ubuntu:~/CR-demos$ ping ubuntu
ping: unknown host ubuntu
```
