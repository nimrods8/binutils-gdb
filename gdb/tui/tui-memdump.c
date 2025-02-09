/* TUI display registers in window.

   Copyright (C) 1998-2022 Free Software Foundation, Inc.

   Contributed by Hewlett-Packard Company.

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
#include "arch-utils.h"
#include "tui/tui.h"
#include "tui/tui-data.h"
#include "tui/tui-regs.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "gdbcmd.h"
#include "frame.h"
#include "regcache.h"
#include "inferior.h"
#include "target.h"
#include "tui/tui-layout.h"
#include "tui/tui-win.h"
#include "tui/tui-wingeneral.h"
#include "tui/tui-file.h"
#include "tui/tui-io.h"
#include "reggroups.h"
#include "valprint.h"
#include "completer.h"

// NS 12/11
#include <pthread.h>
#include "tui/tui-hooks.h"

#include "tui/tui-memdump.h"

#include "gdb_curses.h"


static std::vector<std::string>mem_contents;
static std::vector<gdb_byte>saved_contents;
static CORE_ADDR savedCoreAddr;
static std::string request;
static int requestLength, requestSize = 1;

#if 0
/* A subclass of string_file that expands tab characters.  */
class tab_expansion_file : public string_file
{
public:

  tab_expansion_file () = default;

  void write (const char *buf, long length_buf) override;

private:

  int m_column = 0;
};

void
tab_expansion_file::write (const char *buf, long length_buf)
{
  for (long i = 0; i < length_buf; ++i)
    {
      if (buf[i] == '\t')
	{
	  do
	    {
	      string_file::write (" ", 1);
	      ++m_column;
	    }
	  while ((m_column % 8) != 0);
	}
      else
	{
	  string_file::write (&buf[i], 1);
	  if (buf[i] == '\n')
	    m_column = 0;
	  else
	    ++m_column;
	}
    }
}
#endif
/* Get the register from the frame and return a printable
   representation of it.  */
#if 0
static std::string
tui_register_format (frame_info_ptr frame, int regnum)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);

  /* Expand tabs into spaces, since ncurses on MS-Windows doesn't.  */
  tab_expansion_file stream;

  scoped_restore save_pagination
    = make_scoped_restore (&pagination_enabled, 0);
  scoped_restore save_stdout
    = make_scoped_restore (&gdb_stdout, &stream);

  gdbarch_print_registers_info (gdbarch, &stream, frame, regnum, 1);

  /* Remove the possible \n.  */
  std::string str = stream.release ();


  if (!str.empty () && str.back () == '\n')
    str.resize (str.size () - 1);


  // NS 20/10
  const char *regname = gdbarch_register_name (gdbarch, regnum);
  gdb::observers::tui_next_reg.notify( regname,  &str);             // in tui_hooks.c
#endif
#if 0
  // NS 19/10/2022 add something
  try 
  {
      char xxx[32];
      const char *name = gdbarch_register_name (gdbarch, regnum);
      //str += std::string( name);

      //gdb_printf( "reg=%s\n", name);

      //str += std::string( name);

      if (*name != '\0')
      {
         sprintf( xxx, "*(long *)$%s", name); 
         //str += std::string( xxx);

         struct value *val = parse_and_eval( xxx);
         //str += std::string( name);
         sprintf( xxx, " - %#lx", value_as_address( val)); 
         str +=  std::string( xxx); // "%x %lu\n", *(unsigned int *)val, value_as_long( val));
      }
  }
  catch( ...)
  { 
//     str += "**";
  }

  return str;
}
#endif
#if 0
/* Get the register value from the given frame and format it for the
   display.  When changep is set, check if the new register value has
   changed with respect to the previous call.  */
static void
tui_get_register (frame_info_ptr frame,
		  struct tui_console_item_window *data, 
		  int regnum, bool *changedp)
{
  if (changedp)
    *changedp = false;
  if (target_has_registers ())
    {
      std::string new_content = tui_register_format (frame, regnum);

      if (changedp != NULL && data->content != new_content)
	*changedp = true;

      data->content = std::move (new_content);
    }
}

