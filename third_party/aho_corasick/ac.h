#pragma once

struct ac_t;

/* Create an AC instance. "pattern_v" is a vector of patterns, the length of
 * i-th pattern is specified by "pattern_len_v[i]"; the number of patterns
 * is specified by "vect_len".
 *
 * Return the instance on success, or NUL otherwise.
 */
ac_t* ac_create(const char** pattern_v, unsigned int* pattern_len_v, unsigned int vect_len);

int ac_match(ac_t*, const char* str, unsigned int len);

void ac_free(void*);
