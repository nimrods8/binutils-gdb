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
#include "tui/tui-console.h"
#include "tui/tui-io.h"
#include "reggroups.h"
#include "valprint.h"
#include "completer.h"

// NS 12/11
#include <pthread.h>
#include "tui/tui-hooks.h"


#include "gdb_curses.h"


// NS 12/11
/*thread function definition*/
void* threadFunction(void* args);
std::vector<std::string>contents;
static bool newData = false, thread_kill = false;



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

/* See tui-regs.h.  */
#if 0
void
tui_console_window::delete_data_content_windows ()
{
  for (auto &win : m_regs_content)
    win.visible = false;
}
#endif

void
tui_console_window::erase_data_content (const char *prompt)
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

/* See tui-regs.h.  */

void
tui_console_window::rerender ()
{
  #if 1
// gdb_printf( "rerender=%ld", contents.size());

  //check_and_display_highlight_if_needed ();

  if (contents.empty ())
    erase_data_content (_("[ Values Unavailable ]"));
  else
    {
      erase_data_content (NULL);
      //delete_data_content_windows ();
      display_console_from (-1);
    }
    #endif
}

// -1 = show terminal type screen, so that last line is at bottom of window
void tui_console_window::display_console_from( int fromline)
{
  int cur_y = 1, i = fromline; //fromline;

  if( newData)
  {
     newData = false;
     top_line = 0;
     m_horizontal_offset = 0;
  }

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


  if( contents.size() > height - 2 && fromline == -1)
  {
     i = contents.size() - (height - 2);
  }
  else if( fromline == -1) i = 0;

  top_line = i;

/*
  else if( contents.size() > fromline && fromline <= height - 2)
  {
     i = fromline;
  }
*/


  while (i < contents.size () && cur_y <= height - 2)
  {
        // gdb_printf( "i=%d, %d %s", i, cur_y, contents.at(i).c_str());      

      	int x_pos = 2;
        int print_width = contents.at(i).length() >= width - x_pos - 2  ? width - x_pos - 2 : contents.at(i).length();
        
        std::string s = contents.at(i);
        if( contents.at(i).length() > width - x_pos - 2 && m_horizontal_offset < contents.at(i).length())
           s = contents.at(i).substr( m_horizontal_offset, print_width - 1);
        if( m_horizontal_offset >= contents.at(i).length())
           s = " ";

        mvwaddstr (handle.get (), cur_y, x_pos, (char *) s.c_str());


        if( s.length() < width - x_pos - 2)
           waddstr( handle.get(), n_spaces ( width - x_pos - 2 - s.length()));


#if 0
        wmove (handle.get (), cur_y, 0);
        tui_puts (contents.at(i).c_str (), handle.get ());


        gdb_printf( "%s", contents.at(i).c_str());

        mvwaddnstr( handle.get(), cur_y, x, contents.at(i).c_str (), contents.at(i).length());
      
//      if (content.size () < field_width)
//         waddstr (handle, n_spaces (field_width - content.size ()));
#endif      
	    i++;		    /* Next line.  */
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

  //refresh_window ();

} // endfunc


/* Scroll the data window horizontally.  */
void
tui_console_window::do_scroll_horizontal (int num_to_scroll)
{
  if (!contents.empty ())
    {
      int offset = m_horizontal_offset + num_to_scroll;
      if( offset < 0)
	       offset = 0;

      int max = 0;
      int shown_lines = top_line + height - 2;
      if( shown_lines >= contents.size())
         shown_lines = contents.size() - 1;
      
      for( int p = top_line; p < shown_lines; p++)
      {
          if( max < contents.at(p).length())
             max = contents.at(p).length();
      }
      if( offset > max) 
         offset = max;

      m_horizontal_offset = offset;
      //refresh_window ();
      display_console_from( top_line);
    }
}





/* Scroll the data window vertically forward or backward.  */
void
tui_console_window::do_scroll_vertical (int num_to_scroll)
{

  //if (top_line >= 0)
    {
      top_line += num_to_scroll;
      if( top_line < 0) top_line = 0;
      if( top_line > contents.size() - height + 2)
          top_line = contents.size() - (height - 2);


      erase_data_content (NULL);
      //delete_data_content_windows ();
      display_console_from ( top_line);
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



#if 0

/* Scroll the data window horizontal right or left.  */
void
tui_console_window::do_scroll_horizontal (int num_to_scroll)
{
    {
      top_line += num_to_scroll;
      if( top_line < 0) top_line = 0;
      if( top_line > contents.size() - height + 2)
          top_line = contents.size() - (height - 2);


      erase_data_content (NULL);
      //delete_data_content_windows ();
      display_console_from ( top_line);
    }
}
#endif


/* Display a register in a window.  If hilite is TRUE, then the value
   will be displayed in reverse video.  */

// NS 23/10
void
tui_console_item_window::rerender (WINDOW *handle, int field_width)
{
        return;
}

        
/*thread function definition*/
void* threadFunction(void* args)
{
   char myfifo[] = "/tmp/myfifo";
   char arr1[32];
   int fd, red, pos = 0;
   std::string oneline = "";

   // Open FIFO for Read only
   fd = open(myfifo, O_RDONLY /*| O_NONBLOCK*/);
   while(1)
   {
      // Read from FIFO
      if(( red = read(fd, &arr1[pos], 1)) > 0)
      {
        if( red == 1) {
          if( arr1[pos] == '\n')
          {
             contents.push_back( oneline);
             oneline = "";
          }
          else
          {
             arr1[pos + 1] = 0;
             oneline += arr1[pos];
          }
          newData = true;
        } // endif got one byte from fifo
#if 0
         // Print the read message
         // arr1[red] = 0;
         int frm = 0;
         for( int i = 0; i < red; i++) {
                if( arr1[i] == '\n' || i == red - 1) 
                {       
                        if( i == frm) {
                                contents.push_back( " ");
                                frm = i + 1;
                                continue;
                        }
                        arr1[i] = 0;
                        contents.push_back( std::string( &arr1[frm]));
                        frm = i + 1;
                        continue;
                }
         } // endfor
#endif
/*
         std::vector<std::string>v1 = tui_hooks_split( std::string( arr1), '\n');
            // NULL at end...
         gdb_printf("split=%ld", v1.size());
         for( int i = 0; i < v1.size(); i++)
         {   

             if( v1.at(i).length() > 0)
                contents.push_back( v1.at(i));        
         // gdb_printf("User2: %s %ld\n", arr1, contents.size());
         }
*/         
      } // endif read data
    }
    close(fd);
} // endfunc thread


/*creating thread id*/
static pthread_t id;

//////////////////////////////////
void tui_console_leave( void)
{
    pthread_cancel( id);
    thread_kill = true; 
    //contents.clear();
    //newData = false; 
}


void initialize_tui_console()
{
    int ret;
    
    /*creating thread*/
    ret=pthread_create(&id,NULL,&threadFunction,NULL);
    if(ret==0){
        gdb_printf("Thread created successfully.\n");
    }
    else{
        gdb_printf("Thread not created.\n");
    }
}


/*********************************/
/* THIS FUNCTION IS CALLED       */
/* AUTOMATICALLY BY gdb/init.c   */
/*********************************/
void _initialize_tui_console ();
void
_initialize_tui_console ()
{


/*  
  struct cmd_list_element **tuicmd, *cmd;

  tuicmd = tui_get_cmd_list ();

  cmd = add_cmd ("reg", class_tui, tui_reg_command, _("\
TUI command to control the register window.\n\
Usage: tui reg NAME\n\
NAME is the name of the register group to display"), tuicmd);
  set_cmd_completer (cmd, tui_reggroup_completer);
*/
}