/* Show the registers of the given group in the data window
   and refresh the window.  */
void
tui_console_window::show_registers (const reggroup *group)
{
  if (group == 0)
    group = general_reggroup;

  if (target_has_registers () && target_has_stack () && target_has_memory ())
    {
      show_register_group (group, get_selected_frame (NULL),
			   group == m_current_group);

      /* Clear all notation of changed values.  */
      for (auto &&data_item_win : m_regs_content)
	data_item_win.highlight = false;
      m_current_group = group;
    }
  else
    {
      m_current_group = 0;
      m_regs_content.clear ();
    }

  rerender ();
}


/* Set the data window to display the registers of the register group
   using the given frame.  Values are refreshed only when
   refresh_values_only is true.  */

void
tui_console_window::show_register_group (const reggroup *group,
				      frame_info_ptr frame, 
				      bool refresh_values_only)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  int nr_regs;
  int regnum, pos;

  /* Make a new title showing which group we display.  */
  title = string_printf ("Register group: %s", group->name ());

  /* See how many registers must be displayed.  */
  nr_regs = 0;
  for (regnum = 0; regnum < gdbarch_num_cooked_regs (gdbarch); regnum++)
    {
      const char *name;

      /* Must be in the group.  */
      if (!gdbarch_register_reggroup_p (gdbarch, regnum, group))
	continue;

      /* If the register name is empty, it is undefined for this
	 processor, so don't display anything.  */
      name = gdbarch_register_name (gdbarch, regnum);
      if (*name == '\0')
	continue;

      nr_regs++;
    }

  m_regs_content.resize (nr_regs);

  /* Now set the register names and values.  */
  pos = 0;
  for (regnum = 0; regnum < gdbarch_num_cooked_regs (gdbarch); regnum++)
    {
      struct tui_console_item_window *data_item_win;
      const char *name;

      /* Must be in the group.  */
      if (!gdbarch_register_reggroup_p (gdbarch, regnum, group))
	continue;

      /* If the register name is empty, it is undefined for this
	 processor, so don't display anything.  */
      name = gdbarch_register_name (gdbarch, regnum);
      if (*name == '\0')
	continue;

      data_item_win = &m_regs_content[pos];
      if (!refresh_values_only)
	{
	  data_item_win->regno = regnum;
	  data_item_win->highlight = false;
	}
      tui_get_register (frame, data_item_win, regnum, 0);
      pos++;
    }
}

/* See tui-regs.h.  */

void
tui_console_window::display_registers_from (int start_element_no)
{
  int max_len = 0;
  for (auto &&data_item_win : m_regs_content)
    {
      int len = data_item_win.content.size ();

      if (len > max_len)
	max_len = len;
    }
  m_item_width = max_len + 1;

  int i;
  /* Mark register windows above the visible area.  */
  for (i = 0; i < start_element_no; i++)
    m_regs_content[i].y = 0;

  m_regs_column_count = (width - 2) / m_item_width;
  if (m_regs_column_count == 0)
    m_regs_column_count = 1;
  m_item_width = (width - 2) / m_regs_column_count;

  /* Now create each data "sub" window, and write the display into
     it.  */
  int cur_y = 1;
  while (i < m_regs_content.size () && cur_y <= height - 2)
    {
      for (int j = 0;
	   j < m_regs_column_count && i < m_regs_content.size ();
	   j++)
	{
	  /* Create the window if necessary.  */
	  m_regs_content[i].x = (m_item_width * j) + 1;
	  m_regs_content[i].y = cur_y;
	  m_regs_content[i].visible = true;
	  m_regs_content[i].rerender (handle.get (), m_item_width);
	  i++;		/* Next register.  */
	}
      cur_y++;		/* Next row.  */
    }

  /* Mark register windows below the visible area.  */
  for (; i < m_regs_content.size (); i++)
    m_regs_content[i].y = 0;

  refresh_window ();
}

/* See tui-regs.h.  */

