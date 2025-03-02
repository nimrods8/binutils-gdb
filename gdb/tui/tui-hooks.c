/* GDB hooks for TUI.

   Copyright (C) 2001-2022 Free Software Foundation, Inc.

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
#include "symtab.h"
#include "inferior.h"
#include "command.h"
#include "bfd.h"
#include "symfile.h"
#include "objfiles.h"
#include "target.h"
#include "gdbcore.h"
#include "gdbsupport/event-loop.h"
#include "event-top.h"
#include "frame.h"
#include "breakpoint.h"
#include "ui-out.h"
#include "top.h"
#include "observable.h"
#include "source.h"

#include "arch-utils.h"
#include "frame.h"
#include "disasm.h"

// NS 0801
#include "gdbcmd.h"

#include <unistd.h>
#include <fcntl.h>

// NS 230225
#include <iostream>
#include <limits.h>
#include <libgen.h>
#include <dirent.h>
#include <fnmatch.h>
#include <sys/stat.h>
#include <iostream>
#include <sstream>     // For stringstream


#include "tui/tui.h"
#include "tui/tui-hooks.h"
#include "tui/tui-data.h"
#include "tui/tui-layout.h"
#include "tui/tui-io.h"
#include "tui/tui-regs.h"
#include "tui/tui-win.h"
#include "tui/tui-stack.h"
#include "tui/tui-winsource.h"
#include "tui/tui-location.h"
#include "tui/tui-decomp.h"

//NS 01/11
#include "tui/tui-disasm.h"
#include "tui/tui-console.h"
#include "tui/tui-memdump.h"
#include <sstream>
#include "gdb_curses.h"
#include <iostream>

// NS 09/01
#include <unordered_map>
#include <signal.h>



// NS 16/10
static void tui_hooks_comment_all_command (const char *, int);
static void tui_hooks_call_rename_command (const char *, int);
static void tui_hooks_break_command( const char *, int);
static void tui_hooks_skip_command( const char *arg, int from_tty);
bool tui_hooks_serialize_comments( bool);
bool tui_hooks_deserialize_comments( void);
std::string toHexFromDecimal(long long t);
static bool tui_hooks_check_if_in_filesMap( CORE_ADDR add_reg);
static void tui_hooks_file_command( const char *arg, int from_tty);
// Function to calculate the SHA-1 hash of a string
static std::string tui_hooks_calculate_sha1(const std::string& input);
static void tui_decompiler_finished_signal( int sig);
static void tui_hooks_goto_command( const char *arg, int from_tty);
static void tui_hooks_parse_sal_file( void);


/* Data from one mapping from /proc/PID/maps.  */
#define MAX_FN_TEXT	256
#define MAX_COMMENT_LEN 256

// struct to hold the mapping of all executable memory regions
struct mapping
{
  ULONGEST addr;
  ULONGEST endaddr;
  gdb::string_view permissions;
  ULONGEST offset;
  gdb::string_view device;
  ULONGEST inode;

  /* This field is guaranteed to be NULL-terminated, hence it is not a
     gdb::string_view.  */
  char filename[MAX_FN_TEXT];
};

std::vector<mapping> m_execMaps;
std::vector<mapping> m_rwMaps;

// NS 050225
std::vector<segments_lookup> m_filesMap;

static bool newSolibsLoaded;         // true when new solibs have been loaded
static bool commentsTainted;
static bool breaksTainted;

typedef enum
{ 
   TUI_TYPE_COMMENT,
   TUI_TYPE_RENAME,
   TUI_TYPE_BREAKPOINT,
   TUI_TYPE_END
} enum_tui_type;

// struct for all the comments
struct comment
{
   enum_tui_type type;
   char text[MAX_COMMENT_LEN];
   char filename[MAX_FN_TEXT];
   ULONGEST unbased_addr;
};
std::vector<comment> m_comments;


/* Service function for corefiles and info proc.  */
static mapping
read_mapping (const char *line)
{
  struct mapping mapping;
  const char *p = line;

  mapping.addr = strtoulst (p, &p, 16);
  if (*p == '-')
    p++;
  mapping.endaddr = strtoulst (p, &p, 16);

  p = skip_spaces (p);
  const char *permissions_start = p;
  while (*p && !isspace (*p))
    p++;
  mapping.permissions = {permissions_start, (size_t) (p - permissions_start)};

  mapping.offset = strtoulst (p, &p, 16);

  p = skip_spaces (p);
  const char *device_start = p;
  while (*p && !isspace (*p))
    p++;
  mapping.device = {device_start, (size_t) (p - device_start)};

  mapping.inode = strtoulst (p, &p, 10);

  p = skip_spaces (p);
  memcpy( mapping.filename, p, MAX_FN_TEXT - 1);
  mapping.filename[MAX_FN_TEXT - 1] = (char)NULL;

  return mapping;
}


static void
tui_new_objfile_hook (struct objfile* objfile)
{
  if (tui_active)
    tui_display_main ();
}

/* Prevent recursion of deprecated_register_changed_hook().  */
static bool tui_refreshing_registers = false;


/* Observer for the register_changed notification.  */
static void
tui_register_changed (frame_info_ptr frame, int regno)
{
  frame_info_ptr fi;

  if (!tui_is_window_visible (DATA_WIN))
    return;

  /* The frame of the register that was changed may differ from the selected
     frame, but we only want to show the register values of the selected frame.
     And even if the frames differ a register change made in one can still show
     up in the other.  So we always use the selected frame here, and ignore
     FRAME.  */
  fi = get_selected_frame (NULL);
  if (!tui_refreshing_registers)
    {
      tui_refreshing_registers = true;
      TUI_DATA_WIN->check_register_values (fi);
      tui_refreshing_registers = false;
    }
}

/* Breakpoint creation hook.
   Update the screen to show the new breakpoint.  */
static void
tui_event_create_breakpoint (struct breakpoint *b)
{
  tui_update_all_breakpoint_info (nullptr);
}

/* Breakpoint deletion hook.
   Refresh the screen to update the breakpoint marks.  */
static void
tui_event_delete_breakpoint (struct breakpoint *b)
{
  tui_update_all_breakpoint_info (b);
}

static void
tui_event_modify_breakpoint (struct breakpoint *b)
{
  tui_update_all_breakpoint_info (nullptr);
}


/* This is set to true if the next window refresh should come from the
   current stack frame.  */
static bool from_stack;

/* This is set to true if the next window refresh should come from the
   current source symtab.  */
static bool from_source_symtab;

/* Refresh TUI's frame and register information.  This is a hook intended to be
   used to update the screen after potential frame and register changes.  */

static void
tui_refresh_frame_and_register_information ()
{
  if (!from_stack && !from_source_symtab)
    return;

  target_terminal::scoped_restore_terminal_state term_state;
  target_terminal::ours_for_output ();

  if (from_stack && has_stack_frames ())
    {
      frame_info_ptr fi = get_selected_frame (NULL);

      /* Display the frame position (even if there is no symbols or
	 the PC is not known).  */
      bool frame_info_changed_p = tui_show_frame_info (fi);

      /* Refresh the register window if it's visible.  */
      if (tui_is_window_visible (DATA_WIN)
	  && (frame_info_changed_p || from_stack))
	{
	  tui_refreshing_registers = true;
	  TUI_DATA_WIN->check_register_values (fi);
	  tui_refreshing_registers = false;
	}
    }
  else if (!from_stack)
    {
      /* Make sure that the source window is displayed.  */
      tui_add_win_to_layout (SRC_WIN);

      struct symtab_and_line sal = get_current_source_symtab_and_line ();
      tui_update_source_windows_with_line (sal);
    }
}

/* Dummy callback for deprecated_print_frame_info_listing_hook which is called
   from print_frame_info.  */

static void
tui_dummy_print_frame_info_listing_hook (struct symtab *s,
					 int line,
					 int stopline, 
					 int noerror)
{
}

