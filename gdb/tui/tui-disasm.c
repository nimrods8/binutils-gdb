/*Disassembly display.

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
#include "symtab.h"
#include "breakpoint.h"
#include "frame.h"
#include "value.h"
#include "source.h"
#include "disasm.h"
#include "tui/tui.h"
#include "tui/tui-command.h"
#include "tui/tui-data.h"
#include "tui/tui-win.h"
#include "tui/tui-layout.h"
#include "tui/tui-winsource.h"
#include "tui/tui-stack.h"
#include "tui/tui-file.h"
#include "tui/tui-disasm.h"
#include "tui/tui-source.h"
#include "progspace.h"
#include "objfiles.h"
#include "cli/cli-style.h"
#include "tui/tui-location.h"
#include "gdbcmd.h"

#include "gdb_curses.h"

#include <fstream>

static CORE_ADDR showAddr;
static bool isDecompiler = false;
static int decompiler_line;


// NS 04/11 should be list per arch
static std::vector<std::string> calls = {"call", "bl", "blr", "jmp"};
static std::vector<std::string> calls_funcnames = {"call", "bl", "blr"};
static std::vector<std::string> loads = {"lea", "mov"};

/********************************************************/
/*      P R I V A T E      D A T A S T R U C T S        */
/********************************************************/
struct tui_asm_line
{
  CORE_ADDR addr;
  std::string addr_string;
  size_t addr_size;
  std::string insn;
};

/********************************************************/
/*      P R I V A T E     F U N C T I O N S             */
/********************************************************/
bool tui_disasm_str_replace(std::string &str, const std::string &from, const std::string &to);
CORE_ADDR tui_disasm_parse_line(std::string asm_line);
std::string tui_diasm_remove_ansi_colors(std::string _line);
CORE_ADDR tui_disasm_find_maybe_end_of_func(CORE_ADDR from);
std::vector<std::string> tui_disasm_find_funcnames(CORE_ADDR, CORE_ADDR);
std::string tui_disasm_parse_for_funcnames(std::string asm_line);
CORE_ADDR tui_disasm_is_abs(std::string line, int fopcode);
CORE_ADDR tui_disasm_check_load_add(std::vector<tui_asm_line> asmlines);
// static void tui_disasm_value_as_string (char *dest, struct value *val, int length);
inline std::string tui_disasm_format(const char *fmt, ...);

/* Helper function to find the number of characters in STR, skipping
   any ANSI escape sequences.  */
static size_t
len_without_escapes(const std::string &str)
{
  size_t len = 0;
  const char *ptr = str.c_str();
  char c;

  while ((c = *ptr) != '\0')
  {
    if (c == '\033')
    {
      ui_file_style style;
      size_t n_read;
      if (style.parse(ptr, &n_read))
        ptr += n_read;
      else
      {
        /* Shouldn't happen, but just skip the ESC if it somehow
     does.  */
        ++ptr;
      }
    }
    else
    {
      ++len;
      ++ptr;
    }
  }
  return len;
}

/* Function to disassemble up to COUNT instructions starting from address
   PC into the ASM_LINES vector (which will be emptied of any previous
   contents).  Return the address of the COUNT'th instruction after pc.
   When ADDR_SIZE is non-null then place the maximum size of an address and
   label into the value pointed to by ADDR_SIZE, and set the addr_size
   field on each item in ASM_LINES, otherwise the addr_size fields within
   ASM_LINES are undefined.

   It is worth noting that ASM_LINES might not have COUNT entries when this
   function returns.  If the disassembly is truncated for some other
   reason, for example, we hit invalid memory, then ASM_LINES can have
   fewer entries than requested.  */
