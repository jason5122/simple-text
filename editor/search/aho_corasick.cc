#include "editor/search/ac_fast.h"
#include "editor/search/ac_slow.h"
#include "editor/search/aho_corasick.h"
#include <spdlog/spdlog.h>

namespace editor {

AhoCorasick::AhoCorasick(const std::vector<std::string>& patterns) {
    if (patterns.size() >= 65535) {
        // TODO: Currently we use 16-bit to encode pattern-index (see the comment to
        // AC_State::is_term), therefore we are not able to handle pattern set with more than 65535
        // entries.
        spdlog::error("Error: Pattern limit of 65535 exceeded in AhoCorasick constructor.");
        std::abort();
    }

    ACSlowConstructor acc;
    acc.construct(patterns);

    ACConverter cvt{acc};
    ACBuffer* buf = cvt.convert();
    this->buf = static_cast<void*>(buf);
}

AhoCorasick::~AhoCorasick() {
    ACBuffer* buf = static_cast<ACBuffer*>(this->buf);
    const char* b = reinterpret_cast<const char*>(buf);
    delete[] b;
}

namespace {
inline ACState* get_state_addr(unsigned char* buf_base, ACOffset* StateOfstVect, uint32 state_id) {
    assert(state_id != 0 && "root node is handled in speical way");
    assert(state_id < ((ACBuffer*)buf_base)->state_num);
    return (ACState*)(buf_base + StateOfstVect[state_id]);
}

// The performance of the binary search is critical to this work. This is a modified version of
// binary search that seems to perform faster.
inline bool binary_search_input(input_t* input_vect, int vect_len, input_t input, int& idx) {
    if (vect_len <= 8) {
        for (int i = 0; i < vect_len; ++i) {
            if (input_vect[i] == input) {
                idx = i;
                return true;
            }
        }
        return false;
    }

    // The "low" and "high" must be signed integers, as they could become -1.
    // Also since they are signed integer, "(low + high)/2" is slightly more
    // expensive than (low+high)>>1 or ((unsigned)(low + high))/2.

    int low = 0;
    int high = vect_len - 1;
    while (low <= high) {
        int mid = (low + high) >> 1;
        input_t mid_c = input_vect[mid];

        if (input < mid_c) {
            high = mid - 1;
        } else if (input > mid_c) {
            low = mid + 1;
        } else {
            idx = mid;
            return true;
        }
    }
    return false;
}
}  // namespace

AhoCorasick::MatchResult AhoCorasick::match(const PieceTree& tree) const {
    ACBuffer* buf = static_cast<ACBuffer*>(this->buf);

    unsigned char* buf_base = reinterpret_cast<unsigned char*>(buf);
    unsigned char* root_goto = buf_base + buf->root_goto_ofst;
    ACOffset* states_ofst_vect = reinterpret_cast<ACOffset*>(buf_base + buf->states_ofst_ofst);

    ACState* state = 0;
    // TODO: Implement starting/stopping at a specific index.
    TreeWalker walker{tree};

    // Skip leading chars that are not valid input of root-nodes.
    if (buf->root_goto_num != 255) [[likely]] {
        while (!walker.exhausted()) {
            unsigned char c = walker.next();
            if (unsigned char kid_id = root_goto[c]) {
                state = get_state_addr(buf_base, states_ofst_vect, kid_id);
                break;
            }
        }
    } else {
        // TODO: Is this correct? Reference the original implementation to see if we transcribed it
        // correctly.
        unsigned char c = walker.next();
        state = get_state_addr(buf_base, states_ofst_vect, c);
    }

    if (state != 0) [[likely]] {
        if (state->is_term) [[unlikely]] {
            uint32 idx = walker.offset();
            /* Dictionary may have string of length 1 */
            return {
                .match_begin = static_cast<int>(idx - state->depth),
                .match_end = static_cast<int>(idx - 1),
                .pattern_idx = state->is_term - 1,
            };
        }
    }

    while (!walker.exhausted()) {
        unsigned char c = walker.current();
        int res;
        bool found;
        found = binary_search_input(state->input_vect, state->goto_num, c, res);
        if (found) {
            // The "t = goto(c, current_state)" is valid, advance to state "t".
            uint32 kid = state->first_kid + res;
            state = get_state_addr(buf_base, states_ofst_vect, kid);
            walker.next();
        } else {
            // Follow the fail-link.
            StateID fl = state->fail_link;
            if (fl == 0) {
                // fail-link is root-node, which implies the root-node doesn't
                // have 255 valid transitions (otherwise, the fail-link should
                // points to "goto(root, c)"), so we don't need speical handling
                // as we did before this while-loop is entered.
                //
                while (!walker.exhausted()) {
                    input_t c = walker.next();
                    if (unsigned char kid_id = root_goto[c]) {
                        state = get_state_addr(buf_base, states_ofst_vect, kid_id);
                        break;
                    }
                }
            } else {
                state = get_state_addr(buf_base, states_ofst_vect, fl);
            }
        }

        // Check to see if the state is terminal state?
        if (state->is_term) {
            uint32 idx = walker.offset();
            return {
                .match_begin = static_cast<int>(idx - state->depth),
                .match_end = static_cast<int>(idx - 1),
                .pattern_idx = state->is_term - 1,
            };
        }
    }

    return {-1, -1, -1};
}

}  // namespace editor
