**README for GNU development tools**
 
The main idea of this fork is to add Ida-like functionality of comments and function renames for the layout asm in the gdb tui.


In order to build gdb for aarch64 (or other arm targets) and x86_64 targets 
on Ubuntu you need to:

(1) Load the following packages:
```
apt install libncurses5-dev
apt install texinfo
apt install build-essential
apt install libgmp-dev
apt install python3-dev
apt install flex bison
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
A new layout has been added: `layout console`. See below.  


**tui break**:  
New break point type:  
`tui break <where>+/-<val>`: set a break point given a symbol. For example:   
`tui break main+11` places a break point at the address of main + 11.  
`tui break apply` to re-apply the breaks that were previously set using `tui break` and saved with `tui comment save`  
Note that `tui break` now supports **regexp** for the function name, through `info func`. It will find all matching functions and place breakpoints on them.  
Use `tui comment save` to save the tui breakpoints to the `~/.comments` file, so you can reapply them in future sessions.  
	
	

Another new layout split has been added:  
`layout console` with a console window which shows the STDOUT of the inferior,  
when running the inferior from the beginning, i.e. not by attaching to it.  


**tui watch**:  
New watch mechanism.   
`tui watch <expression>,[size to watch]`  
NOTE the comma between expression and size. If no size is give 128 bytes are shown.  


**tui skip**:  
Skips the next opcode  
  

**Better mouse interaction**  
Click the mouse inside a window to set it into focus

**tui console layout**
A new layout has been added, called `console`.  
Use `layout console` to set it.  
  

**tui info**
Display any of the `info` commands on the console window. For example, use:
`tui info break` 
to show breakpoints on the console window, or
`tui info thread`
to show current threads execution picture


	  
Notes:
1. You may need to install the compiled gdb python libraries to /usr/share/gdb using:  
`cd ./gdb/data-directory`  
`make install`
which installed the missing python scripts.

2. We encountered problems when loading a debug info symbol file (using the symbol_file command) with only .abs file. Hardware breapoints
did not set correctly due to breakpoint kind defaulting to 4. It is now automatically defaults to 2 (hardware breakpoint) and to set it back
to the original value, use 
`target bp_kind off`



**How to add source code highlightint?**  
(1) add the line:
`pkg_config_prog_path="/usr/bin/pkg-config"`

 before this in the gdb/configure file:  
```

if test "${enable_source_highlight}" != "no"; then
  { $as_echo "$as_me:${as_lineno-$LINENO}: checking for the source-highlight library" >&5
$as_echo_n "checking for the source-highlight library... " >&6; }
  if test "${pkg_config_prog_path}" = "missing"; then
    { $as_echo "$as_me:${as_lineno-$LINENO}: result: no - pkg-config not found" >&5
$as_echo "no - pkg-config not found" >&6; }
    if test "${enable_source_highlight}" = "yes"; then
      as_fn_error $? "pkg-config was not found in your system" "$LINENO" 5
    fi
```

(2) add the libsource-highlight package:  
`apt install libsource-highlight-dev`  
  
(2) add `--enable-source-highlight` to the configure line  
(3) run `./configure` with all flags
(4) run `make all-gdb`




2DO
---
* add break at opcodes by arch
* read the list of renames/comments from ida and set it automatically in gdb tui
* perfect decompiler

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
