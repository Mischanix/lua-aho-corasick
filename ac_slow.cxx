#include <ctype.h>
#include <strings.h> // for bzero
#include <algorithm>
#include "ac_slow.hpp"
#include "ac.h"

//////////////////////////////////////////////////////////////////////////
//
//      Implementation of AhoCorasick_Slow
//
//////////////////////////////////////////////////////////////////////////
//

ACS_Constructor::ACS_Constructor() : _next_node_id(1) {
    _root = new_state();
    _root_char = new InputTy[256];
    memset((void*)_root_char, 0, 256);
}

ACS_Constructor::~ACS_Constructor() {
    for (std::vector<ACS_State* >::iterator i =  _all_states.begin(),
            e = _all_states.end(); i != e; i++) {
        delete *i;
    }
    _all_states.clear();
    delete[] _root_char;
}

ACS_State*
ACS_Constructor::new_state() {
    ACS_State* t = new ACS_State(_next_node_id++);
    _all_states.push_back(t);
    return t;
}

void
ACS_Constructor::Add_String(const char* str, unsigned int str_len) {
    ACS_State* state = _root;
    for (unsigned int i = 0; i < str_len; i++) {
        const char c = str[i];
        ACS_State* new_s = state->Get_Goto(c);
        if (!new_s) {
            new_s = new_state();
            new_s->_depth = state->_depth + 1;
            state->Set_Goto(c, new_s);
        }
        state = new_s;
    }
    state->_is_terminal = true;
}

void
ACS_Constructor::Propagate_faillink() {
    ACS_State* r = _root;
    std::vector<ACS_State*> wl;

    const ACS_Goto_Map& m = r->Get_Goto_Map();
    for (ACS_Goto_Map::const_iterator i = m.begin(), e = m.end(); i != e; i++) {
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

        for (ACS_Goto_Map::const_iterator ii = tran_map.begin(),
                ee = tran_map.end(); ii != ee; ii++) {
            InputTy c = ii->first;
            ACS_State *tran = ii->second;

            ACS_State* tran_fl = 0;
            for (ACS_State* fl_walk = fl; ;) {
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

void
ACS_Constructor::Construct(const char** strv, unsigned int* strlenv,
                           uint32 strnum) {
    for (uint32 i = 0; i < strnum; i++) {
        Add_String(strv[i], strlenv[i]);
    }

    Propagate_faillink();
    unsigned char* p = _root_char;

    const ACS_Goto_Map& m = _root->Get_Goto_Map();
    for (ACS_Goto_Map::const_iterator i = m.begin(), e = m.end();
            i != e; i++) {
        p[i->first] = 1;
    }
}

Match_Result
ACS_Constructor::Match(const char *str, uint32 len)  const {
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
            idx ++;
            state = gs;
        }

        if (state->is_Terminal()) {
            uint32 pos = idx - 1;
            Match_Result r = Match_Result(pos - state->Get_Depth() + 1, pos);
            return r;
        }
    }

    return Match_Result(-1, -1);
}

#ifdef DEBUG
void
ACS_Constructor::dump_text(const char* txtfile) const {
    FILE* f = fopen(txtfile, "w+");
    for (std::vector<ACS_State*>::const_iterator i = _all_states.begin(),
            e = _all_states.end(); i != e; i++) {
        ACS_State* s = *i;

        fprintf(f, "S%d goto:{", s->Get_ID());
        const ACS_Goto_Map& goto_func = s->Get_Goto_Map();

        for (ACS_Goto_Map::const_iterator i = goto_func.begin(), e = goto_func.end();
              i != e; i++) {
            InputTy input = i->first;
            ACS_State* tran = i->second;
            if (isprint(input))
                fprintf(f, "'%c' -> S:%d,", input, tran->Get_ID());
            else
                fprintf(f, "%#x -> S:%d,", input, tran->Get_ID());
        }
        fprintf(f, "} ");

        if (s->_fail_link) {
            fprintf(f, ", fail=S:%d", s->_fail_link->Get_ID());
        }

        if (s->_is_terminal) {
            fprintf(f, ", terminal");
        }

        fprintf(f, "\n");
    }
    fclose(f);
}

void
ACS_Constructor::dump_dot(const char *dotfile) const {
    FILE* f = fopen(dotfile, "w+");
    const char* indent = "  ";

    fprintf(f, "digraph G {\n");

    // Emit node information
    fprintf(f, "%s%d [style=filled];\n", indent, _root->Get_ID());
    for (std::vector<ACS_State*>::const_iterator i = _all_states.begin(),
            e = _all_states.end(); i != e; i++) {
        ACS_State *s = *i;
        if (s->_is_terminal) {
            fprintf(f, "%s%d [shape=doublecircle];\n", indent, s->Get_ID());
        }
    }
    fprintf(f, "\n");

    // Emit edge information
    for (std::vector<ACS_State*>::const_iterator i = _all_states.begin(),
            e = _all_states.end(); i != e; i++) {
        ACS_State* s = *i;
        uint32 id = s->Get_ID();

        const ACS_Goto_Map& m = s->Get_Goto_Map();
        for (ACS_Goto_Map::const_iterator ii = m.begin(), ee = m.end();
             ii != ee; ii++) {
            InputTy input = ii->first;
            ACS_State* tran = ii->second;
            if (isalnum(input))
                fprintf(f, "%s%d -> %d [label=%c];\n",
                        indent, id, tran->Get_ID(), input);
            else
                fprintf(f, "%s%d -> %d [label=\"%#x\"];\n",
                        indent, id, tran->Get_ID(), input);

        }

        // Emit fail-link
        ACS_State* fl = s->Get_FailLink();
        if (fl && fl != _root) {
            fprintf(f, "%s%d -> %d [style=dotted, color=red]; \n",
                    indent, id, fl->Get_ID());
        }
    }
    fprintf(f, "}\n");
    fclose(f);
}
#endif
