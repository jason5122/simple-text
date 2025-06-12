#include "base/buffer/aho_corasick/ac_fast.h"
#include "base/buffer/aho_corasick/ac_slow.h"
#include <cassert>

namespace base {

uint32 ACConverter::calculate_state_size(const ACSlowState* s) const {
    ACState dummy;
    uint32 sz = offsetof(ACState, input_vect);
    sz += s->goto_num() * sizeof(dummy.input_vect[0]);

    if (sz < sizeof(ACState)) sz = sizeof(ACState);

    uint32 align = __alignof__(dummy);
    sz = (sz + align - 1) & ~(align - 1);
    return sz;
}

ACBuffer* ACConverter::alloc_buffer() {
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
        state_sz += calculate_state_size(*i);
    }
    state_sz -= calculate_state_size(root_state);

    sz += state_sz;

    // Step 2: Allocate buffer, and populate header.
    ACBuffer* buf = reinterpret_cast<ACBuffer*>(new unsigned char[sz]);

    buf->buf_len = sz;
    buf->root_goto_ofst = root_goto_ofst;
    buf->states_ofst_ofst = states_ofst_ofst;
    buf->first_state_ofst = first_state_ofst;
    buf->root_goto_num = root_fanout;
    buf->state_num = _acs.state_num();
    return buf;
}

void ACConverter::populate_root_goto_func(ACBuffer* buf, GotoVect& goto_vect) {
    unsigned char* buf_base = reinterpret_cast<unsigned char*>(buf);
    input_t* root_gotos = static_cast<input_t*>(buf_base + buf->root_goto_ofst);
    const ACSlowState* root_state = _acs.root();

    root_state->Get_Sorted_Gotos(goto_vect);

    // Renumber the ID of root-node's immediate kids.
    uint32 new_id = 1;
    bool full_fantout = (goto_vect.size() == 255);
    if (!full_fantout) [[likely]] {
        memset(root_gotos, '\0', 256 * sizeof(input_t));
    }

    for (auto i = goto_vect.begin(), e = goto_vect.end(); i != e; i++, new_id++) {
        input_t c = i->first;
        ACSlowState* s = i->second;
        _id_map[s->id()] = new_id;

        if (!full_fantout) [[likely]] {
            root_gotos[c] = new_id;
        }
    }
}

ACBuffer* ACConverter::convert() {
    // Step 1: Some preparation stuff.
    GotoVect gotovect;

    _id_map.clear();
    _ofst_map.clear();
    _id_map.resize(_acs.next_node_id());
    _ofst_map.resize(_acs.next_node_id());

    // Step 2: allocate buffer to accommodate the entire AC graph.
    ACBuffer* buf = alloc_buffer();
    unsigned char* buf_base = reinterpret_cast<unsigned char*>(buf);

    // Step 3: Root node need special care.
    populate_root_goto_func(buf, gotovect);
    buf->root_goto_num = gotovect.size();
    _id_map[_acs.root()->id()] = 0;

    // Step 4: Converting the remaining states by BFSing the graph.
    // First of all, enter root's immediate kids to the working list.
    std::vector<const ACSlowState*> wl;
    StateID id = 1;
    for (auto i = gotovect.begin(); i != gotovect.end(); i++, id++) {
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
        input_t* input_vect = new_s->input_vect;
        for (auto i = gotovect.begin(), e = gotovect.end(); i != e; i++, id++, input_idx++) {
            input_vect[input_idx] = i->first;

            ACSlowState* kid = i->second;
            _id_map[kid->id()] = id;
            wl.push_back(kid);
        }

        _ofst_map[old_s->id()] = ofst;
        ofst += calculate_state_size(old_s);
    }

    // This assertion might be useful to catch buffer overflow
    assert(ofst == buf->buf_len);

    // Populate the fail-link field.
    for (auto i = wl.begin(), e = wl.end(); i != e; i++) {
        const ACSlowState* slow_s = *i;
        StateID fast_s_id = _id_map[slow_s->id()];
        ACState* fast_s = reinterpret_cast<ACState*>(buf_base + state_ofst_vect[fast_s_id]);
        if (const ACSlowState* fl = slow_s->fail_link()) {
            StateID id = _id_map[fl->id()];
            fast_s->fail_link = id;
        } else fast_s->fail_link = 0;
    }
    return buf;
}

}  // namespace base
