/* TUI display decompilation window.

   Copyright (C) 1998-2025 Free Software Foundation, Inc.

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
#include <math.h>
#include <ctype.h>
#include "symtab.h"
#include "frame.h"
#include "breakpoint.h"
#include "source.h"
#include "objfiles.h"
#include "filenames.h"
#include "source-cache.h"

#include "tui/tui.h"
#include "tui/tui-data.h"
#include "tui/tui-hooks.h"
#include "tui/tui-io.h"
#include "tui/tui-stack.h"
#include "tui/tui-win.h"
#include "tui/tui-winsource.h"
#include "tui/tui-decomp.h"
#include "tui/tui-location.h"
#include "tui/tui-disasm.h"
#include "gdb_curses.h"
#include "valprint.h"


/* Function to display source in the source window.  */
/**
 * Symtab is a structure per source file. Gdb is providing this function
 * with the appropriate symtab_and_line structure which contains the symtab
 * of what is being displayed and its line number.
 * In our case, we can't make gdb provide the correct symtab so we generate
 * it in the fly in this function from sal.rxx.
 * If there is no function in the sal.rxx that is within the cur_pc range
 * this function would simply read the <function_name>.c file and show that
 */
bool
tui_decomp_window::set_contents (struct gdbarch *arch,
				 const struct symtab_and_line &sal)
{
  //std::string *name = new std::string();
  //std::string *filename = new std::string();
  int /*offset, line, unmapped,*/ line_no = sal.line, lineCount; 
  CORE_ADDR cur_pc; // = sal.pc;
  cur_pc = tui_location.addr();
  if( cur_pc == 0)
  {
     cur_pc = sal.pc;
     if( cur_pc == 0)
        return false;
  }

  bool addBaseAddr = false, useMask = false;


  //build_address_symbolic ( arch, cur_pc, true, true, name, &offset, filename, &line, &unmapped);

  // try to find the function name from the disassembly line
  std::string function_name = tui_disasm_get_funcname_at_pc( cur_pc);
  // if have a "+something" at the end - just remove it
  size_t pos = function_name.find('+');
  if (pos != std::string::npos) {
        function_name = function_name.substr(0, pos);
  }

  std::string srclines;

  struct symtab *s = sal.symtab;
  CORE_ADDR baseaddr = 0;
  if( s != NULL && s->compunit() != NULL)
  {
     baseaddr = s->compunit()->objfile()->text_section_offset ();
     addBaseAddr = true;
     //debug: gdb_printf( "baseaddr=%lx", baseaddr);
  }
  else
  {
//     baseaddr = current_program_space->objfiles()->text_section_offset();

     for (objfile *ofile : current_program_space->objfiles ())
     {
         if( ofile->obfd == current_program_space->exec_bfd())
         {
            baseaddr = ofile->text_section_offset();
            addBaseAddr = true;
            break;
         }

/*
        if (ofile->obfd == current_program_space->exec_bfd ())
           maint_print_all_sections (_("Exec file: "), ofile->obfd.get (),
                                  ofile, arg);
        else if (opts.all_objects)
           maint_print_all_sections (_("Object file: "), ofile->obfd.get (),
                                  ofile, arg);
*/
     }



#if 0
     if( get_current_source_symtab_and_line().symtab != NULL)
     {
        baseaddr = get_current_source_symtab_and_line().symtab->compunit()->objfile()->text_section_offset ();
        addBaseAddr = true;
     }
     else 
        addBaseAddr = false;
#endif
  }
//unused?  int line_no = sal.line;
  symtab_and_line *aqs = tui_hooks_parse_sal_file();
  if( aqs == NULL)
     return false;

  symtab_and_line sal_ghidra = *aqs; 
  if( sal_ghidra.symtab == NULL) 
     return false;

  unsigned long mask = 0;
  bool foundStart = false, foundExact = false, foundWithin = false;
if( s == NULL) s = sal_ghidra.symtab;
if( s != NULL) {


  for( int p = 0; p < 2; p++)
  {
  // DEBUG:: int foundLine = 0;
      s = sal_ghidra.symtab;

  while( s != NULL)
  {     // ======================================
        // starting analysis of ghidra's symtab
        s->filename = s->fullname;
        s->set_language( language_c);

        for( int i = 0; i < s->linetable()->nitems; i++) 
        {
            CORE_ADDR ipc =  s->linetable()->item[i].pc;
            if( mask != 0) 
            {
               // ipc &= mask;
               ipc -= 0x100000;
            }
            ipc += baseaddr;

            if( ipc == cur_pc) 
            {
               foundExact = true; 
               // debug:: foundLine = i;
               break;
            }
            if( ipc <= cur_pc)
               foundStart = true;
            else if( ipc >= (unsigned long)cur_pc && foundStart == true)
            {
               foundWithin = true;
               // DEBUG:: foundLine = i;
               break;
            }
        } // endfor
        if( foundExact || foundWithin)
        {
           // found - open file and write to m_contents
           // DEBUG:: gdb_printf( "[D] found file: %s, at line=%d", s->fullname, s->linetable()->item[foundLine].line);

           // open the ghidra decompile file and read all strings to srclines
           srclines = tui_hooks_readFile( s->fullname, &lineCount);
           tui_hooks_style_source_lines( s, (char *)/*"/tmp/ghidra2/main.c"*/ s->fullname, srclines);
           break;
        }
     s = s->next;
  } // endwhile all symtabs
  if( !foundExact && !foundWithin)
  {
     mask = 0xffff;
     useMask = true;
     continue;
  }
  else
  {
     break;
  }
} // endfor
} // endif

////>>> you can't replace the symtab you just found, stupid!  s = sal.symtab;

        // FIX FIX FIX FIX FIX FIX FIX
	if( !foundExact && !foundWithin)
	{
           // DEBUG:: gdb_printf( "[D] func: %s", function_name.c_str());
           std::string f__name = /*"/tmp/ghidra2/" +*/  function_name + ".c";
           srclines = tui_hooks_readFile( f__name.c_str(), &lineCount);
           char *full = (char *)f__name.c_str();
           tui_hooks_style_source_lines( sal_ghidra.symtab, full, srclines);
           sal_ghidra.pspace = sal.pspace;
           sal_ghidra.symtab->fullname = full;
           sal_ghidra.symtab->filename = full;
	   sal_ghidra.symtab->set_language( language_c); 
           s = sal_ghidra.symtab;
	}
 


  if( s == NULL)
  {
     // try to open the sal.rx file for info
     // DEBUG:: gdb_printf( "[D] and out...%d", 1);
     return false;
  }

  /* Take hilite (window border) into account, when
     calculating the number of lines.  */
  int nlines = height - box_size ();

  int cur_line_no, cur_line;

#if 0
  const std::vector<off_t> *offsets;
  if (!g_source_cache.get_source_lines (s, line_no, line_no + nlines,
					&srclines)
      || !g_source_cache.get_line_charpos (s, &offsets))
    return false;
#endif

  const char *s_filename = symtab_to_filename_for_display (s);

  set_title (s_filename);

  m_fullname = make_unique_xstrdup (symtab_to_fullname (s));

  cur_line = 0;
  if( s->compunit() == NULL)
     m_gdbarch = arch;
  else
     m_gdbarch = s->compunit ()->objfile ()->arch ();

  m_start_line_or_addr.loa = LOA_LINE;
  cur_line_no = m_start_line_or_addr.u.line_no = line_no;

  m_digits = 3;
#if 1
  if (compact_source)
    {
      /* Solaris 11+gcc 5.5 has ambiguous overloads of log10, so we
	 cast to double to get the right one.  */
//      int lines_in_file = offsets->size ();
      int lines_in_file = lineCount; //offsets->size ();
      int max_line_nr = lines_in_file;
      int digits_needed = 1 + (int)log10 ((double) max_line_nr);
      int trailing_space = 1;
      m_digits = digits_needed + trailing_space;
    }
#endif

  m_max_length = -1;
  const char *iter = srclines.c_str ();

  m_content.resize (nlines);

  bool firstTime = true;

  // NS 080325
  cur_line_no = 1;

  while (cur_line < nlines)
    {
      struct tui_source_element *element = &m_content[cur_line];

      std::string text;
      if (*iter != '\0')
	{
	  int line_len;
	  text = tui_copy_source_line (&iter, &line_len);
	  m_max_length = std::max (m_max_length, line_len);
	}
      else
	{
	  /* Line not in source file.  */
	  // NS cur_line_no = -1;
	}

      /* Set whether element is the execution point
	 and whether there is a break point on it.  */
      element->line_or_addr.loa = LOA_LINE;
      element->line_or_addr.u.line_no = cur_line_no;


      bool foundLine = false;
      // check if current source line appears in the parsed symtab linetables 
      for( int i = 0; i < s->linetable()->nitems; i++) 
      {
          // CORE_ADDR ipc =  s->linetable()->item[i].pc;
          CORE_ADDR pce = s->linetable()->item[i].pc;

          if( useMask)
             pce -= 0x100000;
             //pce &= 0xffff;
          if( addBaseAddr)
             pce += baseaddr;

          if( pce == cur_pc && s->linetable()->item[i].line == cur_line_no)
          {
             foundLine = true;
             break;
          }
      } // endfor all lines

      if( foundLine && firstTime)
      {  // clear all exec points if have a new one
         for( int pp = 0; pp < m_content.size(); pp++)
         {
             struct tui_source_element *_element = &m_content[pp];
             _element->is_exec_point = false;
         }
         firstTime = false;
      }
      if( foundLine)
         element->is_exec_point = true; //(foundLine) ? true : false;
      
/*
	= (filename_cmp (tui_location.full_name ().c_str (),
			 symtab_to_fullname (s)) == 0
	   && cur_line_no == tui_location.line_no ());
*/
      m_content[cur_line].line = std::move (text);

      cur_line++;
      cur_line_no++;
    }

  return true;
}


