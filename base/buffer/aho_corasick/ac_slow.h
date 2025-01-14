#pragma once

#include "ac_util.h"

#include <algorithm>  // for std::sort
#include <cassert>
#include <map>
#include <stdio.h>
#include <string>
#include <vector>

#include <fmt/base.h>
#include <fmt/format.h>

namespace base {

class ACSlowState;

using ACSlowGotoMap = std::map<InputTy, ACSlowState*>;

struct Match_Result {
    int begin;
    int end;
    int pattern_idx;
};

using GotoPair = std::pair<InputTy, ACSlowState*>;
using GotoVect = std::vector<GotoPair>;

class ACSlowState {
    friend class ACSlowConstructor;

public:
    ACSlowState(uint32 id)
        : _id(id), _pattern_idx(-1), _depth(0), _is_terminal(false), _fail_link(0) {}
    ~ACSlowState() {}

    void Set_Goto(InputTy c, ACSlowState* s) {
        _goto_map[c] = s;
    }
    ACSlowState* Get_Goto(InputTy c) const {
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

    ACSlowState* Get_FailLink() const {
        return _fail_link;
    }
    uint32 Get_GotoNum() const {
        return _goto_map.size();
    }
    uint32 Get_ID() const {
        return _id;
    }
    uint32 Get_Depth() const {
        return _depth;
    }
    const ACSlowGotoMap& Get_Goto_Map(void) const {
        return _goto_map;
    }
    bool is_Terminal() const {
        return _is_terminal;
    }
    int get_Pattern_Idx() const {
        assert(is_Terminal() && _pattern_idx >= 0);
        return _pattern_idx;
    }

private:
    void set_Pattern_Idx(int idx) {
        assert(is_Terminal());
        _pattern_idx = idx;
    }

private:
    uint32 _id;
    int _pattern_idx;
    short _depth;
    bool _is_terminal;
    ACSlowGotoMap _goto_map;
    ACSlowState* _fail_link;
};

class ACSlowConstructor {
public:
    ACSlowConstructor();
    ~ACSlowConstructor();

    void Construct(const std::vector<std::string>& patterns);

    Match_Result Match(const char* s, uint32 len) const {
        Match_Result r = MatchHelper(s, len);
        return r;
    }

    Match_Result Match(const char* s) const {
        return Match(s, strlen(s));
    }

    const ACSlowState* Get_Root_State() const {
        return _root;
    }
    const std::vector<ACSlowState*>& Get_All_States() const {
        return _all_states;
    }

    uint32 Get_Next_Node_Id() const {
        return _next_node_id;
    }
    uint32 Get_State_Num() const {
        return _next_node_id - 1;
    }

private:
    void Add_Pattern(std::string_view str, int pattern_idx);
    ACSlowState* new_state();
    void Propagate_faillink();

    Match_Result MatchHelper(const char*, uint32 len) const;

private:
    ACSlowState* _root;
    std::vector<ACSlowState*> _all_states;
    unsigned char* _root_char;
    uint32 _next_node_id;
};

}  // namespace base
