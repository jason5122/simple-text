#pragma once

#include "base/buffer/aho_corasick/ac_slow.h"
#include <vector>

namespace base {

class ACSlowConstructor;

using ACOffset = uint32;
using StateID = uint32;

// The entire "fast" AC graph is converted from its "slow" version, and store
// in an consecutive trunk of memory or "buffer". Since the pointers in the
// fast AC graph are represented as offset relative to the base address of
// the buffer, this fast AC graph is position-independent, meaning cloning
// the fast graph is just to memcpy the entire buffer.
//
// The buffer is laid-out as following:
//
//   1. The buffer header. (i.e. the AC_Buffer content)
//   2. root-node's goto functions. It is represented as an array indiced by
//      root-node's valid inputs, and the element is the ID of the corresponding
//      transition state (aka kid). To save space, we used 8-bit to represent
//      the IDs. ID of root's kids starts with 1.
//
//        Root may have 255 valid inputs. In this special case, i-th element
//      stores value i -- i.e the i-th state. So, we don't need such array
//      at all. On the other hand, 8-bit is insufficient to encode kids' ID.
//
//   3. An array indiced by state's id, and the element is the offset
//      of corresponding state wrt the base address of the buffer.
//
//   4. the contents of states.
//
struct ACBuffer {
    uint32 buf_len;
    ACOffset root_goto_ofst;    // addr of root node's goto() function.
    ACOffset states_ofst_ofst;  // addr of state pointer vector (indiced by id)
    ACOffset first_state_ofst;  // addr of the first state in the buffer.
    uint16 root_goto_num;       // fan-out of root-node.
    uint16 state_num;           // number of states

    // Followed by the gut of the buffer:
    // 1. map: root's-valid-input -> kid's id
    // 2. map: state's ID -> offset of the state
    // 3. states' content.
};

// Depict the state of "fast" AC graph.
struct ACState {
    // transition are sorted. For instance, state s1, has two transitions :
    //   goto(b) -> S_b, goto(a)->S_a. The inputs are sorted in the ascending
    // order, and the target states are permuted accordingly. In this case,
    // the inputs are sorted as : a, b, and the target states are permuted
    // into S_a, S_b. So, S_a is the 1st kid, the ID of kids are consecutive,
    // so we don't need to save all the target kids.
    //
    StateID first_kid;
    ACOffset fail_link;
    short depth;             // How far away from root.
    unsigned short is_term;  // Is terminal node. if is_term != 0, it encodes
                             // the value of "1 + pattern-index".
    unsigned char goto_num;  // The number of valid transition.
    input_t input_vect[1];   // Vector of valid input. Must be last field!
};

class BufAllocator {
public:
    BufAllocator() {}
    virtual ~BufAllocator() { free(); }

    virtual ACBuffer* alloc(int sz) = 0;
    virtual void free() {}
};

// Convert slow-AC-graph into fast one.
class ACConverter {
public:
    ACConverter(ACSlowConstructor& acs, BufAllocator& ba) : _acs(acs), _buf_alloc(ba) {}
    ACBuffer* convert();

private:
    // Return the size in byte needed to to save the specified state.
    uint32 calculate_state_size(const ACSlowState*) const;

    ACBuffer* alloc_buffer();
    void populate_root_goto_func(ACBuffer*, GotoVect&);

private:
    ACSlowConstructor& _acs;
    BufAllocator& _buf_alloc;

    // map: ID of state in slow-graph -> ID of counterpart in fast-graph.
    std::vector<uint32> _id_map;

    // map: ID of state in slow-graph -> offset of counterpart in fast-graph.
    std::vector<ACOffset> _ofst_map;
};

}  // namespace base
