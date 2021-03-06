/*
 *   
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation. 
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt). 
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA 
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions. 
 */

/*
 * Globals_wince.hpp: Command line switches for the WinCE platform.
 */

#define PLATFORM_RUNTIME_FLAGS(develop, product)                            \
  product(bool, LogConsole, true,                                           \
          "Should we write console output to the \cldc_vm.log?")            \
  product(bool, LogByDate, false,                                           \
          "Append the date/time to the name of the log file?")              \
  product(bool, WriteConsole, true,                                         \
          "Should we write console output to actual console?")              \
  product(bool, StickyConsole, true,                                        \
          "Should console be displayed after VM has exited?")               \
  product(bool, EVCDebug, false,                                            \
          "Set this to true if you're debugging under EVC.")                \
  product(int, TickInterval, 10,                                            \
          "Milliseconds between each timer tick")
