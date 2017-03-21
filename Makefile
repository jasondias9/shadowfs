all: 
	clang sshfs.c disk_emu.c

clean:
	rm -f *.out
