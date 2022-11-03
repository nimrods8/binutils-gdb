/* UI_FILE - a generic STDIO like output stream.
   Copyright (C) 1999-2022 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "defs.h"
#include "tui/tui-file.h"
#include "tui/tui-io.h"
#include "tui/tui-command.h"
#include "tui.h"


// NS 03/11
#include <stdio.h>
#include <stdlib.h>


void
tui_file::puts (const char *linebuffer)
{
    // NS 03.11
//    FILE *fd = fopen( "/tmp/stdout0", "a+");
//    fwrite( linebuffer, 1, strlen( linebuffer), fd);
//    fclose( fd);

//  tui_puts( "\033[1;33m");


  tui_puts (linebuffer);
  if (!m_buffered)
    tui_refresh_cmd_win ();

//  tui_puts( "\033[0m");
}

void
tui_file::write (const char *buf, long length_buf)
{
    // NS 03.11
//   FILE *fd = fopen( "/tmp/stdout0", "a+");
//    fwrite( buf, 1, length_buf, fd);
//    fclose( fd);

  tui_write (buf, length_buf);
  if (!m_buffered)
    tui_refresh_cmd_win ();
}

void
tui_file::flush ()
{
  if (m_buffered)
    tui_refresh_cmd_win ();
  stdio_file::flush ();
}
