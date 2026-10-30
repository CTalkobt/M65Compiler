/* unistd.h — sleep and getopt for cc45 / MEGA65 (as cc65's) */
#pragma once

/* Waits seconds seconds (clock()'s frames); 0 */
__fastcall__ unsigned int sleep(unsigned int seconds);

/* POSIX getopt (lib/stdlib/getopt.c): the next option letter of argv
 * (main's, see doc/bin/cc45.md: RUN : REM a, b ...) found in optstring,
 * -1 after the last option ("--" ends them too); a letter followed by ':'
 * takes an argument (optarg: the rest of the word, or the next one). '?'
 * for an unknown option or a missing argument (':' for the latter when
 * optstring starts with ':'), with a message on stderr unless opterr is 0
 * or optstring starts with ':'; optopt is the letter. optind: the next
 * argv element. */
int getopt(int argc, char *const argv[], const char *optstring);
extern char *optarg;
extern int optind, opterr, optopt;
