/* environ.c — getenv / putenv, and system (<stdlib.h>)
 *
 * The environment starts empty (the MEGA65 has none); putenv adds
 * "NAME=value" strings (the strings themselves, as POSIX's putenv: the
 * program keeps them) to a malloc'd list, replacing one of the same name,
 * and a string without '=' removes the name. system has no command
 * processor to run: 0 for NULL (none there), -1 for a command.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

static char **env_list;
static unsigned int env_count, env_room;

/* The entry for the name of s (up to '=' or its end), or env_count */
static unsigned int env_find(const char *s) {
    unsigned int i, n = 0;
    while (s[n] && s[n] != '=') n++;
    for (i = 0; i < env_count; i++)
        if (!strncmp(env_list[i], s, n) && env_list[i][n] == '=') return i;
    return env_count;
}

char *getenv(const char *name) {
    unsigned int i;
    if (!name || strchr(name, '=')) return NULL;
    i = env_find(name);
    if (i == env_count) return NULL;
    return env_list[i] + strlen(name) + 1;
}

int putenv(char *string) {
    unsigned int i;
    char **grown;
    if (!string || !*string || *string == '=') {
        errno = EINVAL;
        return -1;
    }
    i = env_find(string);
    if (!strchr(string, '=')) {
        if (i < env_count) env_list[i] = env_list[--env_count];
        return 0;
    }
    if (i < env_count) {
        env_list[i] = string;
        return 0;
    }
    if (env_count == env_room) {
        grown = (char **)realloc(env_list, (env_room + 4) * sizeof(char *));
        if (!grown) {
            errno = ENOMEM;
            return -1;
        }
        env_list = grown;
        env_room += 4;
    }
    env_list[env_count++] = string;
    return 0;
}

int system(const char *command) {
    return command ? -1 : 0;
}
