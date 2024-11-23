#include "ac_fast.h"

#include "ac_slow.h"

#include <algorithm>  // for std::sort
#include <cassert>

uint32_t AC_Converter::Calc_State_Sz(const ACS_State* s) const {
    AC_State dummy;
    uint32_t sz = offsetof(AC_State, input_vect);
    sz += s->Get_GotoNum() * sizeof(dummy.input_vect[0]);

    if (sz < sizeof(AC_State)) sz = sizeof(AC_State);

    uint32_t align = __alignof__(dummy);
    sz = (sz + align - 1) & ~(align - 1);
    return sz;
}

AC_Buffer* AC_Converter::Alloc_Buffer() {
    const std::vector<ACS_State*>& all_states = _acs.Get_All_States();
    const ACS_State* root_state = _acs.Get_Root_State();
    uint32_t root_fanout = root_state->Get_GotoNum();

    // Step 1: Calculate the buffer size
    uint32_t root_goto_ofst, states_ofst_ofst, first_state_ofst;

    // part 1 :  buffer header
    uint32_t sz = root_goto_ofst = sizeof(AC_Buffer);

    // part 2: Root-node's goto function
    if (root_fanout != 255) [[likely]] {
        sz += 256;
    } else [[unlikely]] {
        root_goto_ofst = 0;
    }

    // part 3: mapping of state's relative position.
    unsigned align = __alignof__(uint32_t);
    sz = (sz + align - 1) & ~(align - 1);
    states_ofst_ofst = sz;

    sz += sizeof(uint32_t) * all_states.size();

    // part 4: state's contents
    align = __alignof__(AC_State);
    sz = (sz + align - 1) & ~(align - 1);
    first_state_ofst = sz;

    uint32_t state_sz = 0;
    for (auto i = all_states.begin(), e = all_states.end(); i != e; i++) {
        state_sz += Calc_State_Sz(*i);
    }
    state_sz -= Calc_State_Sz(root_state);

    sz += state_sz;

    // Step 2: Allocate buffer, and populate header.
    AC_Buffer* buf = _buf_alloc.alloc(sz);

    buf->buf_len = sz;
    buf->root_goto_ofst = root_goto_ofst;
    buf->states_ofst_ofst = states_ofst_ofst;
    buf->first_state_ofst = first_state_ofst;
    buf->root_goto_num = root_fanout;
    buf->state_num = _acs.Get_State_Num();
    return buf;
}

void AC_Converter::Populate_Root_Goto_Func(AC_Buffer* buf, GotoVect& goto_vect) {
    unsigned char* buf_base = (unsigned char*)(buf);
    unsigned char* root_gotos = (unsigned char*)(buf_base + buf->root_goto_ofst);
    const ACS_State* root_state = _acs.Get_Root_State();

    root_state->Get_Sorted_Gotos(goto_vect);

    // Renumber the ID of root-node's immediate kids.
    uint32_t new_id = 1;
    bool full_fantout = (goto_vect.size() == 255);
    if (!full_fantout) [[likely]] {
        bzero(root_gotos, 256 * sizeof(unsigned char));
    }

    for (auto i = goto_vect.begin(), e = goto_vect.end(); i != e; i++, new_id++) {
        unsigned char c = i->first;
        ACS_State* s = i->second;
        _id_map[s->Get_ID()] = new_id;

        if (!full_fantout) [[likely]] {
            root_gotos[c] = new_id;
        }
    }
}