static CORE_ADDR
tui_disassemble(struct gdbarch *gdbarch,
                std::vector<tui_asm_line> &asm_lines,
                CORE_ADDR pc, int count,
                size_t *addr_size = nullptr)
{
  bool term_out = source_styling && gdb_stdout->can_emit_style_escape();
  string_file gdb_dis_out(term_out);

  /* Must start with an empty list.  */
  asm_lines.clear();

  // NS call notify to signal start...
  gdb::observers::tui_next_instruction.notify(0, NULL, NULL);

  /* Now construct each line.  */
  for (int i = 0; i < count; ++i)
  {
    tui_asm_line tal;
    CORE_ADDR orig_pc = pc;

    try
    {
      pc = pc + gdb_print_insn(gdbarch, pc, &gdb_dis_out, NULL);
    }
    catch (const gdb_exception_error &except)
    {
      /* If PC points to an invalid address then we'll catch a
         MEMORY_ERROR here, this should stop the disassembly, but
         otherwise is fine.  */
      if (except.error != MEMORY_ERROR)
        throw;
      return pc;
    }

    /* Capture the disassembled instruction.  */
    tal.insn = gdb_dis_out.release();

    // NS
    //      std::printf( "Instruction: %s", tal.insn.c_str());
    // tal.insn += "\t\033[0;93m// this is a comment!!\033[0m";

    /* And capture the address the instruction is at.  */
    tal.addr = orig_pc;
    print_address(gdbarch, orig_pc, &gdb_dis_out);
    tal.addr_string = gdb_dis_out.release();

    // NS 15/10
    {
      std::string str_comment = "";

      gdb::observers::tui_next_instruction.notify(orig_pc, &str_comment, &tal.insn);
      if (str_comment.size() > 0)
      {
        tal.insn += "\t\033[0;93m// ";
        tal.insn += str_comment;
        tal.insn += "\033[0m";
      } // endif have new comment

      std::vector<tui_asm_line> asml;
      asml.push_back(tal);
      CORE_ADDR lea = tui_disasm_check_load_add(asml);

      //    gdb_printf( "target =0x%lx", lea);

      if (lea != 0L)
      {
        // char dest[13];

        gdb_byte byte_buf[300];
        int g = target_read_memory(lea, byte_buf, sizeof(byte_buf));
        // gdb_printf( "target =%d", g);
        if (g == 0)
        {
          bool drop = false;
          int w, takeupto = 12;
          for (w = 0; w < takeupto; w++)
          {
            if (byte_buf[w] == 0x00)
              break;
            if (byte_buf[w] >= 0x80 || byte_buf[w] < 0x20)
            {
              drop = true;
              break;
            }
          }
          byte_buf[takeupto] = 0x00;
          std::string dots = "";
          if (w == takeupto && !drop)
          {
            dots = "...";
          }

          // std::string comm = tui_disasm_format( "x/s 0x%lx", lea);
          // struct value *val = parse_and_eval( comm.c_str());
          // tui_disasm_value_as_string( dest, val, 12);
          // gdb_printf( "%s", byte_buf);
          if (!drop)
          {
            tal.insn += " \"" + std::string((char *)byte_buf) + dots + "\"";
          }
        } // endif
      }   // endif
    }

    if (addr_size != nullptr)
    {
      size_t new_size;

      if (term_out)
        new_size = len_without_escapes(tal.addr_string);
      else
        new_size = tal.addr_string.size();
      *addr_size = std::max(*addr_size, new_size);
      tal.addr_size = new_size;
    }

    asm_lines.push_back(std::move(tal));
  }

  // NS 17/10
  // notify observer that all lines have been formatted
  //  gdb::observers::tui_next_instruction.notify( 0, NULL, 0, NULL);

  return pc;
}

/* Look backward from ADDR for an address from which we can start
   disassembling, this needs to be something we can be reasonably
   confident will fall on an instruction boundary.  We use msymbol
   addresses, or the start of a section.  */

static CORE_ADDR
tui_find_backward_disassembly_start_address(CORE_ADDR addr)
{
  struct bound_minimal_symbol msym, msym_prev;

  msym = lookup_minimal_symbol_by_pc_section(addr - 1, nullptr,
                                             lookup_msym_prefer::TEXT,
                                             &msym_prev);
  if (msym.minsym != nullptr)
    return msym.value_address();
  else if (msym_prev.minsym != nullptr)
    return msym_prev.value_address();

  /* Find the section that ADDR is in, and look for the start of the
     section.  */
  struct obj_section *section = find_pc_section(addr);
  if (section != NULL)
    return section->addr();

  return addr;
}

/* Find the disassembly address that corresponds to FROM lines above
   or below the PC.  Variable sized instructions are taken into
   account by the algorithm.  */
static CORE_ADDR
tui_find_disassembly_address(struct gdbarch *gdbarch, CORE_ADDR pc, int from)
{
  CORE_ADDR new_low;
  int max_lines;

  max_lines = (from > 0) ? from : -from;
  if (max_lines == 0)
    return pc;

  std::vector<tui_asm_line> asm_lines;

  new_low = pc;
  if (from > 0)
  {
    /* Always disassemble 1 extra instruction here, then if the last
 instruction fails to disassemble we will take the address of the
 previous instruction that did disassemble as the result.  */
    tui_disassemble(gdbarch, asm_lines, pc, max_lines + 1);
    new_low = asm_lines.back().addr;
  }
  else
  {
    /* In order to disassemble backwards we need to find a suitable
 address to start disassembling from and then work forward until we
 re-find the address we're currently at.  We can then figure out
 which address will be at the top of the TUI window after our
 backward scroll.  During our backward disassemble we need to be
 able to distinguish between the case where the last address we
 _can_ disassemble is ADDR, and the case where the disassembly
 just happens to stop at ADDR, for this reason we increase
 MAX_LINES by one.  */
    max_lines++;

    /* When we disassemble a series of instructions this will hold the
 address of the last instruction disassembled.  */
    CORE_ADDR last_addr;

    /* And this will hold the address of the next instruction that would
 have been disassembled.  */
    CORE_ADDR next_addr;

    /* As we search backward if we find an address that looks like a
 promising starting point then we record it in this structure.  If
 the next address we try is not a suitable starting point then we
 will fall back to the address held here.  */
    gdb::optional<CORE_ADDR> possible_new_low;

    /* The previous value of NEW_LOW so we know if the new value is
 different or not.  */
    CORE_ADDR prev_low;

    do
    {
      /* Find an address from which we can start disassembling.  */
      prev_low = new_low;
      new_low = tui_find_backward_disassembly_start_address(new_low);

      /* Disassemble forward.  */
      next_addr = tui_disassemble(gdbarch, asm_lines, new_low, max_lines);
      last_addr = asm_lines.back().addr;

      /* If disassembling from the current value of NEW_LOW reached PC
         (or went past it) then this would do as a starting point if we
         can't find anything better, so remember it.  */
      if (last_addr >= pc && new_low != prev_low && asm_lines.size() >= max_lines)
        possible_new_low.emplace(new_low);

      /* Continue searching until we find a value of NEW_LOW from which
         disassembling MAX_LINES instructions doesn't reach PC.  We
         know this means we can find the required number of previous
         instructions then.  */
    } while ((last_addr > pc || (last_addr == pc && asm_lines.size() < max_lines)) && new_low != prev_low);

    /* If we failed to disassemble the required number of lines then the
 following walk forward is not going to work, it assumes that
 ASM_LINES contains exactly MAX_LINES entries.  Instead we should
 consider falling back to a previous possible start address in
 POSSIBLE_NEW_LOW.  */
    if (asm_lines.size() < max_lines)
    {
      if (!possible_new_low.has_value())
        return new_low;

      /* Take the best possible match we have.  */
      new_low = *possible_new_low;
      next_addr = tui_disassemble(gdbarch, asm_lines, new_low, max_lines);
      last_addr = asm_lines.back().addr;
      gdb_assert(asm_lines.size() >= max_lines);
    }

    /* Scan forward disassembling one instruction at a time until
 the last visible instruction of the window matches the pc.
 We keep the disassembled instructions in the 'lines' window
 and shift it downward (increasing its addresses).  */
    int pos = max_lines - 1;
    if (last_addr < pc)
      do
      {
        pos++;
        if (pos >= max_lines)
          pos = 0;

        CORE_ADDR old_next_addr = next_addr;
        std::vector<tui_asm_line> single_asm_line;
        next_addr = tui_disassemble(gdbarch, single_asm_line,
                                    next_addr, 1);
        /* If there are some problems while disassembling exit.  */
        if (next_addr <= old_next_addr)
          return pc;
        gdb_assert(single_asm_line.size() == 1);
        asm_lines[pos] = single_asm_line[0];
      } while (next_addr <= pc);
    pos++;
    if (pos >= max_lines)
      pos = 0;
    new_low = asm_lines[pos].addr;

    /* When scrolling backward the addresses should move backward, or at
 the very least stay the same if we are at the first address that
 can be disassembled.  */
    gdb_assert(new_low <= pc);
  }
  return new_low;
}