/**
 *
 *
 *
**/
#if 0
int tui_source_build_sal( const struct symtab_and_line &sal)
{
   struct symtab_and_line sal = get_current_source_symtab_and_line ();
   if( true)
   {
     FILE *fil = fopen( "/tmp/ghidra/sal.rx", "r");
     if( fil != NULL)
     {
        char l[1024], src_file_name[1024];
        while( fgets( l, sizeof( l), fil)) 
        {
            if( sscanf( l, " 0x%lx 0x%lx:%s:%lu", &from_addr, &to_addr, src_file_name, line_no) >= 4)
            {
               linetable *entry = new linetable();
               entry->nitems = 1;
               entry->item[0].line = line_no;
               entry->item[0].pc   = (CORE_ADDR)from_addr;
               symtab *s = new symtab();
               s->linetable = entry;
               s->filename = std::string( src_file_name).c_str();
            segments_lookup lookup;
            lookup.from_addr = from_addr;
            lookup.to_addr   = to_addr;
            lookup.func_name = func_name;
            retVec.push_back( lookup);
            //found = true;
        } // endif

        } // endwhile
        fclose( fil);
     } // endif have file
  
   } // endif
} // endfunc
#endif



/* Answer whether the source is currently displayed in the source
   window.  */
bool
tui_decomp_window::showing_source_p (const char *fullname) const
{
  return (!m_content.empty ()
	  && (filename_cmp (tui_location.full_name ().c_str (),
			    fullname) == 0));
}