void
tui_console_window::display_reg_element_at_line (int start_element_no,
					      int start_line_no)
{
  int element_no = start_element_no;

  if (start_element_no != 0 && start_line_no != 0)
    {
      int last_line_no, first_line_on_last_page;

      last_line_no = last_regs_line_no ();
      first_line_on_last_page = last_line_no - (height - 2);
      if (first_line_on_last_page < 0)
	first_line_on_last_page = 0;

      /* If the element_no causes us to scroll past the end of the
	 registers, adjust what element to really start the
	 display at.  */
      if (start_line_no > first_line_on_last_page)
	element_no = first_reg_element_no_inline (first_line_on_last_page);
    }
  display_registers_from (element_no);
}

/* See tui-regs.h.  */

int
tui_console_window::display_registers_from_line (int line_no)
{
  int element_no;

  if (line_no < 0)
    line_no = 0;
  else
    {
      /* Make sure that we don't display off the end of the
	 registers.  */
      if (line_no >= last_regs_line_no ())
	{
	  line_no = line_from_reg_element_no (m_regs_content.size () - 1);
	  if (line_no < 0)
	    line_no = 0;
	}
    }

  element_no = first_reg_element_no_inline (line_no);
  if (element_no < m_regs_content.size ())
    display_reg_element_at_line (element_no, line_no);
  else
    line_no = (-1);

  return line_no;
}


/* Answer the index first element displayed.  If none are displayed,
   then return (-1).  */
int
tui_console_window::first_data_item_displayed ()
{
  for (int i = 0; i < m_regs_content.size (); i++)
    {
      if (m_regs_content[i].visible)
	return i;
    }

  return -1;
}
#endif


#if 0
void
tui_console_window::delete_data_content_windows ()
{
  for (auto &win : m_regs_content)
    win.visible = false;
}
#endif

//////////////////////////////////
// MEMDUMP FORMATTING
void tui_memdump_window::tui_memdump_format( size_t watchSize)
{
bool newData = false;
std::string m_cont;

     gdb_byte *byte_buf = (gdb_byte *)malloc( watchSize);
     //target_read_memory( watchaddr, byte_buf, watchSize);
     char f[100];
     CORE_ADDR pos = 0;
     mem_contents.clear();     

     // evaluate the user's request
     struct value *val = parse_and_eval( request.c_str());
     if( val == NULL)
     {
            mem_contents.push_back( "Can't sync with " + request);
            return;
     }
     CORE_ADDR watchaddr = value_as_address(val);

     if( watchaddr != savedCoreAddr)
     {
        newData = true;
        savedCoreAddr = watchaddr;
        saved_contents.clear();
     }

     // NS 090225 make sure that watchAddrs is aligned with the requestSize
     


     while( true)
     {
        m_cont = "";
        try
        {
           if( target_read_memory( watchaddr + pos, byte_buf, 16) == -1)
           {
              mem_contents.push_back( "Can't sync with " + request);

              break;       
           }
        }
        catch (const gdb_exception_error &except)
        {
           return;
        }
        sprintf( f, "0x%08lx", watchaddr + pos);
        m_cont += " ";
       
        std::string fname = tui_hooks_get_name_of_rwMaps( watchaddr + pos);
        if( !fname.empty())
        {
            m_cont += tui_hooks_filename2color( fname);
        }

        m_cont += f;

        if( !fname.empty())
            m_cont += tui_hooks_filename2color( "reset");             // reset the color back to black

        m_cont += "    ";
        for( int i = 0; i < 16; i++)
        {
            if( !newData && ( gdb_byte)( saved_contents.at( i + pos)) != byte_buf[i])
               sprintf( f, "\033[0;93m%02x\033[0m ", byte_buf[i]);
            else
               sprintf( f, "%02x ", byte_buf[i]);

            if( pos + i > watchSize)
               strcpy( f, "   ");

            m_cont += f;
        } // endfor
        m_cont += "    ";

        for( int i = 0; i < 16; i++)
        {
            gdb_byte g = byte_buf[i];
            if( g < 0x20 || g > 0x7f) g = '.';

            if( !newData && ( gdb_byte)saved_contents.at( i + pos) != byte_buf[i])
               sprintf( f, "\033[0;93m%c\033[0m", g);
            else
               sprintf( f, "%c", g);

            if( pos + i > watchSize)
               strcpy( f, " ");

            m_cont += f;

            // update the saved content for next time
            if( saved_contents.size() <= i + pos)
               saved_contents.push_back( byte_buf[i]);
            else
               saved_contents[i+pos] = byte_buf[i];
        } // endfor
        // m_cont += "\n";
        mem_contents.push_back( m_cont);
        pos += 16;
        if( pos > watchSize)
           break;
     }            
} // endfunc