/* Function to set the disassembly window's content.  */
bool tui_disasm_window::set_contents(struct gdbarch *arch,
                                     const struct symtab_and_line &sal)
{
  int i;
  int max_lines;
  CORE_ADDR cur_pc;
  int tab_len = tui_tab_width;
  int insn_pos;

  // NS 02/11
  // bool found = false;

  CORE_ADDR pc = sal.pc;
  if (pc == 0)
    return false;

  m_gdbarch = arch;
  m_start_line_or_addr.loa = LOA_ADDRESS;
  m_start_line_or_addr.u.addr = pc;
  cur_pc = tui_location.addr();

  /* Window size, excluding highlight box.  */
  max_lines = height - 2;

  /* Get temporary table that will hold all strings (addr & insn).  */
  std::vector<tui_asm_line> asm_lines;
  size_t addr_size = 0;
  tui_disassemble(m_gdbarch, asm_lines, pc, max_lines, &addr_size);

  /* Align instructions to the same column.  */
  insn_pos = (1 + (addr_size / tab_len)) * tab_len;

  /* Now construct each line.  */
  m_content.resize(max_lines);
  m_max_length = -1;
  for (i = 0; i < max_lines; i++)
  {
    tui_source_element *src = &m_content[i];

    std::string line;
    CORE_ADDR addr;

    if (i < asm_lines.size())
    {
      line = (asm_lines[i].addr_string + n_spaces(insn_pos - asm_lines[i].addr_size) + asm_lines[i].insn);
      addr = asm_lines[i].addr;
    }
    else
    {
      line = "";
      addr = 0;
    }

    const char *ptr = line.c_str();
    int line_len;
    src->line = tui_copy_source_line(&ptr, &line_len);
    m_max_length = std::max(m_max_length, line_len);

    src->line_or_addr.loa = LOA_ADDRESS;
    src->line_or_addr.u.addr = addr;
    src->is_exec_point = (addr == cur_pc && line.size() > 0);
#if 0
      // NS 01/11
      if( asm_lines[i].insn.find( std::string( "xor")) != std::string::npos) 
      {
        found = true;

        //gdb_printf( "have xor\n\n");
        if( TUI_DISASMOT_WIN != nullptr) 
        {
           TUI_DISASMOT_WIN->isVisible = true;

           //gdb_printf( "have resize\n\n");
           //TUI_DISASMOT_WIN->resize( 20, 40, 20, 40);
         //switch_to( "layout ontop");
        }
      }
#endif
  } // endfor all lines

#if 0
    // NS 02/11
    if( !found)
    {
      if( TUI_DISASMOT_WIN != nullptr) {
        TUI_DISASMOT_WIN->isVisible = false;
      }
    }
#endif
  return true;
}

void tui_get_begin_asm_address(struct gdbarch **gdbarch_p, CORE_ADDR *addr_p)
{
  struct gdbarch *gdbarch = get_current_arch();
  CORE_ADDR addr = 0;

  if (tui_location.addr() == 0)
  {
    if (have_full_symbols() || have_partial_symbols())
    {
      set_default_source_symtab_and_line();
      struct symtab_and_line sal = get_current_source_symtab_and_line();

      if (sal.symtab != nullptr)
        find_line_pc(sal.symtab, sal.line, &addr);
    }

    if (addr == 0)
    {
      struct bound_minimal_symbol main_symbol = lookup_minimal_symbol(main_name(), nullptr, nullptr);
      if (main_symbol.minsym != nullptr)
        addr = main_symbol.value_address();
    }
  }
  else /* The target is executing.  */
  {
    gdbarch = tui_location.gdbarch();
    addr = tui_location.addr();
  }

  *gdbarch_p = gdbarch;
  *addr_p = addr;
}

