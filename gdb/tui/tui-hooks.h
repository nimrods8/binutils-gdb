/* External/Public TUI hools header file, for GDB the GNU debugger.

   Copyright (C) 2004-2022 Free Software Foundation, Inc.

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

#ifndef TUI_TUI_HOOKS_H
#define TUI_TUI_HOOKS_H

extern void tui_install_hooks (void);
extern void tui_remove_hooks (void);
std::vector<std::string> tui_hooks_split(const std::string& s, char seperator);

std::string tui_hooks_get_name_of_rwMaps( CORE_ADDR addr);
std::string tui_hooks_filename2color( std::string filename);

/************************************/
typedef struct
{
   CORE_ADDR   func_addr;
   std::string func_name;
} functions_lookup;

/// @brief This is a util helper function which requests "info func"
///        with a given func name or regexp and returns a vector of addreses and func names
/// @param args regexp or straight func name to look for
/// @return std::vector of struct functions
///
std::vector<functions_lookup> tui_hooks_get_info_func( std::string args);
#endif /* TUI_TUI_HOOKS_H */