void
tui_memdump_window::erase_data_content (const char *prompt)
{
  werase (handle.get ());
  check_and_display_highlight_if_needed ();
  if (prompt != NULL)
    {
      int half_width = (width - 2) / 2;
      int x_pos;

      if (strlen (prompt) >= half_width)
	x_pos = 1;
      else
	x_pos = half_width - strlen (prompt);
      mvwaddstr (handle.get (), (height / 2), x_pos, (char *) prompt);
    }
    tui_wrefresh (handle.get ());
}

void
tui_memdump_window::rerender ()
{
  #if 1
// gdb_printf( "rerender=%ld", contents.size());

  //check_and_display_highlight_if_needed ();

  if( request.empty())
     title = "watch";
  else
     title = request;

  if (mem_contents.empty ())
    erase_data_content (_("[ Watch Unavailable ]"));
  else
    {
      erase_data_content (NULL);
      tui_memdump_format( requestLength);
      //delete_data_content_windows ();
      display_memdump_from ( 0);
    }
    #endif
    
   refresh_window ();

}



/* Called for each mouse click inside this window.  Coordinates MOUSE_X
   and MOUSE_Y are 0-based relative to the window, and MOUSE_BUTTON can
   be 1 (left), 2 (middle), or 3 (right).  */
void tui_memdump_window::click(int mouse_x, int mouse_y, int mouse_button)
{
     tui_set_win_focus_to ( TUI_MEMDUMP_WIN);
     TUI_MEMDUMP_WIN->rerender();
}



// -1 = show terminal type screen, so that last line is at bottom of window
void tui_memdump_window::display_memdump_from( int fromline)
{
  int cur_y = 1, i = fromline; //fromline;

#if 0
  if (highlight)
    /* We ignore the return value, casting it to void in order to avoid
       a compiler warning.  The warning itself was introduced by a patch
       to ncurses 5.7 dated 2009-08-29, changing this macro to expand
       to code that causes the compiler to generate an unused-value
       warning.  */
    (void) wstandout (handle);
#endif

  //werase (handle.get ());


  if( mem_contents.size() > height - 2 && fromline == -1)
  {
     i = mem_contents.size() - (height - 2);
  }
  else if( fromline == -1) i = 0;

  top_line = i;
  /*
  else if( contents.size() > fromline && fromline <= height - 2)
  {
     i = fromline;
  }
*/

  while (i < mem_contents.size () && cur_y <= height - 2)
  {
        // gdb_printf( "i=%d, %d %s", i, cur_y, contents.at(i).c_str());      

  	int x_pos = 4;

//        mvwaddstr (handle.get (), cur_y, x_pos, (char *) mem_contents.at(i).c_str());
          wmove( handle.get(), cur_y, x_pos);
          tui_puts( (char *) mem_contents.at(i).c_str(), handle.get());

#if 0
        wmove (handle.get (), cur_y, 0);
        tui_puts (contents.at(i).c_str (), handle.get ());


        gdb_printf( "%s", contents.at(i).c_str());

        mvwaddnstr( handle.get(), cur_y, x, contents.at(i).c_str (), contents.at(i).length());
      
//      if (content.size () < field_width)
//         waddstr (handle, n_spaces (field_width - content.size ()));
#endif      
      i++;	        /* Next line.  */
      cur_y++;		/* Next row.  */
  } // endwhile

  tui_wrefresh (handle.get ());

#if 0
  if (highlight)
    /* We ignore the return value, casting it to void in order to avoid
       a compiler warning.  The warning itself was introduced by a patch
       to ncurses 5.7 dated 2009-08-29, changing this macro to expand
       to code that causes the compiler to generate an unused-value
       warning.  */
    (void) wstandend (handle);
#endif

  refresh_window ();

} // endfunc