/* Scroll the source forward or backward vertically.  */
void
tui_decomp_window::do_scroll_vertical (int num_to_scroll)
{
  if (!m_content.empty ())
    {
      struct symtab *s;
      struct symtab_and_line cursal = get_current_source_symtab_and_line ();
      struct gdbarch *arch = m_gdbarch;

      if (cursal.symtab == NULL)
	{
	  frame_info_ptr fi = get_selected_frame (NULL);
	  s = find_pc_line_symtab (get_frame_pc (fi));
	  arch = get_frame_arch (fi);
	}
      else
	s = cursal.symtab;

      int line_no = m_start_line_or_addr.u.line_no + num_to_scroll;
      const std::vector<off_t> *offsets;
      if (g_source_cache.get_line_charpos (s, &offsets)
	  && line_no > offsets->size ())
	line_no = m_start_line_or_addr.u.line_no;
      if (line_no <= 0)
	line_no = 1;

      cursal.line = line_no;
      find_line_pc (cursal.symtab, cursal.line, &cursal.pc);
      for (struct tui_source_window_base *win_info : tui_source_windows ())
	win_info->update_source_window_as_is (arch, cursal);
    }
}

bool
tui_decomp_window::location_matches_p (struct bp_location *loc, int line_no)
{
  return (m_content[line_no].line_or_addr.loa == LOA_LINE
	  && m_content[line_no].line_or_addr.u.line_no == loc->line_number
	  && loc->symtab != NULL
	  && filename_cmp (m_fullname.get (),
			   symtab_to_fullname (loc->symtab)) == 0);
}

/* See tui-decomp.h.  */

bool
tui_decomp_window::line_is_displayed (int line) const
{
  if (m_content.size () < SCROLL_THRESHOLD)
    return false;

  for (size_t i = 0; i < m_content.size () - SCROLL_THRESHOLD; ++i)
    {
      if (m_content[i].line_or_addr.loa == LOA_LINE
	  && m_content[i].line_or_addr.u.line_no == line)
	return true;
    }

  return false;
}

void
tui_decomp_window::maybe_update (frame_info_ptr fi, symtab_and_line sal)
{
  int start_line = (sal.line - ((height - box_size ()) / 2)) + 1;
  if (start_line <= 0)
    start_line = 1;

  bool source_already_displayed = (sal.symtab != 0
				   && showing_source_p (m_fullname.get ()));

  if (!(source_already_displayed && line_is_displayed (sal.line)))
    {
      sal.line = start_line;
      update_source_window (get_frame_arch (fi), sal);
    }
  else
    {
      struct tui_line_or_address l;

      l.loa = LOA_LINE;
      l.u.line_no = sal.line;
      set_is_exec_point_at (l);
    }
}

void
tui_decomp_window::display_start_addr (struct gdbarch **gdbarch_p,
				       CORE_ADDR *addr_p)
{
  struct symtab_and_line cursal = get_current_source_symtab_and_line ();

  *gdbarch_p = m_gdbarch;
  find_line_pc (cursal.symtab, m_start_line_or_addr.u.line_no, addr_p);
}

/* See tui-winsource.h.  */

void
tui_decomp_window::show_line_number (int offset) const
{
  int lineno = m_content[offset].line_or_addr.u.line_no;
  char text[20];
  char space = tui_left_margin_verbose ? '_' : ' ';
  if (lineno == -1)
    {
      /* Line not in source file, don't show line number.  */
      for (int i = 0; i <= m_digits; ++i)
	text[i] = (i == m_digits) ? '\0' : space;
    }
  else
    {
      xsnprintf (text, sizeof (text),
		 tui_left_margin_verbose ? "%0*d%c" : "%*d%c", m_digits - 1,
		 lineno, space);
    }
  waddstr (handle.get (), text);
}