/* Determine what the low address will be to display in the TUI's
   disassembly window.  This may or may not be the same as the low
   address input.  */
CORE_ADDR
tui_get_low_disassembly_address(struct gdbarch *gdbarch,
                                CORE_ADDR low, CORE_ADDR pc)
{
  int pos;

  /* Determine where to start the disassembly so that the pc is about
     in the middle of the viewport.  */
  if (TUI_DISASM_WIN != NULL)
    pos = TUI_DISASM_WIN->height;
  else if (TUI_CMD_WIN == NULL)
    pos = tui_term_height() / 2 - 2;
  else
    pos = tui_term_height() - TUI_CMD_WIN->height - 2;
  pos = (pos - 2) / 2;

  pc = tui_find_disassembly_address(gdbarch, pc, -pos);

  if (pc < low)
    pc = low;
  return pc;
}

/* Scroll the disassembly forward or backward vertically.  */
void tui_disasm_window::do_scroll_vertical(int num_to_scroll)
{
  if (!m_content.empty())
  {
    CORE_ADDR pc;

    pc = m_start_line_or_addr.u.addr;

    symtab_and_line sal{};
    sal.pspace = current_program_space;
    sal.pc = tui_find_disassembly_address(m_gdbarch, pc, num_to_scroll);
    update_source_window_as_is(m_gdbarch, sal);
  }
}

bool tui_disasm_window::location_matches_p(struct bp_location *loc, int line_no)
{
  return (m_content[line_no].line_or_addr.loa == LOA_ADDRESS && m_content[line_no].line_or_addr.u.addr == loc->address);
}

bool tui_disasm_window::addr_is_displayed(CORE_ADDR addr) const
{
  if (m_content.size() < SCROLL_THRESHOLD)
    return false;

  for (size_t i = 0; i < m_content.size() - SCROLL_THRESHOLD; ++i)
  {
    if (m_content[i].line_or_addr.loa == LOA_ADDRESS && m_content[i].line_or_addr.u.addr == addr)
      return true;
  }

  return false;
}

void tui_disasm_window::maybe_update(frame_info_ptr fi, symtab_and_line sal)
{
  CORE_ADDR low;

  struct gdbarch *frame_arch = get_frame_arch(fi);

  if (find_pc_partial_function(sal.pc, NULL, &low, NULL) == 0)
  {
    /* There is no symbol available for current PC.  There is no
 safe way how to "disassemble backwards".  */
    low = sal.pc;
  }
  else
    low = tui_get_low_disassembly_address(frame_arch, low, sal.pc);

  struct tui_line_or_address a;

  a.loa = LOA_ADDRESS;
  a.u.addr = low;
  if (!addr_is_displayed(sal.pc))
  {
    sal.pc = low;
    update_source_window(frame_arch, sal);
  }
  else
  {
    a.u.addr = sal.pc;
    set_is_exec_point_at(a);
  }
}

void tui_disasm_window::display_start_addr(struct gdbarch **gdbarch_p,
                                           CORE_ADDR *addr_p)
{
  *gdbarch_p = m_gdbarch;
  *addr_p = m_start_line_or_addr.u.addr;
}

//===========================
// NS 30/10
//===========================

/* Scroll the disassembly forward or backward vertically.  */
void tui_disasm_ontop_window::do_scroll_vertical(int num_to_scroll)
{
  // gdb_printf( "now scrolling!");
  if (!m_content.empty() && !isDecompiler)
  {
    CORE_ADDR pc;

    pc = m_start_line_or_addr.u.addr;

    symtab_and_line sal{};
    sal.pspace = current_program_space;
    sal.pc = tui_find_disassembly_address(m_gdbarch, pc, num_to_scroll);
    update_source_window_as_is(m_gdbarch, sal);
  }
  else if( !m_content.empty() && isDecompiler)
  {
     decompiler_line += num_to_scroll;
     if( decompiler_line >= m_content.size())
        decompiler_line = m_content.size() - 1;
     if( decompiler_line < 0) decompiler_line = 0;
  } // endif isdecompiler
}

bool tui_disasm_ontop_window::location_matches_p(struct bp_location *loc, int line_no)
{
  return (m_content[line_no].line_or_addr.loa == LOA_ADDRESS && m_content[line_no].line_or_addr.u.addr == loc->address);
}

bool tui_disasm_ontop_window::addr_is_displayed(CORE_ADDR addr) const
{
  if (m_content.size() < SCROLL_THRESHOLD)
    return false;

  for (size_t i = 0; i < m_content.size() - SCROLL_THRESHOLD; ++i)
  {
    if (m_content[i].line_or_addr.loa == LOA_ADDRESS && m_content[i].line_or_addr.u.addr == addr)
      return true;
  }

  return false;
}

