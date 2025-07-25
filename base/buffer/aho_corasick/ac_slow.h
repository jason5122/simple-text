#pragma once

#include <algorithm>
#include <cassert>
#include <map>
#include <stdio.h>
#include <string>
#include <vector>

namespace base {

class ACSlowState;

// TODO: Clean this up. Don't define it here since we're polluting the namespace.
using uint16 = unsigned short;
using uint32 = unsigned int;
using input_t = unsigned char;

using ACSlowGotoMap = std::map<input_t, ACSlowState*>;

using GotoPair = std::pair<input_t, ACSlowState*>;
using GotoVect = std::vector<GotoPair>;

class ACSlowState {
    friend class ACSlowConstructor;

public:
    ACSlowState(uint32 id)
        : _id(id), _pattern_idx(-1), _depth(0), _is_terminal(false), _fail_link(0) {}
    ~ACSlowState() {}

    void Set_Goto(input_t c, ACSlowState* s) { _goto_map[c] = s; }
    ACSlowState* Get_Goto(input_t c) const {
        auto iter = _goto_map.find(c);
        return iter != _goto_map.end() ? (*iter).second : 0;
    }

    // Return all transitions sorted in the ascending order of their input.
    void Get_Sorted_Gotos(GotoVect& gotos) const {
        const ACSlowGotoMap& m = _goto_map;
        gotos.clear();
        for (auto i = m.begin(), e = m.end(); i != e; i++) {
            gotos.emplace_back(i->first, i->second);
        }
        std::sort(gotos.begin(), gotos.end());
    }

    ACSlowState* fail_link() const { return _fail_link; }
    uint32 goto_num() const { return _goto_map.size(); }
    uint32 id() const { return _id; }
    uint32 depth() const { return _depth; }
    const ACSlowGotoMap& goto_map(void) const { return _goto_map; }
    bool is_terminal() const { return _is_terminal; }
    int pattern_index() const {
        assert(is_terminal() && _pattern_idx >= 0);
        return _pattern_idx;
    }

private:
    uint32 _id;
    int _pattern_idx;
    short _depth;
    bool _is_terminal;
    ACSlowGotoMap _goto_map;
    ACSlowState* _fail_link;

    void set_pattern_index(int idx) {
        assert(is_terminal());
        _pattern_idx = idx;
    }
};

class ACSlowConstructor {
public:
    ACSlowConstructor();
    ~ACSlowConstructor();

    void construct(const std::vector<std::string>& patterns);

    const ACSlowState* root() const { return _root; }
    const std::vector<ACSlowState*>& all_states() const { return _all_states; }

    uint32 next_node_id() const { return _next_node_id; }
    uint32 state_num() const { return _next_node_id - 1; }

private:
    ACSlowState* _root;
    std::vector<ACSlowState*> _all_states;
    unsigned char* _root_char;
    uint32 _next_node_id;

    void add_pattern(std::string_view str, int pattern_idx);
    ACSlowState* new_state();
    void propagate_faillink();
};

}  // namespace base
