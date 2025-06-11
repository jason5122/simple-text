#pragma once

#include "base/buffer/piece_tree.h"
#include <string>
#include <vector>

namespace base {

class AhoCorasick {
public:
    AhoCorasick(const std::vector<std::string>& patterns);
    ~AhoCorasick();
    // TODO: Implement rule of five.

    /* If the subject-string doesn't match any of the given patterns, "match_begin"
     * should be a negative; otherwise the substring of the subject-string,
     * starting from offset "match_begin" to "match_end" inclusively,
     * should exactly match the pattern specified by the 'pattern_idx' (i.e.
     * the pattern is "pattern_v[pattern_idx]" where the "pattern_v" is the
     * first actual argument passing to ac_create())
     */
    struct MatchResult {
        int match_begin;
        int match_end;
        int pattern_idx;
    };

    MatchResult match(const PieceTree& tree) const;

private:
    void* buf;
};

}  // namespace base
