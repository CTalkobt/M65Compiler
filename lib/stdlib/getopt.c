/* getopt.c — POSIX getopt (<unistd.h>)
 *
 * Options are letters after '-' in argv's words, several in one word
 * ("-ab"); a letter followed by ':' in optstring takes an argument: the
 * rest of its word, or the next word. A word that is not an option ("-"
 * alone, or one not starting with '-') or "--" ends them. */

#pragma cc45 signed_default

#include <unistd.h>
#include <string.h>
#include <stdio.h>

char *optarg;
int optind = 1;
int opterr = 1;
int optopt;

static int pos = 1;             /* the letter's place in argv[optind] */

static void complain(const char *argv0, const char *msg, char c, const char *optstring) {
    char letter[2];
    if (!opterr || *optstring == ':') return;
    letter[0] = c;
    letter[1] = 0;
    fputs(argv0 ? argv0 : "getopt", stderr);
    fputs(": ", stderr);
    fputs(msg, stderr);
    fputs(" -- ", stderr);
    fputs(letter, stderr);
    fputs("\n", stderr);
}

int getopt(int argc, char *const argv[], const char *optstring) {
    char c;
    const char *spec;
    char *word;
    optarg = NULL;
    if (pos == 1) {
        if (optind >= argc || !argv[optind]) return -1;
        word = argv[optind];
        if (word[0] != '-' || word[1] == 0) return -1;
        if (word[1] == '-' && word[2] == 0) {
            optind++;
            return -1;
        }
    }
    word = argv[optind];
    c = word[pos];
    optopt = c;
    spec = c == ':' ? NULL : strchr(optstring, c);
    if (!spec) {
        complain(argv[0], "illegal option", c, optstring);
        if (!word[++pos]) {
            optind++;
            pos = 1;
        }
        return '?';
    }
    if (spec[1] == ':') {
        if (word[pos + 1]) {
            optarg = word + pos + 1;
            optind++;
        } else if (optind + 1 < argc) {
            optarg = argv[optind + 1];
            optind += 2;
        } else {
            optind++;
            pos = 1;
            complain(argv[0], "option requires an argument", c, optstring);
            return *optstring == ':' ? ':' : '?';
        }
        pos = 1;
        return c;
    }
    if (!word[++pos]) {
        optind++;
        pos = 1;
    }
    return c;
}
