**README for GNU development tools**

The main idea of this fork is to add Ida-like functionality of comments and function renames for the layout asm in the gdb tui.


In order to build gdb for aarch64 (or other arm targets) and x86_64 targets 
on Ubuntu you need to:

(1) Load the following packages:
```
apt install libncurses5-dev
apt install texinfo
apt install build-essential
apt-get install libgmp-dev
```
(2) configure:   
`./configure --enable-targets=aarch64-linux-gnu,arm-linux-gnueabi,x86_64-pc-linux-gnu --enable-tui --with-python --disable-ld --disable-gas --disable-sim`

(3) make only gdb:  
`make all-gdb` 
and  
`make install` in order to install all gdb components, including gdb-python and TUI support


This process generates a `gdb` executable.

New functions added:
**tui comment**:  
`tui comment set <text>`: to add a comment at the current PC  
`tui comment save`: to save all comments (and renames) to $HOME/.comments file  
`tui comment clear`: to clear all in-memory comments  

**tui rename**:  
`tui rename set <text>`: renames the current PC call address to <text>. Will also show as a comment at that address  
`tui rename save`: saves all renames, similar to `tui comment save`.	  

Use `info comments` to display all stored comments.


A new layout has been added. It is experimental! use:
`layout ontop`
This new layout allows to expand calls/branches without actually jumping into them. By clicking the opcode
a new floating window is opened that shows the disassembled jump-to. 

New break point type:
`tui break <where>+/-<val>`: set a break point given a symbol. For example: 
`tui break main+11` places a break point at the address of main + 11.

	
Notes:
1. You may need to copy the compiled gdb python libraries to /usr/share/gdb:  
`cp -R ./gdb/data-directory/* /usr/share/gdb/`  
	

2DO
---
* add break at opcodes by arch
* add tui break into comments file to save/load automatically
* read the list of renames/comments from ida and set it automatically in gdb tui

-------------------------------------------------------------------------------------------------------------------------------
This directory contains various GNU compilers, assemblers, linkers, 
debuggers, etc., plus their support routines, definitions, and documentation.

If you are receiving this as part of a GDB release, see the file gdb/README.
If with a binutils release, see binutils/README;  if with a libg++ release,
see libg++/README, etc.  That'll give you info about this
package -- supported targets, how to use it, how to report bugs, etc.

It is now possible to automatically configure and build a variety of
tools with one command.  To build all of the tools contained herein,
run the ``configure'' script here, e.g.:

	./configure 
	make

	
To install them (by default in /usr/local/bin, /usr/local/lib, etc),
then do:
	make install

(If the configure script can't determine your type of computer, give it
the name as an argument, for instance ``./configure sun4''.  You can
use the script ``config.sub'' to test whether a name is recognized; if
it is, config.sub translates it to a triplet specifying CPU, vendor,
and OS.)

If you have more than one compiler on your system, it is often best to
explicitly set CC in the environment before running configure, and to
also set CC when running make.  For example (assuming sh/bash/ksh):

	CC=gcc ./configure
	make

A similar example using csh:

	setenv CC gcc
	./configure
	make

Much of the code and documentation enclosed is copyright by
the Free Software Foundation, Inc.  See the file COPYING or
COPYING.LIB in the various directories, for a description of the
GNU General Public License terms under which you can copy the files.

REPORTING BUGS: Again, see gdb/README, binutils/README, etc., for info
on where and how to report problems.
