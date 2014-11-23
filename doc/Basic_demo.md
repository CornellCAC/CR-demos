Let's try the C demo first, compiling it with your favorite compiler:

```
user@ubuntu:~/CR-demos$ gcc count.c -o count
```

user@ubuntu:~/CR-demos$ dmtcp_checkpoint count
*** ERROR:  Executable to run w/ DMTCP appears not to be readable,
***         or no such executable in path.

count
user@ubuntu:~/CR-demos$ dmtcp_checkpoint ./count
dmtcp_coordinator starting...
    Host: ubuntu (127.0.0.1)
    Port: 7779
    Checkpoint Interval: disabled (checkpoint manually instead)
    Exit on last client: 1
Backgrounding...
0 1 2 3 4 5 6 7 8 ^C
user@ubuntu:~/CR-demos$ ls
count  count.c  count.pl  README.md
user@ubuntu:~/CR-demos$ ls -las
total 40
 4 drwxrwxr-x  3 brandon brandon 4096 Nov 23 02:52 .
 4 drwxr-xr-x 20 brandon brandon 4096 Nov 23 02:50 ..
12 -rwxrwxr-x  1 brandon brandon 8660 Nov 23 02:52 count
 4 -rw-rw-r--  1 brandon brandon  174 Nov 23 02:50 count.c
 4 -rwxrwxr-x  1 brandon brandon  115 Nov 23 02:50 count.pl
 4 drwxrwxr-x  8 brandon brandon 4096 Nov 23 02:50 .git
 4 -rw-rw-r--  1 brandon brandon  288 Nov 23 02:50 .gitignore
 4 -rw-rw-r--  1 brandon brandon   64 Nov 23 02:50 README.md
user@ubuntu:~/CR-demos$ man dmtcp_checkpoint 
user@ubuntu:~/CR-demos$ dmtcp_checkpoint -i 5 ./count
dmtcp_coordinator starting...
    Host: ubuntu (127.0.0.1)
    Port: 7779
    Checkpoint Interval: 5
    Exit on last client: 1
Backgrounding...
0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 ^C
user@ubuntu:~/CR-demos$ ls
ckpt_count_1a7dd72006bd7-40000-54714d2a.dmtcp  count  count.c  count.pl  dmtcp_restart_script_1a7dd72006bd7-40000-54714d29.sh  dmtcp_restart_script.sh  README.md
user@ubuntu:~/CR-demos$ dmtcp_restart ckpt_count_1a7dd72006bd7-40000-54714d2a.dmtcp 
dmtcp_coordinator starting...
    Host: ubuntu (127.0.0.1)
    Port: 7779
    Checkpoint Interval: disabled (checkpoint manually instead)
    Exit on last client: 1
Backgrounding...
17 18 19 20 21 22 23 24 25 ^C
user@ubuntu:~/CR-demos$ 
user@ubuntu:~/CR-demos$ dmtcp_checkpoint -i 5 ./count.pl 
dmtcp_coordinator starting...
    Host: ubuntu (127.0.0.1)
    Port: 7779
    Checkpoint Interval: 5
    Exit on last client: 1
Backgrounding...
./count.pl: 3: ./count.pl: =: not found
./count.pl: 3: ./count.pl: $: not found
./count.pl: 5: ./count.pl: =: not found
./count.pl: 6: ./count.pl: Syntax error: "{" unexpected (expecting "do")
user@ubuntu:~/CR-demos$ which perl
/usr/bin/perl
user@ubuntu:~/CR-demos$ dmtcp_checkpoint -i 5 perl count.pl 
dmtcp_coordinator starting...
    Host: ubuntu (127.0.0.1)
    Port: 7779
    Checkpoint Interval: 5
    Exit on last client: 1
Backgrounding...
0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 ^C
user@ubuntu:~/CR-demos$ ls *.dmtcp
ckpt_count_1a7dd72006bd7-40000-54714d2a.dmtcp  ckpt_perl_1a7dd72006bd7-40000-54714f4f.dmtcp
user@ubuntu:~/CR-demos$ dmtcp_restart ckpt_perl_1a7dd72006bd7-40000-54714f4f.dmtcp 
dmtcp_coordinator starting...
    Host: ubuntu (127.0.0.1)
    Port: 7779
    Checkpoint Interval: disabled (checkpoint manually instead)
    Exit on last client: 1
Backgrounding...
17 18 19 20 21 ^C
user@ubuntu:~/CR-demos$ 
user@ubuntu:~/CR-demos$ du -h *.dmtcp
2.9M    ckpt_count_1a7dd72006bd7-40000-54714d2a.dmtcp
4.0M    ckpt_perl_1a7dd72006bd7-40000-54714f4f.dmtcp
user@ubuntu:~/CR-demos$ ./dmtcp_restart_script_1a7dd72006bd7-40000-54714f4e.sh 
[13268] WARNING at jsocket.cpp:121 in JSockAddr; REASON='JWARNING(e == 0) failed'
     e = -5
     gai_strerror(e) = No address associated with hostname
     hostname = ubuntu
Message: No such host
Segmentation fault (core dumped)
user@ubuntu:~/CR-demos$ ping ubuntu
ping: unknown host ubuntu
