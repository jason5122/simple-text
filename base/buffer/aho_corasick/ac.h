#pragma once

#include <string>
#include <vector>

/* If the subject-string doesn't match any of the given patterns, "match_begin"
 * should be a negative; otherwise the substring of the subject-string,
 * starting from offset "match_begin" to "match_end" inclusively,
 * should exactly match the pattern specified by the 'pattern_idx' (i.e.
 * the pattern is "pattern_v[pattern_idx]" where the "pattern_v" is the
 * first actual argument passing to ac_create())
 */
struct ac_result_t {
    int match_begin;
    int match_end;
    int pattern_idx;
};

struct ac_t;

// Return the instance on success, or null otherwise.
ac_t* ac_create(const std::vector<std::string>& patterns);

ac_result_t ac_match(ac_t*, std::string_view str, unsigned int len);

void ac_free(void*);
