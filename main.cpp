#define export exports
extern "C" {
#include <qbe/all.h>
}
#undef export

#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>

struct Def {
    uint blk_id;
    uint tmp_idx;
    
    bool operator==(const Def& other) const {
        return this->blk_id == other.blk_id && 
                this->tmp_idx == other.tmp_idx;
    }
};

template <class T>
inline static void hash_combine(std::size_t& s, const T& v) {
    std::hash<T> h;
    s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
}

struct DefHash {
    std::size_t operator()(const Def& d) const {
        std::size_t res = 0;
        hash_combine(res, d.blk_id);
        hash_combine(res, d.tmp_idx);
        return res;
    }
};


typedef std::unordered_set<Def, DefHash> Defs;
struct BlkInfo {
    Blk* blk;
    Defs input;
    Defs gen;
    Defs kill;
};


const char *tmp_name(uint id, Fn* fn) {
    return fn->tmp[id].name;
}



static void readfn (Fn *fn) {
    fillpreds(fn);
    fillrpo(fn);
    
    for (uint i = 0; i < fn->nblk; ++i) {
        const Blk &blk = *fn->rpo[i];
    }
        
    std::unordered_map<uint, Blk*> id2blk;
    std::unordered_map<uint, BlkInfo> id2blkinfo;
    for (uint i = 0; i < fn->nblk; ++i) {
        const Blk &blk = *fn->rpo[i];
        id2blk[blk.id] = &blk;
    }

    // Construct defs for each var
    std::vector<Defs> tmp_defs(fn->ntmp - Tmp0);
    for (uint i = 0; i < fn->nblk; ++i) {
        const Blk &blk = *fn->rpo[i];
        for (uint j = 0; j < blk.nins; ++j) {
            const Ins &ins = blk.ins[j];
            tmp_defs[ins.to.val - Tmp0].insert(Def{blk.id, ins.to.val});
        }
    }

    for (int i = 0; i < tmp_defs.size(); ++i) {
        std::cout << "VAR " << tmp_name(i + Tmp0, fn) << ":\n";
        for (const auto& d : tmp_defs[i]) {
            std::cout << "\t" << id2blk[d.blk_id]->name << " " << tmp_name(d.tmp_idx, fn) << std::endl;
        }
    }
}

static void readdat (Dat *dat) {
  (void) dat;
}

char *STDIN_NAME = "<stdin>";

int main () {
  parse(stdin, STDIN_NAME, readdat, readfn);
  freeall();
}
