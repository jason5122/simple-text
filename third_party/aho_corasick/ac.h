#pragma once

#include <string>
#include <vector>

struct ac_t;

/* Create an AC instance. "pattern_v" is a vector of patterns, the length of
 * i-th pattern is specified by "pattern_len_v[i]"; the number of patterns
 * is specified by "vect_len".
 *
 * Return the instance on success, or NUL otherwise.
 */
ac_t* ac_create(const std::vector<std::string>& patterns);

int ac_match(ac_t*, std::string_view str, unsigned int len);

void ac_free(void* ac);
