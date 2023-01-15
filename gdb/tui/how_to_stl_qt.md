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


