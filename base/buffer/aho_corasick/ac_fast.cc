#include "ac_fast.h"

#include "ac_slow.h"

#include <algorithm>  // for std::sort
#include <cassert>

namespace base {

uint32 ACConverter::Calc_State_Sz(const ACSlowState* s) const {
    ACState dummy;
    uint32 sz = offsetof(ACState, input_vect);
    sz += s->goto_num() * sizeof(dummy.input_vect[0]);

    if (sz < sizeof(ACState)) sz = sizeof(ACState);

    uint32 align = __alignof__(dummy);
    sz = (sz + align - 1) & ~(align - 1);
    return sz;
}

ACBuffer* ACConverter::Alloc_Buffer() {
    const std::vector<ACSlowState*>& all_states = _acs.all_states();
    const ACSlowState* root_state = _acs.root();
    uint32 root_fanout = root_state->goto_num();

    // Step 1: Calculate the buffer size
    ACOffset root_goto_ofst, states_ofst_ofst, first_state_ofst;

    // part 1 :  buffer header
    uint32 sz = root_goto_ofst = sizeof(ACBuffer);

    // part 2: Root-node's goto function
    if (root_fanout != 255) [[likely]] {
        sz += 256;
    } else {
        root_goto_ofst = 0;
    }

    // part 3: mapping of state's relative position.
    unsigned align = __alignof__(ACOffset);
    sz = (sz + align - 1) & ~(align - 1);
    states_ofst_ofst = sz;

    sz += sizeof(ACOffset) * all_states.size();

    // part 4: state's contents
    align = __alignof__(ACState);
    sz = (sz + align - 1) & ~(align - 1);
    first_state_ofst = sz;

    uint32 state_sz = 0;
    for (auto i = all_states.begin(), e = all_states.end(); i != e; i++) {
        state_sz += Calc_State_Sz(*i);
    }
    state_sz -= Calc_State_Sz(root_state);

    sz += state_sz;

    // Step 2: Allocate buffer, and populate header.
    ACBuffer* buf = _buf_alloc.alloc(sz);

    buf->buf_len = sz;
    buf->root_goto_ofst = root_goto_ofst;
    buf->states_ofst_ofst = states_ofst_ofst;
    buf->first_state_ofst = first_state_ofst;
    buf->root_goto_num = root_fanout;
    buf->state_num = _acs.state_num();
    return buf;
}

void ACConverter::Populate_Root_Goto_Func(ACBuffer* buf, GotoVect& goto_vect) {
    unsigned char* buf_base = (unsigned char*)(buf);
    InputTy* root_gotos = (InputTy*)(buf_base + buf->root_goto_ofst);
    const ACSlowState* root_state = _acs.root();

    root_state->Get_Sorted_Gotos(goto_vect);

    // Renumber the ID of root-node's immediate kids.
    uint32 new_id = 1;
    bool full_fantout = (goto_vect.size() == 255);
    if (!full_fantout) [[likely]] {
        memset(root_gotos, '\0', 256 * sizeof(InputTy));
    }

    for (auto i = goto_vect.begin(), e = goto_vect.end(); i != e; i++, new_id++) {
        InputTy c = i->first;
        ACSlowState* s = i->second;
        _id_map[s->id()] = new_id;

        if (!full_fantout) [[likely]] {
            root_gotos[c] = new_id;
        }
    }
}

ACBuffer* ACConverter::Convert() {
    // Step 1: Some preparation stuff.
    GotoVect gotovect;

    _id_map.clear();
    _ofst_map.clear();
    _id_map.resize(_acs.next_node_id());
    _ofst_map.resize(_acs.next_node_id());

    // Step 2: allocate buffer to accommodate the entire AC graph.
    ACBuffer* buf = Alloc_Buffer();
    unsigned char* buf_base = (unsigned char*)buf;

    // Step 3: Root node need special care.
    Populate_Root_Goto_Func(buf, gotovect);
    buf->root_goto_num = gotovect.size();
    _id_map[_acs.root()->id()] = 0;

    // Step 4: Converting the remaining states by BFSing the graph.
    // First of all, enter root's immediate kids to the working list.
    std::vector<const ACSlowState*> wl;
    StateID id = 1;
    for (auto i = gotovect.begin(), e = gotovect.end(); i != e; i++, id++) {
        ACSlowState* s = i->second;
        wl.push_back(s);
        _id_map[s->id()] = id;
    }

    ACOffset* state_ofst_vect = (ACOffset*)(buf_base + buf->states_ofst_ofst);
    ACOffset ofst = buf->first_state_ofst;
    for (uint32 idx = 0; idx < wl.size(); idx++) {
        const ACSlowState* old_s = wl[idx];
        ACState* new_s = (ACState*)(buf_base + ofst);

        // This property should hold as we:
        //  - States are appended to worklist in the BFS order.
        //  - sibling states are appended to worklist in the order of their
        //    corresponding input.
        //
        StateID state_id = idx + 1;
        assert(_id_map[old_s->id()] == state_id);

        state_ofst_vect[state_id] = ofst;

        new_s->first_kid = wl.size() + 1;
        new_s->depth = old_s->depth();
        new_s->is_term = old_s->is_terminal() ? old_s->pattern_index() + 1 : 0;

        uint32 gotonum = old_s->goto_num();
        new_s->goto_num = gotonum;

        // Populate the "input" field
        old_s->Get_Sorted_Gotos(gotovect);
        uint32 input_idx = 0;
        uint32 id = wl.size() + 1;
        InputTy* input_vect = new_s->input_vect;
        for (auto i = gotovect.begin(), e = gotovect.end(); i != e; i++, id++, input_idx++) {
            input_vect[input_idx] = i->first;

            ACSlowState* kid = i->second;
            _id_map[kid->id()] = id;
            wl.push_back(kid);
        }

        _ofst_map[old_s->id()] = ofst;
        ofst += Calc_State_Sz(old_s);
    }

    // This assertion might be useful to catch buffer overflow
    assert(ofst == buf->buf_len);

    // Populate the fail-link field.
    for (auto i = wl.begin(), e = wl.end(); i != e; i++) {
        const ACSlowState* slow_s = *i;
        StateID fast_s_id = _id_map[slow_s->id()];
        ACState* fast_s = (ACState*)(buf_base + state_ofst_vect[fast_s_id]);
        if (const ACSlowState* fl = slow_s->fail_link()) {
            StateID id = _id_map[fl->id()];
            fast_s->fail_link = id;
        } else fast_s->fail_link = 0;
    }
    return buf;
}

namespace {
inline ACState* Get_State_Addr(unsigned char* buf_base, ACOffset* StateOfstVect, uint32 state_id) {
    assert(state_id != 0 && "root node is handled in speical way");
    assert(state_id < ((ACBuffer*)buf_base)->state_num);
    return (ACState*)(buf_base + StateOfstVect[state_id]);
}

// The performance of the binary search is critical to this work. This is a modified version of
// binary search that seems to perform faster.
inline bool Binary_Search_Input(InputTy* input_vect, int vect_len, InputTy input, int& idx) {
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
    //
    int low = 0;
    int high = vect_len - 1;
    while (low <= high) {
        int mid = (low + high) >> 1;
        InputTy mid_c = input_vect[mid];

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

AhoCorasick::MatchResult Match(ACBuffer* buf, const PieceTree& tree) {
    unsigned char* buf_base = (unsigned char*)(buf);
    unsigned char* root_goto = buf_base + buf->root_goto_ofst;
    ACOffset* states_ofst_vect = (ACOffset*)(buf_base + buf->states_ofst_ofst);

    ACState* state = 0;
    // TODO: Implement starting/stopping at a specific index.
    TreeWalker walker{&tree};

    // Skip leading chars that are not valid input of root-nodes.
    if (buf->root_goto_num != 255) [[likely]] {
        while (!walker.exhausted()) {
            unsigned char c = walker.next();
            if (unsigned char kid_id = root_goto[c]) {
                state = Get_State_Addr(buf_base, states_ofst_vect, kid_id);
                break;
            }
        }
    } else {
        // TODO: Is this correct? Reference the original implementation to see if we transcribed it
        // correctly.
        unsigned char c = walker.next();
        state = Get_State_Addr(buf_base, states_ofst_vect, c);
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
        found = Binary_Search_Input(state->input_vect, state->goto_num, c, res);
        if (found) {
            // The "t = goto(c, current_state)" is valid, advance to state "t".
            uint32 kid = state->first_kid + res;
            state = Get_State_Addr(buf_base, states_ofst_vect, kid);
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
                    InputTy c = walker.next();
                    if (unsigned char kid_id = root_goto[c]) {
                        state = Get_State_Addr(buf_base, states_ofst_vect, kid_id);
                        break;
                    }
                }
            } else {
                state = Get_State_Addr(buf_base, states_ofst_vect, fl);
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

}  // namespace base