void tui_disasm_ontop_window::maybe_update(frame_info_ptr fi, symtab_and_line sal)
{
  CORE_ADDR low;

  struct gdbarch *frame_arch = get_frame_arch(fi);

  if (find_pc_partial_function(sal.pc, NULL, &low, NULL) == 0)
  {
    /* There is no symbol available for current PC.  There is no
 safe way how to "disassemble backwards".  */
    low = sal.pc;
  }
  else
    low = tui_get_low_disassembly_address(frame_arch, low, sal.pc);

  struct tui_line_or_address a;

  a.loa = LOA_ADDRESS;
  a.u.addr = low;
  if (!addr_is_displayed(sal.pc))
  {
    sal.pc = low;
    update_source_window(frame_arch, sal);
  }
  else
  {
    a.u.addr = sal.pc;
    set_is_exec_point_at(a);
  }
}

void tui_disasm_ontop_window::display_start_addr(struct gdbarch **gdbarch_p,
                                                 CORE_ADDR *addr_p)
{
  *gdbarch_p = m_gdbarch;
  *addr_p = m_start_line_or_addr.u.addr;
}

/* Function to set the disassembly window's content.  */
bool tui_disasm_ontop_window::set_contents(struct gdbarch *arch,
                                           const struct symtab_and_line &sal)
{
  int i;
  int max_lines;
  CORE_ADDR cur_pc;
  int tab_len = tui_tab_width;
  int insn_pos;

  CORE_ADDR pc = sal.pc;
  if (pc == 0 || !isVisible)
    return false;

  //////////////////
  // return false;
  //////////////////

  pc = showAddr;

  // gdb_printf( "now inside set contents\n");

  m_gdbarch = arch;
  m_start_line_or_addr.loa = LOA_ADDRESS;
  m_start_line_or_addr.u.addr = pc;
  cur_pc = tui_location.addr();

  // NS 01/11
  // gdb_printf( "Prior height=%d, width=%d\n", height, width);

  /*
    height = 20;
    width = 60;
    x = 60;
    y = 20;
  */

  /* Window size, excluding highlight box.  */
  max_lines = height - 2;

  /* Get temporary table that will hold all strings (addr & insn).  */
  std::vector<tui_asm_line> asm_lines;
  size_t addr_size = 0;
  tui_disassemble(m_gdbarch, asm_lines, pc, max_lines, &addr_size);

  /* Align instructions to the same column.  */
  insn_pos = (1 + (addr_size / tab_len)) * tab_len;

  /* Now construct each line.  */
  m_content.resize(max_lines);
  m_max_length = -1;

// NS 11/11
#if 1
  std::vector<std::string> decomp;
  if (isDecompiler)
  {
    // NS 11/11 read from /tmp/decompiler.out
    std::ifstream inf;
    inf.open("/tmp/decompiler.txt");
    if (inf.is_open())
    {
      std::string l1;
      while (getline(inf, l1))
      {
        decomp.push_back(l1);
      }
      inf.close();
    } // endif open OK
  }   // endif decompiler

#endif

  for (i = 0; i < max_lines; i++)
  {
    tui_source_element *src = &m_content[i];

    std::string line;
    CORE_ADDR addr;

    if (i < asm_lines.size())
    {
      // NS 11/11
      std::string addstr;
      if (isDecompiler)
      {
        addstr = (i < decomp.size() ? decomp.at(i + decompiler_line) : "");
        line = n_spaces(insn_pos - asm_lines[i].addr_size) + addstr;
        addr = asm_lines[i].addr;
      }
      else
      {
        line = (asm_lines[i].addr_string + n_spaces(insn_pos - asm_lines[i].addr_size) + asm_lines[i].insn /*+ addstr*/);
        addr = asm_lines[i].addr;
      }
    }
    else
    {
      line = "";
      addr = 0;
    }

    const char *ptr = line.c_str();
    int line_len;
    src->line = tui_copy_source_line(&ptr, &line_len);
    m_max_length = std::max(m_max_length, line_len);

    src->line_or_addr.loa = LOA_ADDRESS;
    src->line_or_addr.u.addr = addr;
    src->is_exec_point = (addr == cur_pc && line.size() > 0);
  }
  return true;
}

// NS 03/11

/* Called for each mouse click inside this window.  Coordinates MOUSE_X
   and MOUSE_Y are 0-based relative to the window, and MOUSE_BUTTON can
   be 1 (left), 2 (middle), or 3 (right).  */
