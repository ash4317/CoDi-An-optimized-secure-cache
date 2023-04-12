#ifndef MCACHE_H
#define MCACHE_H

#include "global_types.h"
#include <stack>
#include <vector>
#include <unordered_set>


typedef enum MCache_ReplPolicy_Enum {
    REPL_LRU=0,
    REPL_RND=1,
    REPL_SRRIP=2, 
    REPL_DRRIP=3, 
    REPL_FIFO=4, 
    REPL_DIP=5, 
    NUM_REPL_POLICY=6
} MCache_ReplPolicy;


typedef struct MCache_Line MCache_Line;
typedef struct MCache MCache;
typedef struct MCache_Skew MCache_Skew;
typedef struct PathNode PathNode;

///////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

struct MCache_Line {
    Flag    valid;            // valid: Line has data, Invalid: Line is empty
    Flag    dirty;            // whether line is dirty
    Addr    tag;              // tag
    uns     ripctr;
    uns64   last_access;      // timestamp
    Addr    orig_lineaddr;
    uns     nID;              // neighbourhood ID
};

/**
 * MCache_Skew: A skew is a set-associative cache
*/
struct MCache_Skew {
  uns sets;                   // number of sets
  uns assocs;                 // associativity
  uns64 key;                  // cryptographic key for this skew
  MCache_Line *entries;       // cache lines
};


struct MCache{
  uns sets;
  uns assocs;
  MCache_ReplPolicy repl_policy; //0:LRU  1:RND 2:SRRIP
  uns index_policy; // how to index cache
  uns num_skews;

  Flag *is_leader_p0; // leader SET for D(RR)IP
  Flag *is_leader_p1; // leader SET for D(RR)IP
  uns psel;

  MCache_Line *entries;
  MCache_Skew *skews;
  uns *fifo_ptr; // for fifo replacement (per set)
  int touched_wayid;
  int touched_setid;
  int touched_lineid;

  uns64 s_count; // number of accesses
  uns64 s_miss; // number of misses
  uns64 s_evict; // number of evictions
  uns64 s_sae;
  uns64 s_displacement_overflow;
};

struct PathNode {
  MCache_Line *line;
  struct PathNode *parent;
  uns skew_num;
  uns set_num;
};

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

MCache *mcache_new(uns skews, uns sets, uns assocs, uns repl);
// MCache_Skew *mcache_skew_new(uns sets, uns assocs, uns64 key);
Flag    mcache_access               (MCache *c, Addr addr);
void    mcache_install              (MCache *c, Addr addr);
Flag    mcache_probe                (MCache *c, Addr addr);
Flag    mcache_invalidate           (MCache *c, Addr addr);
Flag    mcache_mark_dirty           (MCache *c, Addr addr);
uns     mcache_get_index            (MCache *c, Addr addr);

uns     mcache_find_victim          (MCache *c, uns set);
MCache_Line* mcache_find_victim_skew(MCache *c, Addr addr);
void get_line_displacement_graph    (MCache *c, Addr addr, uns victim_skew, uns victim_index);
uns     mcache_find_victim_lru      (MCache *c, uns set);
uns     mcache_find_victim_rnd      (MCache *c, uns set);
void    displace_lines              (MCache *c, PathNode *victim_node);
uns     mcache_find_victim_srrip    (MCache *c, uns set);
MCache_Line* perform_vertical_moves (MCache *c, uns victim_nID, uns victim_skew, uns mapped_set);
void    mcache_select_leader_sets   (MCache *c,uns sets);
uns     mcache_drrip_get_ripctrval  (MCache *c, uns set);
Flag    mcache_dip_check_lru_update (MCache *c, uns set);

void    mcache_print_stats          (MCache *c, char *header);
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

#endif // MCACHE_H