/* Perform all necessary cleanups regarding our module's inferior data
   that is required after the inferior INF just exited.  */

static void
tui_inferior_exit (struct inferior *inf)
{
   try
   {
      tui_console_leave();
   }
   catch( ...)
   {
      return;
   }
  /* Leave the SingleKey mode to make sure the gdb prompt is visible.  */
  tui_set_key_mode (TUI_COMMAND_MODE);
  tui_show_frame_info (0);
  tui_display_main ();
}

/* Perform all necessary cleanups regarding our module's inferior data
   that is required after the inferior INF just exited.  */

static void
tui_inferior_attach (struct inferior *inf)
{
   try
   {
      m_filesMap = tui_hooks_get_info_files("");
   }
   catch( ...)
   {
      m_filesMap.clear();
      return;
   }
}



// NS 17/10
/**
 re-sorts the m_execMaps according to current PC
**/
static void tui_hooks_sort_maps( CORE_ADDR pc)
{
  struct mapping m;
  
  //gdbarch *gdbarch = target_gdbarch();
  //gdb_printf( "PC= %s", paddress( gdbarch, pc));
 
  for( int i = 0; i < m_execMaps.size(); i++)
  {
     m = m_execMaps.at( i);
// DEBUG::
/*
		  gdb_printf ("  %18s %18s %10s %10s  %-5.*s  %s\n",
			      paddress (gdbarch, m.addr),
			      paddress (gdbarch, m.endaddr),
			      hex_string (m.endaddr - m.addr),
			      hex_string (m.offset),
			      (int) m.permissions.size (),
			      m.permissions.data (),
			      m.filename);
*/

     if( m.addr <= pc && m.endaddr >= pc && i > 0)
     {
        // DEBUG:: gdb_printf( "In here: %s\n", m.filename);
        m_execMaps.erase( m_execMaps.begin() + i);
        m_execMaps.insert( m_execMaps.begin(), m);
        //break;
     }
  } //endfor
}

/**
 returns the index of pc inside m_execMaps
 or -1 if not found
**/
static int tui_hooks_get_index_of_maps( CORE_ADDR pc)
{
  struct mapping m;
  // DEBUG:: struct gdbarch *gdbarch = target_gdbarch();

  for( int i = 0; i < m_execMaps.size(); i++)
  {
     m = m_execMaps.at( i);

     if( m.addr <= pc && m.endaddr >= pc)
     {
        // DEBUG:: gdb_printf( "In here: %s\n", m.filename);
        return i;
     }
  } //endfor
  return -1;
}


