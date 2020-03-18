#define export exports
extern "C" {
#include <qbe/all.h>
}
#undef export

#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <utility>

// Definition of a variable
struct Def {
    // Id of a block
    uint blk_id;

    // Variable index (in the Fn::tmp array)
    uint tmp_idx;
    
    bool operator==(const Def& other) const {
        return this->blk_id == other.blk_id && 
                this->tmp_idx == other.tmp_idx;
    }
};

// Auxillary function for combining hashes of two objects into one
template <class T>
inline static void hash_combine(std::size_t& s, const T& v) {
    std::hash<T> h;
    s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
}

// Hash function for `Def`
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
    // Pointer to the block
    Blk* blk;
    // `Input` of the dataflow equation
    Defs input;
    // Definitions, generated in this block
    Defs gen;
    // Variables, whose definitions are killed in this block
    std::unordered_set<uint> kill;
};


const char *tmp_name(uint id, Fn* fn) {
    return fn->tmp[id].name;
}



static void readfn (Fn *fn) {
    // Fill the `pred` field for each block
    fillpreds(fn);
    // Construct the reverse post order array of pointers to blocks
    fillrpo(fn);
        
    // A mapping from block id to its `BlkInfo` struct
    std::unordered_map<uint, BlkInfo> id2blkinfo;
    for (uint i = 0; i < fn->nblk; ++i) {
        Blk &blk = *fn->rpo[i];
        id2blkinfo.insert(std::make_pair(
            blk.id,
            BlkInfo {
                &blk,
                Defs(),
                Defs(),
                std::unordered_set<uint>()
            }
        ));
    }

    // Construct defs for each var
    std::vector<Defs> var_defs(fn->ntmp - Tmp0);
    std::cout << fn->ntmp << ' ' << Tmp0 << std::endl;
    for (uint i = 0; i < fn->nblk; ++i) {
        const Blk &blk = *fn->rpo[i];
        for (uint ins_num = 0; ins_num < blk.nins; ++ins_num) {
            const Ins &ins = blk.ins[ins_num];
            // Only process this instruction if it has a def
            if (ins.to.val >= Tmp0) {
                var_defs[ins.to.val - Tmp0].insert(Def{blk.id, ins.to.val});
            }
        }
    }

    // Construct GEN and KILL for each block
    for (uint i = 0; i < fn->nblk; ++i) {
        const Blk &blk = *fn->rpo[i];
        BlkInfo &blk_info = id2blkinfo[blk.id];
        for (uint ins_num = 0; ins_num < blk.nins; ++ins_num) {
            const Ins &ins = blk.ins[ins_num];
            // `ins.to.val` is the def of this instruction
            // Add this definition to the GEN of this block
            blk_info.gen.insert(Def { blk.id, ins.to.val });
            // Add this variable to the KILL of this block
            blk_info.kill.insert(ins.to.val);
        }
    }

    // DEBUG OUTPUT - print all gens and killcs for each block
    for (uint i = 0; i < fn->nblk; ++i) {
        const Blk &blk = *fn->rpo[i];
        const BlkInfo &blk_info = id2blkinfo[blk.id];
        std::cout << blk.name << std::endl <<
            "\tGEN" << std::endl;
        for (const Def &def : blk_info.gen) {
            std::cout << "\t\t" << id2blkinfo[def.blk_id].blk->name << '%' <<
                fn->tmp[def.tmp_idx].name << std::endl;
        }
        std::cout << "\tKILL" << std::endl;
        for (uint var : blk_info.kill) {
            std::cout << "\t\t" << fn->tmp[var].name << std::endl;
        }
    }
}

static void readdat (Dat *dat) {
  (void) dat;
}

char STDIN_NAME[] = "<stdin>";

int main () {
  parse(stdin, STDIN_NAME, readdat, readfn);
  freeall();
}