void tui_disasm_window::click(int mouse_x, int mouse_y, int mouse_button)
{
  bool found = false;

  isDecompiler = false;

  if (!m_content.empty() && m_content.size() >= mouse_y)
  {
    if (mouse_button == 3)
    {
      if (TUI_DISASMOT_WIN != nullptr && !TUI_DISASMOT_WIN->isVisible)
      {
        // gdb_printf( "have xor\n\n");
        //struct gdbarch *gdbarch = get_current_arch();

        decompiler_line = 0;

        int _x = TUI_DISASMOT_WIN->x;
        int _y = TUI_DISASMOT_WIN->y;
        int _w = TUI_DISASMOT_WIN->width;
        int _h = TUI_DISASMOT_WIN->height;
        _y = mouse_y + TUI_DISASM_WIN->y + 2;
        _x = mouse_x + TUI_DISASM_WIN->x + 1;
        _w = 100;
        _h = 20;

        TUI_DISASMOT_WIN->resize(_h, _w, _x, _y);
        TUI_DISASMOT_WIN->make_visible(true);
        TUI_DISASMOT_WIN->isVisible = true;
        // TUI_DISASMOT_WIN->refill();
        //tui_apply_current_layout(true);
        //tui_update_ontop_windows_with_addr(gdbarch, 0L);
        TUI_DISASMOT_WIN->erase_data_content( "Wait for Decompiler...");
        TUI_DISASMOT_WIN->isVisible = false;
      }
    }

    // gdb_printf( "Line=%s", m_content[mouse_y].line.c_str());

    std::string line2 = m_content[mouse_y].line;
    std::string decompName = "func";
    CORE_ADDR addr;

    addr = tui_disasm_parse_line(line2); // return jump to address

    // NS 11/11 try to get the address AT the line instead
    if (addr == 0L)
    {
      std::string line3 = tui_diasm_remove_ansi_colors(line2);
      std::size_t frip = line3.find("0x");
      std::size_t fripend = line3.find(" ", frip);
      addr = std::stoul(line3.substr(frip, fripend - frip), nullptr, 16);
      // gdb_printf( "0x%lx", addr);

      // next try to find the decompiled function name
      std::size_t fnme = line3.find("<", frip);
      std::size_t fnmed = line3.find(">", frip);
      std::size_t fnmed2 = line3.find("+", frip);
      if( fnmed2 < fnmed) fnmed = fnmed2;
      if( fnme != std::string::npos)
         decompName = line3.substr( fnme + 1, fnmed - fnme - 1);
    }

    

    if (addr != 0L && !TUI_DISASMOT_WIN->isVisible)
    {
      // read this memory to array

      // find -maybe- end of func address by scanning
      CORE_ADDR end_addr = tui_disasm_find_maybe_end_of_func(addr) + 1; // to add the ret
      //gdb_printf("decompile @%lx", end_addr);

      long take = 300; // = sizeof( byte_buf);
      if (end_addr - 1L > 0L)
      {
        take = end_addr - addr + 1;
        //gdb_printf("take=%lx::%lx\n", take, end_addr);
      }
      gdb_byte *byte_buf = (gdb_byte *)malloc(take);

      if (byte_buf != 0)
      {
        /*int g =*/target_read_code(addr, byte_buf, take);

        // gdb_printf( "ret=%d", g);
        FILE *fil;
        fil = fopen("/tmp/data000", "w");
        fprintf(fil, "\"0x%lx\"\n", addr);

        for (int iz = 0; iz < take /*sizeof( byte_buf)*/; iz++)
        {
          fprintf(fil, "%02x", byte_buf[iz]);
        }
        fprintf(fil, "\n");
        fprintf( fil, "%s\n", decompName.c_str());

        std::vector<std::string> comVec = tui_disasm_find_funcnames(addr, end_addr);
        for (int ii = 0; ii < comVec.size(); ii++)
        {
          fprintf(fil, "%s\n", comVec.at(ii).c_str());
        }
        fclose(fil);
      } // endif malloc OK

      if (mouse_button == 3)
      {
        int p = system("/home/nstoler/projects/rz-ghidra/ghidra/ghidra/Ghidra/Features/Decompiler/src/decompile/cpp/ghidra_test_dbg -sleighpath /home/nstoler/projects/rz-ghidra/ghidra -path /home/nstoler/projects/datatests datatests > /dev/null");
        if( p != 0) { 
//          gdb_printf("problemos!");
           TUI_DISASMOT_WIN->erase_data_content( "Decompiler Error!");
        }
        isDecompiler = true;
      }
      showAddr = addr;

#if 0
                  // just dump pit for now on the cmd window
		  fil = fopen( "/tmp/decompiler.txt", "r");
                  if( fil != NULL)
                  {
                     char l[1024];
                     while( fgets( l, sizeof( l), fil)) {
                         gdb_printf( "%s\n", l);
                     } // endwhile
                     fclose( fil);
                  }
#endif

      if (TUI_DISASMOT_WIN != nullptr)
      {
        // gdb_printf( "have xor\n\n");
        struct gdbarch *gdbarch = get_current_arch();

        int _x = TUI_DISASMOT_WIN->x;
        int _y = TUI_DISASMOT_WIN->y;
        int _w = TUI_DISASMOT_WIN->width;
        int _h = TUI_DISASMOT_WIN->height;
        _y = mouse_y + TUI_DISASM_WIN->y + 2;
        _x = mouse_x + TUI_DISASM_WIN->x + 1;
        _w = 100;
        _h = 20;

        TUI_DISASMOT_WIN->resize(_h, _w, _x, _y);
        TUI_DISASMOT_WIN->make_visible(true);
        TUI_DISASMOT_WIN->isVisible = true;
        // TUI_DISASMOT_WIN->refill();
        tui_apply_current_layout(true);
        tui_update_ontop_windows_with_addr(gdbarch, addr);

        found = true;
      }
      //                    break;
    } // found one of calls
  }   // if have enough lines in y

  if (TUI_DISASMOT_WIN != nullptr && (!found))
  {
    // gdb_printf( "have xor\n\n");
    TUI_DISASMOT_WIN->isVisible = false;
    tui_apply_current_layout(true);
    TUI_DISASMOT_WIN->make_visible(false);
    TUI_DISASMOT_WIN->refill();
  }

  // execute_command( "x/100bx $ax", false);
#if 0     
      int _first_element_no = first_data_item_displayed ();
      int _line_no = 0, i;

      for( i = _first_element_no; i < m_regs_content.size(); i++)
      {
         _line_no = line_from_reg_element_no ( i);
         if( _line_no == mouse_y) break;
	    }
      int per_line = ( i - _first_element_no + 1) / _line_no;
      int separator = ( width - 2) / per_line;
      i += mouse_x / separator;
         
      //gdb_printf( "mouse @%d:%d, %d, element=%d\n", mouse_x, mouse_y, mouse_button, i);
  
      execute_command( m_regs_content.at(i).cmd.c_str(), false);
#endif
} // endfunc




