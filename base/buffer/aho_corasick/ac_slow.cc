#include "ac_slow.h"

#include "ac.h"

#include <cassert>

ACS_Constructor::ACS_Constructor() : _next_node_id(1) {
    _root = new_state();
    _root_char = new InputTy[256];
    memset((void*)_root_char, '\0', 256);
}

ACS_Constructor::~ACS_Constructor() {
    for (auto i = _all_states.begin(), e = _all_states.end(); i != e; i++) {
        delete *i;
    }
    _all_states.clear();
    delete[] _root_char;
}

ACS_State* ACS_Constructor::new_state() {
    ACS_State* t = new ACS_State(_next_node_id++);
    _all_states.push_back(t);
    return t;
}

void ACS_Constructor::Add_Pattern(std::string_view str, int pattern_idx) {
    ACS_State* state = _root;
    for (char c : str) {
        ACS_State* new_s = state->Get_Goto(c);
        if (!new_s) {
            new_s = new_state();
            new_s->_depth = state->_depth + 1;
            state->Set_Goto(c, new_s);
        }
        state = new_s;
    }
    state->_is_terminal = true;
    state->set_Pattern_Idx(pattern_idx);
}

void ACS_Constructor::Propagate_faillink() {
    ACS_State* r = _root;
    std::vector<ACS_State*> wl;

    const ACS_Goto_Map& m = r->Get_Goto_Map();
    for (auto i = m.begin(), e = m.end(); i != e; i++) {
        ACS_State* s = i->second;
        s->_fail_link = r;
        wl.push_back(s);
    }

    // For any input c, make sure "goto(root, c)" is valid, which make the
    // fail-link propagation lot easier.
    ACS_Goto_Map goto_save = r->_goto_map;
    for (uint32 i = 0; i <= 255; i++) {
        ACS_State* s = r->Get_Goto(i);
        if (!s) r->Set_Goto(i, r);
    }

    for (uint32 i = 0; i < wl.size(); i++) {
        ACS_State* s = wl[i];
        ACS_State* fl = s->_fail_link;

        const ACS_Goto_Map& tran_map = s->Get_Goto_Map();

        for (auto ii = tran_map.begin(), ee = tran_map.end(); ii != ee; ii++) {
            InputTy c = ii->first;
            ACS_State* tran = ii->second;

            ACS_State* tran_fl = 0;
            for (ACS_State* fl_walk = fl;;) {
                if (ACS_State* t = fl_walk->Get_Goto(c)) {
                    tran_fl = t;
                    break;
                } else {
                    fl_walk = fl_walk->Get_FailLink();
                }
            }

            tran->_fail_link = tran_fl;
            wl.push_back(tran);
        }
    }

    // Remove "goto(root, c) == root" transitions
    r->_goto_map = goto_save;
}

void ACS_Constructor::Construct(const std::vector<std::string>& patterns) {
    for (size_t i = 0; i < patterns.size(); ++i) {
        Add_Pattern(patterns[i], i);
    }

    Propagate_faillink();
    unsigned char* p = _root_char;

    const ACS_Goto_Map& m = _root->Get_Goto_Map();
    for (auto i = m.begin(), e = m.end(); i != e; i++) {
        p[i->first] = 1;
    }
}

Match_Result ACS_Constructor::MatchHelper(const char* str, uint32 len) const {
    const ACS_State* root = _root;
    const ACS_State* state = root;

    uint32 idx = 0;
    while (idx < len) {
        InputTy c = str[idx];
        idx++;
        if (_root_char[c]) {
            state = root->Get_Goto(c);
            break;
        }
    }

    if (state->is_Terminal()) [[unlikely]] {
        // This could happen if the one of the pattern has only one char!
        uint32 pos = idx - 1;
        Match_Result r(pos - state->Get_Depth() + 1, pos, state->get_Pattern_Idx());
        return r;
    }

    while (idx < len) {
        InputTy c = str[idx];
        ACS_State* gs = state->Get_Goto(c);

        if (!gs) {
            ACS_State* fl = state->Get_FailLink();
            if (fl == root) {
                while (idx < len) {
                    InputTy c = str[idx];
                    idx++;
                    if (_root_char[c]) {
                        state = root->Get_Goto(c);
                        break;
                    }
                }
            } else {
                state = fl;
            }
        } else {
            idx++;
            state = gs;
        }

        if (state->is_Terminal()) {
            uint32 pos = idx - 1;
            Match_Result r =
                Match_Result(pos - state->Get_Depth() + 1, pos, state->get_Pattern_Idx());
            return r;
        }
    }

    return Match_Result(-1, -1, -1);
}