inline bool ends_with(std::string const & value, std::string const & ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

inline bool ends_with_string(std::string const& str, std::string const& what) {
    return what.size() <= str.size()
        && str.find(what, str.size() - what.size()) != str.npos;
}
/**
 returns the index of pc inside m_execMaps
 or -1 if not found

 bool searchAll - if true, searches all entries in m_execMaps,
                  if false, searches only first entry (entry 0)
**/
static int tui_hooks_get_index_of_maps_by_filename( std::string &filename, bool searchAll)
{
  struct mapping m;
  // DEBUG:: struct gdbarch *gdbarch = target_gdbarch();

  for( int i = 0; i < m_execMaps.size(); i++)
  {
     m = m_execMaps.at( i);

     if( m.filename == filename)
     {
        // DEBUG:: gdb_printf( "In here: %s\n", m.filename);
        return i;
     }
     // NS 08/01/23 check if filename is just local... allow it for now
     // so, if there are no slashes in the registered filename - then
     // this is probably imported line and all filenames ending with that should be OK for us
     if( filename.find("/") == std::string::npos)
     {
       // Crazy Debug::: gdb_printf( "fn=%s len=%ld %s", filename.c_str(), filename.length(), m.filename);

        if( ends_with_string( m.filename, filename))
        {
           // overly zelous debugging.... gdb_printf( "Found ida!");
           return i;        
        }
     }
     if( !searchAll)
        break;
  } //endfor
  return -1;
} // endfunc




/* Observer for the before_prompt notification.  */
static bool prevVisibilty = false;

static void
tui_before_prompt (const char *current_gdb_prompt)
{
  // NS 16/10
  // tui_hooks_sort_maps( tui_location.addr ());

  tui_refresh_frame_and_register_information ();
  from_stack = false;
  from_source_symtab = false;


  if( TUI_DISASMOT_WIN != nullptr && prevVisibilty != TUI_DISASMOT_WIN->isVisible)
  {
      prevVisibilty = TUI_DISASMOT_WIN->isVisible;
      //NS 11/11 tui_apply_current_layout( true);
  }
  
  if( TUI_CONSOLE_WIN != nullptr)
  {
     TUI_CONSOLE_WIN->rerender();
  }
  if( TUI_MEMDUMP_WIN != nullptr)
  {
     TUI_MEMDUMP_WIN->rerender();
  }
}



/**
 returns the type of memory block or the filenae (if
 so lib or executable are found) of addr inside m_rwMaps
 or -1 if not found
**/
std::string tui_hooks_get_name_of_rwMaps( CORE_ADDR addr)
{
  struct mapping m;
  // DEBUG:: struct gdbarch *gdbarch = target_gdbarch();

  for( int i = 0; i < m_rwMaps.size(); i++)
  {
     m = m_rwMaps.at( i);

     if( m.addr <= addr && m.endaddr >= addr)
     {
        // DEBUG:: gdb_printf( "In here: %s\n", m.filename);
        return std::string( m.filename);
     }
  } //endfor
  return std::string();
}




/***************************************************************************************/
// reads /proc/pid/maps from target and parses info into m_execMaps vector
static void 
tui_hooks_read_maps_info( void)
{
char filename[MAX_FN_TEXT];


  // NS 15/10/22
  long pid = current_inferior()->pid;
  sprintf( filename, "/proc/%lu/maps", pid);
  gdb::unique_xmalloc_ptr<char> map = target_fileio_read_stralloc( NULL, filename);
  // debug::   gdb_printf( "hhh: %s\n\n\n", (char *)map.get());
  
  struct gdbarch *gdbarch = target_gdbarch();

  m_execMaps.clear();
  m_rwMaps.clear();        // NS 16/11

  newSolibsLoaded = true;

  if( map == NULL)
  {
     return;
  }


  /* Translate PC address.  */
  // redeclared struct gdbarch *gdbarch = tui_location.gdbarch ();
  CORE_ADDR addr = tui_location.addr ();
  std::string pc_out (gdbarch
		      ? paddress (gdbarch, addr)
		      : "??");
  // const char *pc_buf = pc_out.c_str ();
  // int pc_width = pc_out.size ();

	  char *saveptr, *line;
	  for (line = strtok_r (map.get (), "\n", &saveptr);
	       line;
	       line = strtok_r (NULL, "\n", &saveptr))
	    {
	      struct mapping m = read_mapping (line);

              // has x in permissions?
              if( m.permissions.find ('x') !=  gdb::string_view::npos)
              {
                  // PC in here???
                  if( m.addr <= addr && m.endaddr >= addr)
                  {
                     // DEBUG:: gdb_printf( "In here: %s\n", m.filename);
                     m_execMaps.insert( m_execMaps.begin(), m);
                  }
                  else
                     m_execMaps.push_back( m);

/*
		  gdb_printf ("  %18s %18s %10s %10s  %-5.*s  %s\n",
			      paddress (gdbarch, m.addr),
			      paddress (gdbarch, m.endaddr),
			      hex_string (m.endaddr - m.addr),
			      hex_string (m.offset),
			      (int) m.permissions.size (),
			      m.permissions.data (),
			      m.filename);
*/
              } // endif exec permission

               // has r/w in permissions?
              if( m.permissions.find ('r') !=  gdb::string_view::npos ||
                   m.permissions.find ('w') !=  gdb::string_view::npos)
              {
                     m_rwMaps.push_back( m);
              } // endif exec permission
	    } // endfor
} // endfunc



/* Observer for the normal_stop notification.  */
static void
tui_normal_stop (struct bpstat *bs, int print_frame)
{
  from_stack = true;
}


/* This module's normal_stop observer.  */
static void
tui_hooks_solib_loaded_observer( struct so_list *so)
{
  /* The inferior execution has been resumed, and it just stopped
     again.  This means that the list of shared libraries may have
     evolved.  Reset our cached value.  */
  
     tui_hooks_read_maps_info();  
}


/* Observer for user_selected_context_changed.  */

static void
tui_context_changed (user_selected_what ignore)
{
  from_stack = true;
}

/* Observer for current_source_symtab_and_line_changed.  */

static void
tui_symtab_changed ()
{
   from_source_symtab = true;

   // NS 150225
#if 0
   try
   {
      m_filesMap = tui_hooks_get_info_files("");
   }
   catch( ...)
   {
      m_filesMap.clear();
      return;
   }
#endif
}

/*
//find_memory_region_ftype mem_region_callback;
static int mem_region_callback( CORE_ADDR a, long unsigned int size, int read, int write, int exec, int mod, bool tagged, void *data)
{
   return 1;
}
*/
// NS


// SERIALIZING
bool tui_hooks_serialize_comments( bool onlyShow)
{
  char fn[100], text[1024];

  char *dir = getenv( "HOME");
  sprintf( fn, "/%s/.comments", dir);
  // DEBUG:: gdb_printf( "%s\n", fn);

  int fd = 0, sz;

  if( !onlyShow)
  {
     fd = open( fn, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR);
     if( fd == 0)
       return false;
  }
  sprintf( text, "%s\t%10s\t%-50s\t%s\n", "type", "addr", "file", "comment");

  if( onlyShow)
     gdb_printf( "%s", text);
  else
  {
     sz = write( fd, text, strlen( text));
     if( sz <= 0)
     {
       close( fd);
       return false;
     }
  } // endif

  struct gdbarch *gdbarch = target_gdbarch();

  for( int i = 0; i < m_comments.size(); i++)
  {
     // remove excess LF at the end of the text
     int l1 = strlen( m_comments.at(i).text);
     if( m_comments.at(i).text[l1 - 1] == '\n' && !onlyShow)
        m_comments.at(i).text[l1 - 1] = 0;


     sprintf( text, "%d\t%10s\t%s\t%s\n", 
                    (int)m_comments.at(i).type,
                    paddress( gdbarch, m_comments.at(i).unbased_addr),
                    m_comments.at(i).filename,
                    m_comments.at(i).text);

     if( onlyShow)
     {
         int takefrom = 0;
         if( strlen( m_comments.at(i).filename) > 50)
         {
            takefrom = strlen( m_comments.at(i).filename)  - 50;
         }
         sprintf( text, "%d\t%10s\t%-50s\t%s", 
                    (int)m_comments.at(i).type,
                    paddress( gdbarch, m_comments.at(i).unbased_addr),
                    &m_comments.at(i).filename[takefrom],
                    m_comments.at(i).text);      
         if( text[strlen( text) - 1] != '\n')
            gdb_printf( "%s\n", text);           
         else
            gdb_printf( "%s", text);
     }
     else
     {
        sprintf( text, "%d\t%10s\t%s\t%s\n", 
                    (int)m_comments.at(i).type,
                    paddress( gdbarch, m_comments.at(i).unbased_addr),
                    m_comments.at(i).filename,
                    m_comments.at(i).text);
        sz = write( fd, text, strlen( text));
        if( sz <= 0) { close( fd); return false; }
     }
   } // endfor
   if( !onlyShow)
      close( fd);

   if( onlyShow)
      gdb_printf( "\n");
   
   return true;
}




// DESERIALIZING
bool tui_hooks_deserialize_comments( void)
{
  char fn[100], text[1024];
  //unsigned int unb;
  CORE_ADDR unb;
  int tempi;

  char *dir = getenv( "HOME");
  sprintf( fn, "/%s/.comments", dir);
  // NS 030123 DEBUG: gdb_printf( "%s\n", fn);

  FILE *file = fopen( fn, "rt");
  if( file == NULL)
    return false;

  char *rd = fgets( text, sizeof( text), file);		// read the heaeder line

  // sprintf( text, "inx\taddr\tfile\tcomment\n");

  // struct gdbarch *gdbarch = target_gdbarch();
  struct comment com;

  m_comments.clear();
  
  while( rd != 0)
  {
     rd = fgets( text, sizeof( text), file);

     if( rd != 0 && strlen( rd) > 10)
     {
         //std::string tmpstr = std::string( text);
         //std::vector<std::string> vecstr = tui_hooks_split( tmpstr, '\t');
    
         char tmps[256];

         sscanf( text, "%d\t0x%s\t%s\t%s\n", 
                    &tempi, //&com.type,
//                    (unsigned int *)&unb, //com.unbased_addr, 
                    tmps,
                    com.filename,
                    com.text);

	 unb = strtoull( tmps, NULL, 16);
         gdb_printf( "Comments vector unb = %s %lx", tmps, unb);



         char *token;
         const char s[2] = "\t";
         token = strtok( text, s);  	        // type
         token = strtok( NULL, s);		// address
         token = strtok( NULL, s);		// filename
         token = strtok( NULL, s);		// text
         
         if( token != NULL)
         {
            memcpy( com.text, token, sizeof( com.text));
            com.text[sizeof( com.text) - 1] = 0;
         }
         else
         {
            strcpy( com.text, "ERROR!");
         }
/*
         tempi = (int)strtol( vecstr.at(0).c_str(), NULL, 10);
         unb   = strtoul( vecstr.at(1).substr(2).c_str(), NULL, 16);
         memcpy( com.filename, vecstr.at(3).c_str(), sizeof( com.filename));
         memcpy( com.text, vecstr.at(4).c_str(), sizeof( com.text));
*/
         com.type = (enum_tui_type)tempi;
         com.unbased_addr = (CORE_ADDR)unb;
         m_comments.push_back( com);

         commentsTainted = true;

     } // endif
   } // endwhile
   fclose( file);
   
   // DEBUG... gdb_printf( "Comments vector length = %lu", m_comments.size());
   return true;
}



static void
info_comments_command (const char *args, int from_tty)
{
  tui_hooks_serialize_comments( true);
}



std::vector<std::string> tui_hooks_split(const std::string& s, char seperator)
{
   std::vector<std::string> output;

    std::string::size_type prev_pos = 0, pos = 0;

    while((pos = s.find(seperator, pos)) != std::string::npos)
    {
        std::string substring( s.substr(prev_pos, pos-prev_pos) );

        output.push_back(substring);

        prev_pos = ++pos;
    }

    output.push_back(s.substr(prev_pos, pos-prev_pos)); // Last word

    return output;
}


std::string toHexFromDecimal(long long t) 
{
    std::stringstream is;
    is << std::hex << t;
    return is.str();
}






/// @brief tui_hooks_skip_command
/// @param arg - arguments
/// @param from_tty 
//
//
static void tui_hooks_skip_command( const char *arg, int from_tty)
{
     if( tui_location.addr() == 0)
     {
         gdb_printf( "Can't skip, target is not running!\n");
         return;
     }
     else /* The target is executing.  */
     {
         CORE_ADDR pc = regcache_read_pc (get_current_regcache ());
         // DEBUG>>> gdb_printf( "read_pc=%lx", pc);
         CORE_ADDR next_addr = tui_disasm_find_next_opcode( pc);
         next_addr = tui_disasm_find_next_opcode( next_addr);        // we need to find the next-next one.... don't we?
         
         regcache_write_pc ( get_current_regcache (), next_addr);
         from_stack = true;
         from_source_symtab = true;
         tui_refresh_frame_and_register_information();
     }
} // endfunc


/// @brief tui_hooks_goto_command
/// @param arg - arguments
/// @param from_tty 
//
//
static void tui_hooks_goto_command( const char *arg, int from_tty)
{
   CORE_ADDR func_addr;
   std::string name = std::string( arg);
   
   struct value *val0 = NULL;
   try
   {
      val0 = parse_and_eval( name.c_str());
      func_addr = value_as_address( val0);
   }   
   catch( const gdb_exception_error &except)
   {
      // try now with "info func"
      std::string resultsstr = "";
      std::string infof = "info func " + name;
      execute_command_to_string( resultsstr, infof.c_str(), 0, false);
      
      
      std::vector<std::string>v1 = tui_hooks_split( resultsstr, '\n');
      char func_name[256];

      for( int l = 0; l < v1.size(); l++)
      {  
           // don't allow too big function names here...
           if( v1[l].length() > 256)
              continue;

           if( sscanf( v1[l].c_str(), "0x%lx  %s", &func_addr, func_name) == 2)
           {
               // DEBUG: gdb_printf( "Found %lx %s\t", func_addr, func_name);

               // this is how it looks like: 0x0000ffff8fd32000  QrCodeManager::IsTytoWifiCode()@plt


           } // endif
      } // endfor
  } // end try

  struct gdbarch *gdbarch = target_gdbarch();
  tui_update_source_windows_with_addr( gdbarch, func_addr);
} // endfunc




/// @brief This is a util helper function which requests "info func"
///        with a given func name or regexp and returns a vector of addreses and func names
/// @param args regexp or straight func name to look for
/// @return std::vector of struct functions
///
std::vector<functions_lookup> tui_hooks_get_info_func( std::string args)
{
   std::vector<functions_lookup> retVec;
 
   std::string resultsstr = "";
   std::string infof = "info func " + args /* + "*"*/; // the asterisk is just complicating things...
   try
   {
      execute_command_to_string( resultsstr, infof.c_str(), 0, false);
   }
   catch( ...)
   {
      // bail out...
      return retVec;
   }
   std::vector<std::string>v1 = tui_hooks_split( resultsstr, '\n');
   CORE_ADDR func_addr;
   char func_name[256];


   // DEBUG: gdb_printf( "%s", resultsstr.c_str());

//   bool found = false;
   for( auto vline : v1) // int l = 0; l < v1.size(); l++)
   {  
        // don't allow too big function names here...
        if( vline.length() > sizeof( func_name) - 1)
           continue;

        // DEBUG NS 12/1:   gdb_printf( "%s", vline.c_str());

        if( sscanf( vline.c_str(), "0x%lx  %s", &func_addr, func_name) == 2)
        {
            // this is how it looks like: 0x0000ffff8fd32000  QrCodeManager::IsTytoWifiCode()@plt

            functions_lookup lookup;
            lookup.func_addr = func_addr;
            lookup.func_name = func_name;
            retVec.push_back( lookup);
            //found = true;
        } // endif
   } // endfor
   return retVec;
} // end helper function



/**
 * @brief 	This function checks the m_filesMap vector which is read below
 *		to see if add_reg is located inside any of its addresses
 *
 */
static bool tui_hooks_check_if_in_filesMap( CORE_ADDR add_reg)
{
    bool found = false;

    for( int i = 0; i < m_filesMap.size(); i++)
    {
	if( m_filesMap[i].from_addr <= add_reg && m_filesMap[i].to_addr >= add_reg)
        {
           found = true;
           break;
        }
    } // endfor
    return found;
} // endfunc



// ===========================================================================
// NS 2025 !
/// @brief This is a util helper function which requests "info files"
//
//
/// @param args regexp or straight func name to look for
/// @return std::vector of struct functions
///
std::vector<segments_lookup> tui_hooks_get_info_files( std::string args)
{
   std::vector<segments_lookup> retVec;
 
   std::string resultsstr = "";
   std::string infof = "info files " + args;

   // NS 050225 
   // gdb_printf( "[hooks] info files");

   try
   {
      execute_command_to_string( resultsstr, infof.c_str(), 0, false);
   }
   catch( ...)
   {
      // bail out...
      return retVec;
   }
   std::vector<std::string>v1 = tui_hooks_split( resultsstr, '\n');
   CORE_ADDR from_addr, to_addr;
   char func_name[256];

   // NS 050225
   // gdb_printf( "[hook] lookup1 %lu\n", v1.size());
    

//   bool found = false;
   int lline = 0;
   for( auto vline : v1)
   {
        // don't allow too big function names here...
        if( vline.length() > sizeof( func_name) - 1)
           continue;
        lline++;
        if( lline < 3) continue;

        //        0x20006c2c - 0x20006c80 is .ram_function
        if( sscanf( vline.c_str(), "        0x%lx - 0x%lx is %s", &from_addr, &to_addr, func_name) >= 2)
        {
            segments_lookup lookup;
            lookup.from_addr = from_addr;
            lookup.to_addr   = to_addr;
            lookup.func_name = func_name;
            retVec.push_back( lookup);
            //found = true;
        } // endif
   } // endfor
   // NS 050225
   // gdb_printf( "[tui-hooks] Info files found %lu segments\n", retVec.size());
   return retVec;
} // end helper function



/// @brief static helper to run break points upon request with all sorts of goodies
/// @param arg 
/// @param from_tty 
static void tui_hooks_run_breakpoints_helper( std::string sarg)
{
   int fnd1 = sarg.find( "+");
   int fnd2 = sarg.find( "-");
   if( fnd1 == std::string::npos && fnd2 == std::string::npos)
   {
      gdb_printf( "tui break: Need to add + or -");
      return;
   }
   int v = strtoul( sarg.substr( fnd1 > 0 ? fnd1 + 1 : fnd2).c_str(), NULL, 10);
   std::string name = sarg.substr( 0, (fnd1 > 0 ? fnd1 : fnd2));
   
   struct value *val0 = NULL;
   try
   {
      val0 = parse_and_eval( name.c_str());
   }   
   catch( const gdb_exception_error &except)
   {
      // try now with "info func"
      std::string resultsstr = "";
      std::string infof = "info func " + name;
      execute_command_to_string( resultsstr, infof.c_str(), 0, false);
      
      
      std::vector<std::string>v1 = tui_hooks_split( resultsstr, '\n');
      CORE_ADDR func_addr;
      char func_name[256];

      bool found = false;
      for( int l = 0; l < v1.size(); l++)
      {  
           // don't allow too big function names here...
           if( v1[l].length() > 256)
              continue;

           if( sscanf( v1[l].c_str(), "0x%lx  %s", &func_addr, func_name) == 2)
           {
               // DEBUG: gdb_printf( "Found %lx %s\t", func_addr, func_name);

               // this is how it looks like: 0x0000ffff8fd32000  QrCodeManager::IsTytoWifiCode()@plt

               // now add what you asked...
               func_addr += (fnd1 > 0 ? v : -v);


               char cmd[64];
               sprintf( cmd, "b *0x%s", toHexFromDecimal( func_addr).c_str());
               gdb_printf( "<%s> ", cmd);

               execute_command( cmd, false);
               found = true;

           } // endif
      } // endfor

      // should add new comment?
      if( found)
      {
         struct comment com;
         if( sarg.length() < sizeof( com.text))
         {
            com.filename[0] = '*';
            com.filename[1] = (char)NULL;
            memcpy( com.text, sarg.c_str(), MAX_COMMENT_LEN - 1);
            com.text[MAX_COMMENT_LEN - 1] = (char)NULL;
            com.unbased_addr = 0L;
            com.type = TUI_TYPE_BREAKPOINT;
            m_comments.push_back( com);
            breaksTainted = true;
         } // endif size OK
      } // endif found
      return;
   } // endof catch
   
   CORE_ADDR add = value_as_address( val0) + (fnd1 > 0 ? v : -v);
   char cmd[64];
   sprintf( cmd, "b *0x%s", toHexFromDecimal( add).c_str());

   //////////////////////////////////////////////////
   // save tui breakpoints to comments
      struct comment com;
      // DEBUG... gdb_printf( "set comments\n");
      memcpy( com.filename, m_execMaps.at(0).filename, MAX_FN_TEXT - 1);
      com.filename[MAX_FN_TEXT - 1] = (char)NULL;
      memcpy( com.text, name.c_str(), MAX_COMMENT_LEN - 1);
      com.text[MAX_COMMENT_LEN - 1] = (char)NULL;
      CORE_ADDR pc = add;
      com.unbased_addr = pc - m_execMaps.at(0).addr;

      //struct gdbarch *gdbarch = target_gdbarch();
      //gdb_printf( "Set: %s %s %s\n", paddress( gdbarch, pc), paddress( gdbarch, m_execMaps.at(0).addr), paddress( gdbarch, com.unbased_addr));
      com.type = TUI_TYPE_BREAKPOINT;

      m_comments.push_back( com);
      breaksTainted = true;
   //////////////////////////////////////////////////


   execute_command( cmd, false);
} // endfunc helper tui breaks




/// @brief tui_hooks_break_command
/// @param arg - arguments
/// @param from_tty 
//
//
// add "tui break apply" in order to apply the tui breakpoints

static void tui_hooks_break_command( const char *arg, int from_tty)
{
   std::string sarg = std::string( arg);
   std::string suffix = "apply"; 
   
   // --- APPLY SAVED BREAK POINTS ---
   if( sarg.rfind( suffix) == sarg.size() - suffix.size())
   {
        // assuming we are always running code inside .text or .so libs
        // CORE_ADDR based = addr_pc + m_execMaps.at(0).addr;
        for( struct comment com : m_comments)
        {
           if( com.type == TUI_TYPE_BREAKPOINT)
           { 
//              char cmd[300];
//              sprintf( cmd, "b *%s", m_comments.at(j).text);
//              execute_command( cmd, false);
              // DEBUG gdb_printf( "tui break at");
              sarg = com.text;
              tui_hooks_run_breakpoints_helper( sarg);
           } // endif
        } // endfor
        return;
   }
   else
      tui_hooks_run_breakpoints_helper( sarg);
} // endfunc


static std::string hashed_filename;
/************************************************************************/
/// @brief calls the file command then runs the Ghidra analysis tool
/// @param arg 
/// @param from_tty 
/************************************************************************/
static void tui_hooks_file_command( const char *arg, int from_tty)
{
   std::string sarg = std::string( arg), dir_path;

   // try now with "file ..."
   std::string infof = "file " + sarg;

   execute_command( infof.c_str(), false);

   std::string _hashproj = "gdb_" + tui_hooks_calculate_sha1( sarg);
   std::string hashfn = "/tmp/" + _hashproj;
   hashed_filename = hashfn;

   char path[PATH_MAX];
   size_t count = readlink("/proc/self/exe", path, sizeof(path) - 1);

   if( count != -1) 
   {
      path[count] = '\0'; // Null-terminate the string
      dir_path = std::string( dirname(path)) + "/tui/";

      DIR *dir = opendir( dir_path.c_str());
      if (!dir) 
      {
          gdb_printf( "[tui-file] opendir error");
          return;
      }

      struct dirent *entry;
      struct stat statbuf;

      if( hashfn.length() > 18)
      {
         std::string fpa = "shell rm -rf " + hashfn + "*";
         execute_command( fpa.c_str(), false);
      }

      if( stat( hashfn.c_str(), &statbuf) != 0)
      {
         std::string fpa = "shell mkdir " + hashfn;
         execute_command( fpa.c_str(), false);
      }

      while ((entry = readdir(dir)) != nullptr) 
      {
         if( fnmatch("ghidra_*", entry->d_name, 0) == 0) 
         { // Matches "ghidra_*"
            char tmp_buffer[32];

            std::string full_path = dir_path + "/" + entry->d_name;
            sprintf( tmp_buffer, "%d", getpid());

            if( stat(full_path.c_str(), &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) 
            {
                 full_path = "shell " + full_path + "/support/analyzeHeadless /tmp " + _hashproj + 
                              " -import " + sarg + " -scriptPath " + full_path + "/support/ -postScript GhidraDecompiler2.java " + 
                               hashfn + " " + tmp_buffer + " > /dev/null";
                 // gdb_printf( "[tui-hooks] %s", full_path.c_str());
                 execute_command( full_path.c_str(), false);
            } // endif
          } // endif
      } // endwhile
      closedir(dir);
   } else {
      return;
   }
#if 0 // debug
   // debug symtab:
   struct symtab_and_line sal = get_current_source_symtab_and_line ();
   gdb_printf( "[H] sal.space=%p, sal.symbol=%p, line=%d, pc=%lu, end=%lu", sal.pspace, sal.symbol, sal.line, sal.pc, sal.end);
   struct symtab *nextsal = sal.symtab;

   while( nextsal != NULL)
   {
       gdb_printf( "[H] symtab.filename=%s", nextsal->filename);
   }
#endif // debug
} // endfunc helper tui breaks




// Function to calculate the SHA-1 hash of a string
static std::string tui_hooks_calculate_sha1(const std::string& input) 
{
    std::hash<std::string> hash_fn;
    size_t hash = hash_fn( input);
    
    // Convert the size_t hash value to a hexadecimal string
    std::stringstream ss;
    ss << std::hex << hash;
    return ss.str();  // Return the hexadecimal string
} // endfunc


#if 1
/************************************************************************/
/// @brief Parses the sal.rx/rxx file created by Ghidra decompiler
/// @param arg 
/// @param from_tty 
/************************************************************************/
static void tui_hooks_parse_sal_file( void)
{
  char text[256], *rd;
  struct symtab_and_line sal = get_current_source_symtab_and_line ();
  
  FILE *file = fopen( "/tmp/ghidra2/sal.rxx", "rt");
  if( file == NULL)
    return;

  //char filename[256];
  unsigned long addr, addr2;
  int lineNo, inx = 0;
  struct symtab *prev_s = sal.symtab;

  while( true)
  {
     rd = fgets( text, sizeof( text), file);

     if( rd != 0)
     {
         struct symtab *s = new symtab();
         if( inx == 0)
         {
            sal.symtab = s;
            prev_s = s;
         }
         else 
         { 
            prev_s->next = s;
            prev_s = s;
         }
         std::string tmps = std::string( text);
         std::vector<std::string> vec = tui_hooks_split( tmps, ':');
         sscanf( vec[0].c_str(), "0x%lx 0x%lx", 
                    &addr, //&com.type,
                    &addr2);

         s->fullname = (char *)vec[1].c_str();
         lineNo = atoi( vec[2].c_str()); 

         inx++;
         gdb_printf( "[H] %d:%d %s " , inx, lineNo, s->fullname);

      } // endif have data
      else 
         break;
   } // endwhile
   fclose( file);
}
#endif


static void
tui_hooks_comment_all_command (const char *arg, int from_tty)
{
//   gdb_printf( "args=%s, %d %d %d %d\n", arg, (int)arg[3], (int)arg[4], (int)arg[5], (int)arg[6]);
   if( !memcmp( "save", arg, 4))
   {
       gdb_printf( "comments saved\n");
       tui_hooks_serialize_comments( false);
       return;
   }
   if( !memcmp( "set ", arg, 4))
   {
      struct comment com;
      // DEBUG... gdb_printf( "set comments\n");

      CORE_ADDR pc = tui_location.addr ();
      if( m_execMaps.size() > 0)
      {
         memcpy( com.filename, m_execMaps.at(0).filename, MAX_FN_TEXT - 1);
         com.unbased_addr = pc - m_execMaps.at(0).addr;
      }
      else
      {
         memcpy( com.filename, "unknown", 8);
         com.unbased_addr = pc - /*m_execMaps.at(0).addr*/ 0;
      }
      com.filename[MAX_FN_TEXT - 1] = (char)NULL;
      memcpy( com.text, &arg[4], MAX_COMMENT_LEN - 1);
      com.text[MAX_COMMENT_LEN - 1] = (char)NULL;

      //struct gdbarch *gdbarch = target_gdbarch();
      //gdb_printf( "Set: %s %s %s\n", paddress( gdbarch, pc), paddress( gdbarch, m_execMaps.at(0).addr), paddress( gdbarch, com.unbased_addr));

      com.type = TUI_TYPE_COMMENT;

      m_comments.push_back( com);

      commentsTainted = true;

      // DEBUG... gdb_printf( "Comments vector length = %lu\n", m_comments.size());

   }
   if( !memcmp( "show", arg, 4))
   {
       gdb_printf( "comments\n");
       tui_hooks_serialize_comments( true);
       return;
   }
}


static void
tui_hooks_call_rename_command (const char *arg, int from_tty)
{
//   gdb_printf( "args=%s, %d %d %d %d\n", arg, (int)arg[3], (int)arg[4], (int)arg[5], (int)arg[6]);
   if( !memcmp( "save", arg, 4))
   {
       gdb_printf( "save renames\n");
       tui_hooks_serialize_comments( false);
       return;
   }
   if( !memcmp( "set ", arg, 4))
   {
      struct comment com;
      gdb_printf( "set renames\n");

      bool term_out = true; //source_styling && gdb_stdout->can_emit_style_escape ();
      string_file gdb_dis_out (term_out);
      CORE_ADDR pc = tui_location.addr ();
      struct gdbarch *gdbarch = target_gdbarch();
 
      try
      {
	  gdb_print_insn (gdbarch, pc, &gdb_dis_out, NULL);
      }
      catch (const gdb_exception_error &except)
      {
	  /* If PC points to an invalid address then we'll catch a
	     MEMORY_ERROR here, this should stop the disassembly, but
	     otherwise is fine.  */
	  if (except.error != MEMORY_ERROR)
	    throw;
	  return;
      }

      /* Capture the disassembled instruction.  */
      std::string insn = gdb_dis_out.release ();
      gdb_printf( "instruction: %s", insn.c_str());
      
      // find "0x" at the beginning of the address after the call or jump insturction
      int zero_x = insn.find( "0x");
      int end_x = insn.length();
      std::string hexstr = "0";
      
      // Found?
      if( zero_x >= 0)
      {
         int eee   = insn.find( " ", zero_x);
         if( eee != std::string::npos)
             end_x = eee;
         
         hexstr = insn.substr( zero_x, end_x); 
      }
      CORE_ADDR call_to = 0;

      try 
      {
         call_to  = strtoul( hexstr.c_str(), NULL, 16);
      }
      catch( ...)
      {
      }
      
      gdb_printf( "Set: %s %s %s\n", paddress( gdbarch, pc), paddress( gdbarch, call_to), paddress( gdbarch, com.unbased_addr));

      // if could not find an address in the instruction part
      // use the PC as the call_to and rename that PC
      if( call_to == 0)
         call_to = pc;

      int inx = tui_hooks_get_index_of_maps( call_to); 
      if( inx >= 0)
      {
         memcpy( com.filename, m_execMaps.at(inx).filename, MAX_FN_TEXT - 1);
         com.filename[MAX_FN_TEXT - 1] = (char)NULL;

         memcpy( com.text, &arg[4], MAX_COMMENT_LEN - 1);
         com.text[MAX_COMMENT_LEN - 1] = (char)NULL;

         com.unbased_addr = call_to - m_execMaps.at(inx).addr;


      //struct gdbarch *gdbarch = target_gdbarch();
      //gdb_printf( "Set: %s %s %s\n", paddress( gdbarch, pc), paddress( gdbarch, m_execMaps.at(0).addr), paddress( gdbarch, com.unbased_addr));


         com.type = TUI_TYPE_RENAME;

         m_comments.push_back( com);

         commentsTainted = true;

         // DEBUG: gdb_printf( "Comments vector length = %lu\n", m_comments.size());
      } //endif

      // ???no filename when reversing???
      else
      {
         memcpy( com.filename, "unknown", 8);
         com.filename[MAX_FN_TEXT - 1] = (char)NULL;

         memcpy( com.text, &arg[4], MAX_COMMENT_LEN - 1);
         com.text[MAX_COMMENT_LEN - 1] = (char)NULL;

         com.unbased_addr = call_to;
         com.type = TUI_TYPE_RENAME;

         m_comments.push_back( com);

         commentsTainted = true;
      } // endelse
   }
}


////////////////////////////////////////////////////////////////////////////
/// Hash table basics
class Hashtable {
    std::unordered_map<CORE_ADDR, std::string> htmap;

public:
    void put( CORE_ADDR key, std::string value) {
            htmap[key] = value;
    }

    const std::string get( CORE_ADDR key) {
            return htmap[key];
    }
    void clear()
    {
            htmap.clear();
    }

};
////////////////////////////////////////////////////////////////////////////
/*
int main() {
    ht.put("Bob", "Dylan");
    int one = 1;
    ht.put("one", &one);
    std::cout << (char *)ht.get("Bob") << "; " << *(int *)ht.get("one");
}
*/
static Hashtable ht_comments, ht_renames;




static void
tui_process_next_instruction( CORE_ADDR cur_inst_addr, std::string *str_comment, std::string *inst)
{
  // NS? tui_hooks_sort_maps( tui_location.addr());

  if( cur_inst_addr == 0L)
  { 
      // place current disassembled file in first position
      tui_hooks_sort_maps( tui_location.addr());

      if( newSolibsLoaded || commentsTainted)
      {
         commentsTainted = false;
         newSolibsLoaded = false;
         
         ht_comments.clear();
         ht_renames.clear();


         gdb_printf( "solib=%d, taint=%d, size=%ld", newSolibsLoaded, commentsTainted, m_comments.size());

         static int hooks_axa = -1;

         // prepare a hashmap of all comments, renames and breaks for this filename
         for( int i = 0; i < m_comments.size();  i++)
         {
              std::string filen = m_comments.at(i).filename;
              
              if( m_comments.at(i).type == TUI_TYPE_COMMENT && 
                  ( filen == "unknown" || tui_hooks_get_index_of_maps_by_filename( filen, false) == 0))
              { 
                 //gdb_printf( "com unbased=%lx\n", m_comments.at(j).unbased_addr);
                 //struct gdbarch *gdbarch = target_gdbarch();
                 //gdb_printf( "Set: %s %s %s\n", paddress( gdbarch, addr_pc), paddress( gdbarch, m_execMaps.at(0).addr), paddress( gdbarch, m_comments.at(j).unbased_addr));

                 CORE_ADDR execMapAddr = 0L;
                 if( filen != "unknown")
		    execMapAddr = m_execMaps.at(0).addr;

                 ht_comments.put( m_comments.at(i).unbased_addr + execMapAddr, m_comments.at(i).text);
               } // endif comments

               else if( m_comments.at(i).type == TUI_TYPE_RENAME)
               {
                  if( true /*hooks_axa == -1 || before_prompt_called*/)
                  {
                     // std::string filen = m_comments.at(i).filename;
                     // DEBUG gdb_printf( "Flne=%s, i = %d\n", filen.c_str(), i);
                     if(( filen == "unknown" || (hooks_axa = tui_hooks_get_index_of_maps_by_filename( filen, true)) >= 0))
                     {
 		         CORE_ADDR pow = m_comments.at(i).unbased_addr;
                         if( filen != "unknown" && hooks_axa >= 0)
                            pow += m_execMaps.at( hooks_axa).addr;

                         ht_renames.put( pow, m_comments.at(i).text);

                         gdb_printf( "rename %lx", pow);

   //                      std::string froms = paddress( gdbarch, pow);
   //                      if(( where = inst->find( froms)) > 0)
   //                      {
   //                         inst->replace( where, froms.length(), m_comments.at(i).text);
   //                         break;
                     } //endif
                  } // endif
               } // end else
          } // endfor
       } // new solibs or comments/renames loaded? 
       return;
  } // endif

#if 0
  // NS: this should be executed only after a before_prompt was called and not everytime
  // the observer notify is called...
  int where;
  struct gdbarch *gdbarch = target_gdbarch();
  static int hooks_axa = -1;
  // **************
  // --- RENAME ---
  // **************
     for( int i = 0; i < m_comments.size();  i++)
     {

         if( m_comments.at(i).type == TUI_TYPE_RENAME)
         {
            if( true /*hooks_axa == -1 || before_prompt_called*/)
            {
               // std::string filen = m_comments.at(i).filename;
               // DEBUG gdb_printf( "Flne=%s, i = %d\n", filen.c_str(), i);
               if(( hooks_axa = tui_hooks_get_index_of_maps_by_filename( filen, true)) >= 0)
               {
                   CORE_ADDR pow = m_comments.at(i).unbased_addr + m_execMaps.at( hooks_axa).addr;

                       //  gdb_printf( "decompile20 %lx", pow);

                   std::string froms = paddress( gdbarch, pow);
                   if(( where = inst->find( froms)) > 0)
                   {
                      inst->replace( where, froms.length(), m_comments.at(i).text);
                      break;
                   }
               } //endif
            } 
            /**
            else 
            { 
                   CORE_ADDR pow = m_comments.at(i).unbased_addr + m_execMaps.at( hooks_axa).addr;
                   std::string froms = paddress( gdbarch, pow);
                   if(( where = inst->find( froms)) > 0)
                   {
                      inst->replace( where, froms.length(), m_comments.at(i).text);
                      break;
                   }
            }
            **/
         } // endif
      } // endfor


  // **************
  // --- COMMENTS ---
  // **************
  /* Translate PC address.  */
  // redeclared struct gdbarch *gdbarch = tui_location.gdbarch ();
  CORE_ADDR addr_pc = cur_inst_addr; //tui_location.addr ();
  // ret_comment[0] = (char)NULL;
  if( m_execMaps.size() > 0)
  {
     // assuming we are always running code inside .text or .so libs
     // CORE_ADDR based = addr_pc + m_execMaps.at(0).addr;
     for( int j = 0; j < m_comments.size(); j++)
     {
        std::string filen = m_comments.at(j).filename;

        if( m_comments.at(j).type == TUI_TYPE_COMMENT && 
              tui_hooks_get_index_of_maps_by_filename( filen, false) == 0)
        /*!strcmp( m_comments.at(j).filename, m_execMaps.at(0).filename))*/
        { 
           //gdb_printf( "com unbased=%lx\n", m_comments.at(j).unbased_addr);
           //struct gdbarch *gdbarch = target_gdbarch();
           //gdb_printf( "Set: %s %s %s\n", paddress( gdbarch, addr_pc), paddress( gdbarch, m_execMaps.at(0).addr), paddress( gdbarch, m_comments.at(j).unbased_addr));

           if( m_comments.at(j).unbased_addr + m_execMaps.at(0).addr == addr_pc)
           {
               // DEBUG:: gdb_printf( "Found comment: %s\n", m_comments.at(j).text);
               // int len = strlen( m_comments.at(j).text);
               // if( len > max_len) len = max_len - 1;

               // memcpy( ret_comment, m_comments.at(j).text, len);
               // ret_comment[len] = (char)NULL;
               str_comment->insert( str_comment->size(), m_comments.at(j).text); 
           }
        } // endif
     } // endfor
   } // endif
#else
   std::string addings;
   addings = ht_comments.get( cur_inst_addr);

   if( !addings.empty())
   {
       str_comment->insert( str_comment->size(), addings); 
   }

   addings = ht_renames.get( cur_inst_addr);
   if( !addings.empty())
   {
       int where;
       struct gdbarch *gdbarch = target_gdbarch();

       std::string froms = paddress( gdbarch, cur_inst_addr);
       if(( where = inst->find( froms)) > 0)
       {
          inst->replace( where, froms.length(), addings);
       }
   }
#endif
} // endfunc


////////////////////////////////////////////////////////////////////
// turns filename string to color
std::vector<std::string>FileNames = { "[stack]", "[heap]" };

std::string tui_hooks_filename2color( std::string filename)
{
   if( filename == "reset") return( std::string( "\033[0m")); // reset back to black?
   if( filename == "[stack]") return( std::string( "\033[92m")); // green
   if( filename == "[heap]") return( std::string( "\033[94m")); // blue
   return( std::string("\033[91m"));
}


/**
███    ██ ███████ ██   ██ ████████     ██████  ███████  ██████  ██ ███████ ████████ ███████ ██████  
████   ██ ██       ██ ██     ██        ██   ██ ██      ██       ██ ██         ██    ██      ██   ██ 
██ ██  ██ █████     ███      ██        ██████  █████   ██   ███ ██ ███████    ██    █████   ██████  
██  ██ ██ ██       ██ ██     ██        ██   ██ ██      ██    ██ ██      ██    ██    ██      ██   ██ 
██   ████ ███████ ██   ██    ██        ██   ██ ███████  ██████  ██ ███████    ██    ███████ ██   ██ 
*/
static void
tui_hooks_next_reg( const char *regname, std::string *reg_str, std::string *reg_value)
{
char xyz[64];

// QString:
// ------------------------
// |refcnt    |    size   |
// |capacity  |   alloc   |
// |       offset         |
// | wide char string     |
// ------------------------
// QByteArray - same as QString just with c_str instead of wide char string

// QList:
// ------------------------
// |refcnt    |   alloc   |
// |begin     |   end     |
// |  [0] QString *       |
// |  [1] QString *       |
// |  [...]
// ------------------------
//
// QList: size = d.end - d.begin


// NS 040225 test
  regname = 0;


  try
  {
      if( regname != 0 && *regname != '\0')
      {
         // NS 18/11 check with m_rwMaps to see where this register is pointintg at
         sprintf( xyz, "$%s", regname); 
         
         // NS 020225 make sure you can actually read from the "memory" address in the register
         const target_section_table *table = target_get_section_table (current_inferior ()->top_target ());

         struct value *val9 = parse_and_eval( xyz);
         CORE_ADDR addr_reg = value_as_address( val9);

	 // NS 150225
         bool isInFiles = tui_hooks_check_if_in_filesMap( addr_reg);
         if (table == nullptr && !isInFiles)
         {
            reg_value->insert( 0, xyz);
            return;
         }

         bool _found = false;

         for( const target_section &sec : *table)
         {
            if( sec.addr <= addr_reg && sec.endaddr >= addr_reg)
            {
                _found = true;
               break;
            }
         } // endfor
         if( !_found)
            return;

         //gdb_printf( "Ref %s = %lx", xyz, value_as_address( val9));
         std::string xstr = tui_hooks_get_name_of_rwMaps( value_as_address( val9));
         if( !xstr.empty())
         {
            // gdb_printf( "Found @ %s", xstr.c_str());
            size_t zerox = reg_str->find( "0x");
            if( zerox != std::string::npos) 
            {
               size_t zeroxsp = reg_str->find( " ", zerox);
               reg_str->insert(  zeroxsp, "\033[40m");            // NS 2/1/2023 ...  //"\033[0m");
               reg_str->insert(  zerox, tui_hooks_filename2color( xstr)); //\033[0m");

         	   gdb_byte byte_buf[32];
               int g = target_read_memory( value_as_address( val9), byte_buf, sizeof( byte_buf));
            
               if( g == 0) 
               {
                  bool drop = false;
                  int w, stoptaking = 16;
                  for( w = 0; w < stoptaking; w++)
                  {
                      if( byte_buf[w] == 0x00) break;
                      if( byte_buf[w] >= 0x80 || byte_buf[w] < 0x20)
                      {
 		                   drop = true; 
                         break;
                      }
                  } // endfor
                  byte_buf[stoptaking] = 0x00;
                  std::string dots = "";
                  if( w == stoptaking && !drop)
                     dots = "...";



                  //std::string comm = tui_disasm_format( "x/s 0x%lx", lea);
                  //struct value *val = parse_and_eval( comm.c_str());
                  //tui_disasm_value_as_string( dest, val, 12);
                  //gdb_printf( "%s", byte_buf);
                  if( !drop) { reg_str->insert(  reg_str->size(), " \"" + std::string( (char *)byte_buf) + dots +  "\""); }
               }
            }
         }

         sprintf( xyz, "*(long *)(*(long *)$%s)", regname); 
         struct value *val0 = parse_and_eval( xyz);
         // now, value is the dereferenced value of the register
         CORE_ADDR addr0 = value_as_address( val0);

         sprintf( xyz, "*(long *)((*(long *)$%s) + 0x10)", regname); 
         struct value *val1 = parse_and_eval( xyz);
         // now, value is the dereferenced value of the register
         CORE_ADDR addr1 = value_as_address( val1);

         if( (addr0 & 0x00000000ffffffff)  < 0xff /*&& addr1 == 0x18*/)
         {
            sprintf( xyz, "*(char *)((*(long *)$%s) + 0x19)", regname); 
            val1 = parse_and_eval( xyz);
            char vaddr2 = (char)value_as_long( val1);

            // this is a QString or QByteArray!
            if( vaddr2 == 0x00 && addr1 == 0x18)
               reg_str->insert(  reg_str->size(), " \033[92m<QStr>"); //\033[0m");
             else if( addr1 == 0x18 && vaddr2 != 0x00)
               reg_str->insert(  reg_str->size(), " \033[92m<QByte>"); //\033[0m");
             else if( addr1 != 0x18)
               reg_str->insert(  reg_str->size(), " \033[92m<QList>"); //\033[0m");

            // reg value
            // in case simple QString...
            sprintf( xyz, "(*(long *)$%s)+0x18", regname); 
            reg_value->insert( 0, xyz);
         }
         else
         {
            sprintf( xyz, "$%s", regname); 
            reg_value->insert( 0, xyz);
         }
         /*
         else
            reg_str->insert(  reg_str->size(), "**");
         */
         //sprintf( xxx, " - %#lx", value_as_address( val)); 
         //str +=  std::string( xxx); // "%x %lu\n", *(unsigned int *)val, value_as_long( val));
      }
  }
  catch( ...)
  { 
     //reg_str->insert(  reg_str->size(), "--");
     //str += "\033[1;34m--\033[0m";
  } 
} // endfunc


/* Token associated with observers registered while TUI hooks are
   installed.  */
static const gdb::observers::token tui_observers_token {};

/* Attach or detach a single observer, according to ATTACH.  */

template<typename T>
static void
attach_or_detach (T &observable, typename T::func_type func, bool attach)
{
  if (attach)
    observable.attach (func, tui_observers_token, "tui-hooks");
  else
    observable.detach (tui_observers_token);
}

/* Attach or detach TUI observers, according to ATTACH.  */

static void
tui_attach_detach_observers (bool attach)
{
  attach_or_detach (gdb::observers::breakpoint_created,
		    tui_event_create_breakpoint, attach);
  attach_or_detach (gdb::observers::breakpoint_deleted,
		    tui_event_delete_breakpoint, attach);
  attach_or_detach (gdb::observers::breakpoint_modified,
		    tui_event_modify_breakpoint, attach);
  attach_or_detach (gdb::observers::inferior_exit,
		    tui_inferior_exit, attach);
  attach_or_detach (gdb::observers::before_prompt,
		    tui_before_prompt, attach);
  attach_or_detach (gdb::observers::normal_stop,
		    tui_normal_stop, attach);
  attach_or_detach (gdb::observers::register_changed,
		    tui_register_changed, attach);
  attach_or_detach (gdb::observers::user_selected_context_changed,
		    tui_context_changed, attach);
  attach_or_detach (gdb::observers::current_source_symtab_and_line_changed,
		    tui_symtab_changed, attach);

// NS 15/10
  attach_or_detach (gdb::observers::tui_next_instruction,
                    tui_process_next_instruction, attach);
  attach_or_detach (gdb::observers::solib_loaded,
                    tui_hooks_solib_loaded_observer, attach);

// NS 20/10
  attach_or_detach (gdb::observers::tui_next_reg,
                    tui_hooks_next_reg, attach);

// NS 05/02/25
  attach_or_detach (gdb::observers::inferior_created,
                    tui_inferior_attach, attach);

}

//////////////////////////////////////////////////////////////////////////////////
// NS 02/03/2025
static void tui_decompiler_finished_signal( int sig)
{
   if( sig == SIGUSR1) 
   {
      gdb_printf("[%d] Received SIGUSR1, batch action completed!", 0);
      tui_hooks_parse_sal_file();
   } // endfunc
} // endfunc




/* Install the TUI specific hooks.  */
void
tui_install_hooks (void)
{
  /* If this hook is not set to something then print_frame_info will
     assume that the CLI, not the TUI, is active, and will print the frame info
     for us in such a way that we are not prepared to handle.  This hook is
     otherwise effectively obsolete.  */
  deprecated_print_frame_info_listing_hook
    = tui_dummy_print_frame_info_listing_hook;

  /* Install the event hooks.  */
  tui_attach_detach_observers (true);

  // NS 020325 install user signal for decompiler finished
  signal( SIGUSR1, tui_decompiler_finished_signal);

}

/* Remove the TUI specific hooks.  */
void
tui_remove_hooks (void)
{
  deprecated_print_frame_info_listing_hook = 0;

  /* Remove our observers.  */
  tui_attach_detach_observers (false);
}



void _initialize_tui_hooks ();
void
_initialize_tui_hooks ()
{
  /* Install the permanent hooks.  */
  gdb::observers::new_objfile.attach (tui_new_objfile_hook, "tui-hooks");

  // NS 16/10
  struct cmd_list_element **tuicmd;

  tuicmd = tui_get_cmd_list ();
  add_cmd ( "comment", class_tui, tui_hooks_comment_all_command,
	       _("Set, clear, load or save comments"),
	       tuicmd);

  add_cmd ( "rename", class_tui, tui_hooks_call_rename_command,
	       _("Set, clear, load or save function call renames"),
	       tuicmd);

  add_cmd ( "break", class_tui, tui_hooks_break_command,
	       _("Set a break using a symbol (e.g. main) and a displacement. For example:\ntui break main+11"),
	       tuicmd);

  add_cmd ( "skip", class_tui, tui_hooks_skip_command,
	       _("Skips over the forthcoming opcode, so the next opcode will not be executed."),
	       tuicmd);

  struct cmd_list_element *c = add_cmd ( "file", class_tui, tui_hooks_file_command,
	       _("Loads an elf file (currently with debug information) to be analyzed by Ghidra. Similar to native 'file' command"),
	       tuicmd);
  set_cmd_completer (c, filename_completer);

  add_cmd ( "goto", class_tui, tui_hooks_goto_command,
	       _("Goto specified address"),
	       tuicmd);



  m_comments.clear();
  tui_hooks_deserialize_comments();

  cmd_list_element *info_breakpoints_cmd = add_info ("comments", info_comments_command, _("\
Status of comments and function renames.\n\
The \"Type\" column indicates one of:\n\
\t0   - comment\n\
\t1   - rename\n\
The \"Address\" column indicates the\n\
unbased (base zero) address of the comment or function call.\n\
\n"));


  add_info_alias ("c", info_breakpoints_cmd, 1);
}