void
tui_disasm_ontop_window::erase_data_content (const char *prompt)
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
  
  wrefresh (handle.get());

  //tui_wrefresh (handle.get ());
}




/**************************************************************/
std::string tui_diasm_remove_ansi_colors(std::string _line)
{
  const char *lineptr = _line.c_str();

  /* Init the line with the line number.  */
  std::string line;

  char c;
  do
  {
    int skip_bytes;

    c = *lineptr;
    if (c == '\033' && skip_ansi_escape(lineptr, &skip_bytes))
    {
      /* We always have to preserve escapes.  */
      // result.append (lineptr, lineptr + skip_bytes);
      lineptr += skip_bytes;
      continue;
    }
    if (c == '\0')
      break;

    line.push_back(c);

    ++lineptr;
  } while (c != '\0' && c != '\n' && c != '\r');
  return line;
} // endfunc remove ansu

/******************************************************/
// parses an asm line for calls/branches and returns
// the target address
/******************************************************/
CORE_ADDR tui_disasm_parse_line(std::string asm_line)
{
  CORE_ADDR addr = 0L;

  for (int q = 0; q < calls.size(); q++)
  {
    std::size_t _found = asm_line.find(calls.at(q));

    if (_found != std::string::npos)
    {
      std::string line = tui_diasm_remove_ansi_colors(asm_line);

      // gdb_printf( "line=%s", line.c_str());

      // find current address of asm line for jumps with rip
      // this will not work because the address is somewhere else...
      std::size_t frip = line.find("0x");
      std::size_t fripend = line.find(" ", frip);
      std::string rip_str = line.substr(frip, fripend - frip + 1);
      // unused                  value *rip = parse_and_eval( rip_str.c_str());

      // find the opcode's action
      std::size_t __found = line.find(calls.at(q));
      std::size_t freg = line.find("*", __found); // in att disassembly flavor of x86

      // indirect e.g. *rax
      if (freg != std::string::npos)
      {
        std::size_t fpar = line.find(")", freg);
        std::size_t fper = line.find("%", freg);
        std::size_t fspc = line.find(" ", freg);

        std::string regjump;

        if (fpar == std::string::npos && fper != std::string::npos && fspc > fper) // jmp *%rax ---- for example
        {
          regjump = "$" + line.substr(fper + 1, fspc - fper - 1 + 1); // jump over the * and %
        }
        // NS 11/11 !!FIX THIS
        else if (fpar != std::string::npos && fspc > fpar) // jmp *0x44(%rip) --- for example
        {
          regjump = "$" + line.substr(fper + 1, fpar - 1 - fper - 1 + 1) + " + " + line.substr(freg + 1, fper - 2 - freg - 1 + 1);
          tui_disasm_str_replace(regjump, "$rip", rip_str);
        }
        struct value *val = parse_and_eval(regjump.c_str());
        addr = value_as_address(val);
        // gdb_printf( ">>>2 parse: %s=%lx", regjump.c_str(), addr);
      } // endif found indirect jump

      // ----- just a const jump with address -----
      else
      {
        std::size_t __found2 = line.find(calls.at(q));
        std::size_t fzerox = line.find("0x", __found2);
        if (fzerox != std::string::npos)
        {
          std::size_t fspace = line.find(" ", fzerox);
          std::string address = line.substr(fzerox + 2, fspace);
          addr = std::stoul(address, nullptr, 16);
        }
      } // endelse
    }   // endnif !found
  }     // endfor
  return addr;
} // endfunc

/******************************************************/
/* A short and good string replace function	      */
/* should be moved to some tui utils		      */
/******************************************************/
bool tui_disasm_str_replace(std::string &str, const std::string &from, const std::string &to)
{
  size_t start_pos = str.find(from);
  if (start_pos == std::string::npos)
    return false;
  str.replace(start_pos, (size_t)from.length(), to);
  return true;
} // endfunc **replace**

CORE_ADDR tui_disasm_find_maybe_end_of_func(CORE_ADDR from)
{
  std::vector<tui_asm_line> asmlines;
  struct gdbarch *gdbarch = get_current_arch();

  tui_disassemble(gdbarch, asmlines, from, 0x3000);
  for (int i = 0; i < asmlines.size(); i++)
  {
    // just a silly search for now
    std::string disas = asmlines.at(i).insn;

    size_t f = disas.find("ret");
    if (f != std::string::npos)
      return asmlines.at(i + 1).addr;
  }
  return 0L;
} // endfunc

