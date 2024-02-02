#/* SPDX-License-Identifier: Apache-2.0 */
#/* Copyright (c) 2012-2023 Applied Communication Sciences
# * (now Peraton Labs Inc.)
# *
# * This software was developed in work supported by the following U.S.
# * Government contracts:
# *
# * HR0011-20-C-0160 HR0011-16-C-0061
# * 
# *
# * Any opinions, findings and conclusions or recommendations expressed in
# * this material are those of the author(s) and do not necessarily reflect
# * the views, either expressed or implied, of the U.S. Government.
# *
# * DoD Distribution Statement A
# * Approved for Public Release, Distribution Unlimited
# *
# * DISTAR Case 38846, cleared November 1, 2023
# *
# * Licensed under the Apache License, Version 2.0 (the "License");
# * you may not use this file except in compliance with the License.
# * You may obtain a copy of the License at
# *
# * http://www.apache.org/licenses/LICENSE-2.0
*/
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <ncurses.h>
#include <string.h>
void initAlertDisplay(char *TITLE,char *HEAD) {
    int x;
    initscr();
    keypad(stdscr, TRUE);
    (void) nonl();      
    (void) cbreak();   
    (void) echo();    
    if (has_colors()){
        start_color();
        init_pair(1,COLOR_WHITE,COLOR_BLACK);
        init_pair(2,COLOR_RED,COLOR_BLACK);
        init_pair(3,COLOR_YELLOW,COLOR_BLACK);
        init_pair(4,COLOR_BLUE,COLOR_BLACK);
        init_pair(5,COLOR_CYAN,COLOR_WHITE);
        init_pair(6,COLOR_MAGENTA,COLOR_BLACK);
        init_pair(7,COLOR_WHITE,COLOR_BLACK);
    }
    bkgd(COLOR_PAIR(1));
    refresh();
    scrollok(stdscr,TRUE);
    setscrreg(2,LINES-1);
    attron(A_BOLD);
    attron(A_STANDOUT);
    addstr(TITLE);
    attron(A_UNDERLINE);
    addstr(HEAD);
    attroff(A_BOLD);
    attroff(A_UNDERLINE);
    attroff(A_STANDOUT);
    refresh();
    return;
}
