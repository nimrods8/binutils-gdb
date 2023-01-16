*QT5:*  
From: https://github.com/Lekensteyn/qt5printers  

(1) cd /root/.gdb/
(2) git clone https://github.com/Lekensteyn/qt5printers.git
(3) add to .gdbinit or write in gdb:

python
import sys, os.path
sys.path.insert(0, os.path.expanduser('~/.gdb'))
import qt5printers
qt5printers.register_printers(gdb.current_objfile())
end


*STL:*  
(1) Download python pretty printers for gdb into /root/.gdb/stlprinters using:
	
	svn co svn://gcc.gnu.org/svn/gcc/trunk/libstdc++-v3/python

get svn using: `apt install subversion`


(2) add to .gdbinit or write in a gdb session:

python
import sys
sys.path.insert(0, '/root/.gdb/stlprinters/python')
from libstdcxx.v6.printers import register_libstdcxx_printers
register_libstdcxx_printers (None)
end


(3) if no debug symbols in your app (as you would expect), then you should
    compile a QT5 app. See `/opt/Qt/Examples/Qt-5.12.4/test`. Compiling
    this generates a main.o file, which we use the following way in gdb:

    `add-symbol-file /<QT5-test-folder>/main.o 0`

    This would load symbols from `main.o` to the current gdb context.

(4) QString and QByteArrays pretty printouts are limited to 200 (or so) chars.
    use:

    `p (&{QByteArray}$x20)->d`

    This will create a convenience variable that you can: `tui watch $xxx,600`