/////////////////////////////////////////////////////////////////////////
std::vector<std::string> tui_disasm_find_funcnames(CORE_ADDR from, CORE_ADDR to)
{
  std::vector<std::string> retVec;
  std::vector<tui_asm_line> asmlines;
  struct gdbarch *gdbarch = get_current_arch();
  CORE_ADDR addr;
  int iq;

  CORE_ADDR pc = from;

  //! gdb_printf( "find %lu %lu", from , to);

  for (iq = 0; iq < (int)(to - from + 2); iq++)
  {
    pc = tui_disassemble(gdbarch, asmlines, pc, 1);

    if (pc > to && to > 0L)
    {
      break;
    }
  } // endfor
  tui_disassemble(gdbarch, asmlines, from, iq);

  // just testing
  addr = tui_disasm_check_load_add(asmlines);

  // gdb_printf( "asmline=%ld", asmlines.size());
  std::string funcname;

  for (int i = 0; i < asmlines.size(); i++)
  {
    // just a silly search for now
    std::string disas = asmlines.at(i).insn;
    addr = 0L;

    {
      char str[500];

      for (int q = 0; q < calls.size(); q++)
      {
        std::size_t _found = disas.find(calls.at(q));

        if (_found != std::string::npos)
        {

          // check for function call names in instructions
          funcname = tui_disasm_parse_for_funcnames(disas);
          if (funcname != "")
          {
            std::size_t fzerox = disas.find("0x", _found);
            if (fzerox != std::string::npos)
            {
              std::size_t fspace = disas.find(" ", fzerox);
              std::string address = disas.substr(fzerox + 2, fspace);
              addr = std::stoul(address, nullptr, 16);
            }
          }
          break;
        } // endif found
      }   // endfor
      if (addr != 0L)
      {
        if( funcname.size() > 100)
           funcname.resize( 100);
        sprintf(str, "<symbol space=\"ram\" offset=\"0x%lx\" name=\"%s\"/>\n", addr, funcname.c_str());
        retVec.push_back(std::string(str));
      }
    } // endif found funcname
  }   // endfor all asm lines
  return retVec;
} // endfunc

#if 1
///////////////////////////////////////////////////////////////////////////////////
std::string tui_disasm_parse_for_funcnames(std::string asm_line)
{
  std::string result = "";

  std::string line = tui_diasm_remove_ansi_colors(asm_line);

  std::size_t ad = line.find("0x");
  std::size_t adsp = line.find(" ", ad);
  std::size_t adss = line.find("<", adsp);
  if (adss == adsp + 1)
  {
    // check if have @plt address
    std::size_t plt = adss;
    if (plt > 0)
    {
      std::size_t plt2 = line.find(">", plt);
      if (plt2 > 0)
        result = line.substr(plt + 1, plt2 - 1 - plt);
    }
  }
  gdb_printf( "result=%s", result.c_str());
  return result;
} // endfunc
#endif

#if 1
/***************************************************************/
CORE_ADDR tui_disasm_check_load_add(std::vector<tui_asm_line> asmlines)
{
  std::vector<std::string> retVec;
  tui_asm_line _asm;
  std::string _asmstr;

  for (int i = 0; i < asmlines.size(); i++)
  {
    _asm = asmlines.at(i);
    _asmstr = _asm.insn;

    for (int q = 0; q < loads.size(); q++)
    {
      std::size_t _found = _asmstr.find(loads.at(q));
      if (_found != std::string::npos)
      {
        std::string line = tui_diasm_remove_ansi_colors(_asmstr);
        std::size_t _found2 = line.find(loads.at(q));
        std::size_t _found3 = line.find("# ", _found2);
        if (_found3 != std::string::npos)
        {
          std::size_t _foundZeroX = line.find("0x", _found3);
          std::size_t _found4 = line.find(" ", _foundZeroX);
          std::string adds = line.substr(_foundZeroX, _found4 - _foundZeroX + 1);

          struct value *val = parse_and_eval(adds.c_str());
          CORE_ADDR addr = value_as_address(val);
          // gdb_printf( "))) found %lx", addr);
          return (addr);
        }
      } // endif
    }
  } // endfor all asmlines
  return 0L;

} // endfunc
#endif

#if 0
/////////////////////////////////////////////////////////
CORE_ADDR tui_disasm_is_abs( std::string line, int fopcode)
{
CORE_ADDR addr = 0L;


          std::size_t frip = line.find( "0x", fopcode);
//          std::size_t fripend = line.find( " ", frip);
//	  std::string rip_str = line.substr( frip, fripend - frip + 1);

          std::size_t freg = line.find( "*", fopcode);		// in att disassembly flavor of x86
          
          // indirect e.g. *rax
          if( freg != std::string::npos)
          {
             return 0L;
	  } // endif found indirect jump

	  // ----- just a const with address -----
          else
          {
             std::size_t fzerox = frip;
             if( fzerox != std::string::npos)
             {
                std::size_t fspace = line.find( " ", fzerox);
                std::string address = line.substr( fzerox + 2, fspace);
                addr = std::stoul( address, nullptr, 16);
             }
          } // endelse
	return addr;
} // endfunc
#endif

/*********************************************************/
// missing string printf
// this is safe and convenient but not exactly efficient
/*********************************************************/
inline std::string tui_disasm_format(const char *fmt, ...)
{
  int size = 512;
  char *buffer = 0;
  buffer = new char[size];
  va_list vl;
  va_start(vl, fmt);
  int nsize = vsnprintf(buffer, size, fmt, vl);
  if (size <= nsize)
  { // fail delete buffer and try again
    delete[] buffer;
    buffer = 0;
    buffer = new char[nsize + 1]; //+1 for /0
    nsize = vsnprintf(buffer, size, fmt, vl);
  }
  std::string ret(buffer);
  va_end(vl);
  delete[] buffer;
  return ret;
}

/* Extract the contents of the value as a string whose length is LENGTH,
   and store the result in DEST.  */
#if 0
static void
tui_disasm_value_as_string (char *dest, struct value *val, int length)
{
  memcpy (dest, value_contents (val).data (), length);
  dest[length] = '\0';
}
#endif