AC_Buffer* AC_Converter::Convert() {
    // Step 1: Some preparation stuff.
    GotoVect gotovect;

    _id_map.clear();
    _ofst_map.clear();
    _id_map.resize(_acs.Get_Next_Node_Id());
    _ofst_map.resize(_acs.Get_Next_Node_Id());

    // Step 2: allocate buffer to accommodate the entire AC graph.
    AC_Buffer* buf = Alloc_Buffer();
    unsigned char* buf_base = (unsigned char*)buf;

    // Step 3: Root node need special care.
    Populate_Root_Goto_Func(buf, gotovect);
    buf->root_goto_num = gotovect.size();
    _id_map[_acs.Get_Root_State()->Get_ID()] = 0;

    // Step 4: Converting the remaining states by BFSing the graph.
    // First of all, enter root's immediate kids to the working list.
    std::vector<const ACS_State*> wl;
    uint32_t id = 1;
    for (auto i = gotovect.begin(), e = gotovect.end(); i != e; i++, id++) {
        ACS_State* s = i->second;
        wl.push_back(s);
        _id_map[s->Get_ID()] = id;
    }

    uint32_t* state_ofst_vect = (uint32_t*)(buf_base + buf->states_ofst_ofst);
    uint32_t ofst = buf->first_state_ofst;
    for (uint32_t idx = 0; idx < wl.size(); idx++) {
        const ACS_State* old_s = wl[idx];
        AC_State* new_s = (AC_State*)(buf_base + ofst);

        // This property should hold as we:
        //  - States are appended to worklist in the BFS order.
        //  - sibling states are appended to worklist in the order of their
        //    corresponding input.
        //
        uint32_t state_id = idx + 1;
        assert(_id_map[old_s->Get_ID()] == state_id);

        state_ofst_vect[state_id] = ofst;

        new_s->first_kid = wl.size() + 1;
        new_s->depth = old_s->Get_Depth();
        new_s->is_term = old_s->is_Terminal() ? old_s->get_Pattern_Idx() + 1 : 0;

        uint32_t gotonum = old_s->Get_GotoNum();
        new_s->goto_num = gotonum;

        // Populate the "input" field
        old_s->Get_Sorted_Gotos(gotovect);
        uint32_t input_idx = 0;
        uint32_t id = wl.size() + 1;
        unsigned char* input_vect = new_s->input_vect;
        for (auto i = gotovect.begin(), e = gotovect.end(); i != e; i++, id++, input_idx++) {
            input_vect[input_idx] = i->first;

            ACS_State* kid = i->second;
            _id_map[kid->Get_ID()] = id;
            wl.push_back(kid);
        }

        _ofst_map[old_s->Get_ID()] = ofst;
        ofst += Calc_State_Sz(old_s);
    }

    // This assertion might be useful to catch buffer overflow
    assert(ofst == buf->buf_len);

    // Populate the fail-link field.
    for (auto i = wl.begin(), e = wl.end(); i != e; i++) {
        const ACS_State* slow_s = *i;
        uint32_t fast_s_id = _id_map[slow_s->Get_ID()];
        AC_State* fast_s = (AC_State*)(buf_base + state_ofst_vect[fast_s_id]);
        if (const ACS_State* fl = slow_s->Get_FailLink()) {
            uint32_t id = _id_map[fl->Get_ID()];
            fast_s->fail_link = id;
        } else fast_s->fail_link = 0;
    }
    return buf;
}

static inline AC_State* Get_State_Addr(unsigned char* buf_base,
                                       uint32_t* StateOfstVect,
                                       uint32_t state_id) {
    assert(state_id != 0 && "root node is handled in speical way");
    assert(state_id < ((AC_Buffer*)buf_base)->state_num);
    return (AC_State*)(buf_base + StateOfstVect[state_id]);
}

static inline bool Binary_Search_Input(unsigned char* input_vect,
                                       int vect_len,
                                       unsigned char input,
                                       int& idx) {
    int low = 0, high = vect_len - 1;
    while (low <= high) {
        int mid = (low + high) >> 1;
        auto mid_c = input_vect[mid];

        if (input < mid_c) high = mid - 1;
        else if (input > mid_c) low = mid + 1;
        else {
            idx = mid;
            return true;
        }
    }
    return false;
}

int Match(AC_Buffer* buf, std::string_view str, uint32_t len) {
    unsigned char* buf_base = (unsigned char*)(buf);
    unsigned char* root_goto = buf_base + buf->root_goto_ofst;
    uint32_t* states_ofst_vect = (uint32_t*)(buf_base + buf->states_ofst_ofst);

    AC_State* state = 0;
    uint32_t idx = 0;

    // Skip leading chars that are not valid input of root-nodes.
    if (buf->root_goto_num != 255) [[likely]] {
        while (idx < len) {
            unsigned char c = str[idx++];
            if (unsigned char kid_id = root_goto[c]) {
                state = Get_State_Addr(buf_base, states_ofst_vect, kid_id);
                break;
            }
        }
    } else {
        state = Get_State_Addr(buf_base, states_ofst_vect, str[idx]);
        ++idx;
    }

    if (state != 0) [[likely]] {
        if (state->is_term) [[unlikely]] {
            /* Dictionary may have string of length 1 */
            return idx - state->depth;
        }
    }

    while (idx < len) {
        unsigned char c = str[idx];
        int res;
        bool found = Binary_Search_Input(state->input_vect, state->goto_num, c, res);
        if (found) {
            // The "t = goto(c, current_state)" is valid, advance to state "t".
            uint32_t kid = state->first_kid + res;
            state = Get_State_Addr(buf_base, states_ofst_vect, kid);
            idx++;
        } else {
            // Follow the fail-link.
            uint32_t fl = state->fail_link;
            if (fl == 0) {
                // fail-link is root-node, which implies the root-node doesn't
                // have 255 valid transitions (otherwise, the fail-link should
                // points to "goto(root, c)"), so we don't need speical handling
                // as we did before this while-loop is entered.
                //
                while (idx < len) {
                    unsigned char c = str[idx++];
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
            return idx - state->depth;
        }
    }
    return -1;
}