/* Scroll the data window vertically forward or backward.  */
void
tui_memdump_window::do_scroll_vertical (int num_to_scroll)
{

/*
    gdb_printf( "scroll %d", num_to_scroll);
*/
  //if (top_line >= 0)
    {
      top_line += num_to_scroll;
      if( top_line < 0) top_line = 0;
      if( top_line > mem_contents.size() - height + 2)
          top_line = mem_contents.size() - (height - 2);


      erase_data_content (NULL);
      //delete_data_content_windows ();

//    gdb_printf( "scroll %d, %ld %d", top_line, mem_contents.size(), height);



      display_memdump_from ( top_line);
    }

    


  #if 0
  int first_element_no;
  int first_line = (-1);

  first_element_no = first_data_item_displayed ();
  if (first_element_no < m_regs_content.size ())
    first_line = line_from_reg_element_no (first_element_no);
  else
    { /* Calculate the first line from the element number which is in
	the general data content.  */
    }

  if (first_line >= 0)
    {
      first_line += num_to_scroll;
      erase_data_content (NULL);
      delete_data_content_windows ();
      display_registers_from_line (first_line);
    }
    #endif
}

/* Display a register in a window.  If hilite is TRUE, then the value
   will be displayed in reverse video.  */

// NS 23/10
void
tui_memdump_item_window::rerender (WINDOW *handle, int field_width)
{
        return;
}




/* Implement the 'tui watch' command */

static void
tui_watch_command (const char *args, int from_tty)
{
  // struct gdbarch *gdbarch = get_current_arch ();

  if (args != NULL)
  {
      //size_t len = strlen (args);

      size_t watchLength   = 128;
      std::string ar = args;
      std::vector<std::string>args_vec;
      args_vec = tui_hooks_split( ar, ',');

      request = args;

      for( int i = 0; i < args_vec.size(); i++)
      {
          if( i == 0) request = args_vec.at(i);
          if( i == 1)
          {
             watchLength = atoi( args_vec.at(i).c_str());
             if( watchLength > 0) break;
          }
      }
      if( watchLength == 0) watchLength = 128;

      // NS 090225 add another variable to the watch line "/#
      // to specify the number of bytes per value to be displayed
      // e.g. tui watch $r1/4
      args_vec = tui_hooks_split( ar, '/');
      if( args_vec.size() > 1)
      {
          requestSize = atoi( args_vec.at(1).c_str());
          if( requestSize <= 0) requestSize = 1;
      }

      requestLength = watchLength;
      savedCoreAddr = -1;
      TUI_MEMDUMP_WIN->tui_memdump_format( watchLength);
      //rerender();

      /* Make sure the curses mode is enabled.  */
      tui_enable ();
      tui_suppress_output suppress;

      /* Make sure the register window is visible.  If not, select an
	 appropriate layout.  We need to do this before trying to run the
	 'next' or 'prev' commands.  */
      if( TUI_MEMDUMP_WIN == NULL || !TUI_MEMDUMP_WIN->is_visible ())
	        tui_console_layout ();

      //mem_contents.push_back( args);


  }
  else
  {
      gdb_printf (_("\"tui watch\" must be followed by the name of "
		    "either a register,\nor an address and then, optionally a comma and a size to display"));
  }
} // endfunc



void initialize_tui_memdump()
{
}


/*********************************/
/* THIS FUNCTION IS CALLED       */
/* AUTOMATICALLY BY gdb/init.c   */
/*********************************/
void _initialize_tui_memdump ();
void
_initialize_tui_memdump ()
{
  struct cmd_list_element **tuicmd /*, *cmd*/;

  tuicmd = tui_get_cmd_list ();

  /*cmd =*/ add_cmd ("watch", class_tui, tui_watch_command, _("\
TUI command to control the memdump window.\n\
Usage: tui watch NAME\n\
NAME is the name of the register or address to display"), tuicmd);
  //set_cmd_completer (cmd, tui_reggroup_completer);

}
