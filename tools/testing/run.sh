#!/bin/bash

TESTPATH="$HOME/git/miscellaneous/tools/testing"
QEMUCOMMAND="qemu-system-x86_64"
TERMINALCOMMAND="tmux split-window -v"

echo "Which kernel should I use?"
select KERN in $(ls -t $TESTPATH/kernels/); do
	break;
done

echo "Which disk image should I use as the root filesystem?"
select IMG in $TESTPATH/vm-disks/*; do
	break;
done

tmux new-window -n \"kvm\" -a "$TESTPATH/start-kvm.sh \"$QEMUCOMMAND\" \"$TESTPATH\" \"$IMG\" \"$TESTPATH/kernels/$KERN\""
