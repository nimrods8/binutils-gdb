# Don't wrap line or the coloring regexp won't work.
set width 0

# A yellow prompt
set prompt \033[0;34mgdb>\033[0m 


set print pretty on
set print object on
set print static-members on
set print vtbl on
set print demangle on
set print asm-demangle on
set demangle-style gnu-v3
set print sevenbit-strings off
#set scheduler-locking step

tui enable
layout console
set sysroot /home/cyberark/TytoCare/rootfs3/
#target remote /dev/ttyACM0


python
import sys, os.path
sys.path.insert(0, os.path.expanduser('~/.gdb'))
import qt5printers
qt5printers.register_printers(gdb.current_objfile())
end


#python
#def Memory
#      try:
#          address = Memory.parse_as_address(self.expression)
#          inferior = gdb.selected_inferior()
#          memory = inferior.read_memory(address, self.length)
#          # set the original memory snapshot if needed
#      except gdb.error as e:
#          msg = 'Cannot access {} bytes starting at {}: {}'
#          msg = msg.format(self.length, self.expression, e)
#          return [ansi(msg, R.style_error)]

