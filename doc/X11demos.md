## DMTCP

### (Did not work in virtualbox demo)

A problem with these directions not working is that `xterm` may have been running (which for reasons unknown prevents DMTCP
from functioning), but I have stopped testing due to various difficulties for now.

These directions assume TightVNC is installed (as suggested by the DMTCP website), but may work with other VNC implementations.

vncviewer invokes an Xserver to display the graphics. So, the recipe is: 

```
dmtcp_launch vncserver :1
vncviewer localhost:1
```

Now run an X application of your choice, so long as it doesn't involve audio, video, or hardware 3d.
For our demo we used a screen saver. Start the screen saver daemon, then lock the screen to keep
it running:

```
xscreensaver &
xscreensaver-command -lock
```


To checkpoint, kill the vncviewer (close the VNC window), and then checkpoint and kill the VNC server:

```
dmtcp_command --checkpoint
```

This will apparently kill the vncserver, but you can manually kill it as follows (with no checkpoint):

```
vncserver -kill :1

```

Later, to restart from a checkpoint, do:

```
./dmtcp_restart_script.sh
vncviewer localhost:1 # to re-connect the graphics
```

Since this did not work with TightVNC (at least on my setup), and the checkpoint causes the vncserver to stop, 
iterative checkpointing may not currently be a possibility. 
