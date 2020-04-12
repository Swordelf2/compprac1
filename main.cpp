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
#include <queue>

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
    // `Input` of this block in the dataflow equation
    Defs input;
    // 'Output' of this block in the dataflow equation
    Defs output;
    // Definitions, generated in this block
    Defs gen;
    // Variables, whose definitions are killed in this block
    std::unordered_set<uint> kill;
};


// Insert all defs from `defs2` into `defs1`
static inline void defs_unite(Defs& defs1, const Defs& defs2) {
    for (const Def& def : defs2) {
        defs1.insert(def);
    }
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
                Defs(),
                std::unordered_set<uint>()
            }
        ));
    }

    // Construct defs for each var
    std::vector<Defs> var_defs(fn->ntmp - Tmp0);
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
            if (ins.to.val >= Tmp0) {
                // `ins.to.val` is the def of this instruction
                // Add this definition to the GEN of this block
                blk_info.gen.insert(Def { blk.id, ins.to.val });
                // Add this variable to the KILL of this block
                blk_info.kill.insert(ins.to.val);
            }
        }
    }

    /*
    // DEBUG OUTPUT - print all gens and killcs for each block
    std::cout << "GENs and KILLs" << std::endl;
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
    */

    /* Construct an MFP solution */

    // Add all the block ids to the worklist in the reverse post order
    std::queue<uint> worklist;
    for (uint i = 0; i < fn->nblk; ++i) {
        worklist.push(fn->rpo[i]->id);
    }

    // Pop from the worklist while it's not empty
    while (!worklist.empty()) {
        uint block_id = worklist.front();
        worklist.pop();

        BlkInfo& blk_info = id2blkinfo[block_id];

        // Unite the outputs of preds
        Defs outputs_union;
        for (uint pred_num = 0; pred_num < blk_info.blk->npred; ++pred_num) {
            const Blk& pred = *blk_info.blk->pred[pred_num];
            const BlkInfo& pred_info = id2blkinfo[pred.id];
            defs_unite(outputs_union, pred_info.output);
        }

        blk_info.input = std::move(outputs_union);

        // Update the output by applying the transfer function (subtracting KILL
        // and uniting with GEN) to the input
        Defs new_output = blk_info.input;
        // Subtract KILL
        for (uint var : blk_info.kill) {
            for (const Def& def : var_defs[var - Tmp0]) {
                new_output.erase(def);
            }
        }
        // Unite with GEN
        defs_unite(new_output, blk_info.gen);

        // If the output has changed, push the successors to the worklist
        if (blk_info.output != new_output) {
            blk_info.output = std::move(new_output);
            if (blk_info.blk->s1) {
                worklist.push(blk_info.blk->s1->id);
            }
            if (blk_info.blk->s2) {
                worklist.push(blk_info.blk->s2->id);
            }
        }
    }

    // Print the result - input of each block
    for (uint i = 0; i < fn->nblk; ++i) {
        const Blk& blk = *fn->rpo[i];
        const BlkInfo& blk_info = id2blkinfo[blk.id];
        std::cout << '@' << blk.name << std::endl <<
            "\trd_in = ";
        for (const Def& def : blk_info.input) {
            std::cout << '@' << id2blkinfo[def.blk_id].blk->name << '%' << 
                fn->tmp[def.tmp_idx].name << ' ';
        }
        std::cout << std::endl;
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
