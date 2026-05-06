/* ctype.h — PETSCII-aware character classification for cc45 / MEGA65 */

#pragma once

/*
 * PETSCII character ranges:
 *   Digits    '0'-'9' = $30-$39
 *   Lowercase 'a'-'z' = $41-$5A  (PETSCII default — unshifted)
 *   Uppercase 'A'-'Z' = $C1-$DA  (PETSCII shifted)
 *   Space     $20
 *   CR        $0D  (PETSCII carriage return / Enter)
 *   Tab       $09  (not standard PETSCII, included for compatibility)
 *
 * toupper/tolower convert between the two PETSCII letter ranges.
 */

int isdigit(int c);
int isalpha(int c);
int isalnum(int c);
int isspace(int c);
int isprint(int c);
int toupper(int c);
int tolower(int c);
