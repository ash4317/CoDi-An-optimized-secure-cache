#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "mcache.h"
#include "util.h"
// #include "params.h"

#define MCACHE_SRRIP_MAX 7
#define MCACHE_SRRIP_INIT 1
#define MCACHE_PSEL_MAX 1023
#define MCACHE_LEADER_SETS 32

uns debug = 0;

using namespace std;
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

MCache *
mcache_new(uns num_skews, uns sets, uns assocs, uns repl_policy)
{
  MCache *c = (MCache *)calloc(1, sizeof(MCache));
  c->num_skews = num_skews;
  c->sets = sets;
  c->assocs = assocs;
  c->repl_policy = (MCache_ReplPolicy)repl_policy;

  c->skews = (MCache_Skew *)calloc(num_skews, sizeof(MCache_Skew));
  // c->entries  = (MCache_Line *) calloc (sets * assocs, sizeof(MCache_Line));

  for (uns i = 0; i < num_skews; ++i)
  {
    MCache_Skew *s = (MCache_Skew *)calloc(1, sizeof(MCache_Skew));
    s->assocs = assocs;
    s->sets = sets;
    s->key = rand();
    s->entries = (MCache_Line *)calloc(sets * assocs, sizeof(MCache_Line));
    c->skews[i] = *s;
  }
  // c->skews = mcache_skew_new(sets, assocs, rand());

  c->fifo_ptr = (uns *)calloc(sets, sizeof(uns));

  // for drrip or dip
  mcache_select_leader_sets(c, sets);
  c->psel = (MCACHE_PSEL_MAX + 1) / 2;

  return c;
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

void mcache_select_leader_sets(MCache *c, uns sets)
{
  uns done = 0;

  c->is_leader_p0 = (Flag *)calloc(sets, sizeof(Flag));
  c->is_leader_p1 = (Flag *)calloc(sets, sizeof(Flag));

  while (done <= MCACHE_LEADER_SETS)
  {
    uns randval = rand() % sets;
    if ((c->is_leader_p0[randval] == FALSE) && (c->is_leader_p1[randval] == FALSE))
    {
      c->is_leader_p0[randval] = TRUE;
      done++;
    }
  }

  done = 0;
  while (done <= MCACHE_LEADER_SETS)
  {
    uns randval = rand() % sets;
    if ((c->is_leader_p0[randval] == FALSE) && (c->is_leader_p1[randval] == FALSE))
    {
      c->is_leader_p1[randval] = TRUE;
      done++;
    }
  }
}

////////////////////////////////////////////////////////////////////
// the addr field is the lineaddress = address/cache_line_size
////////////////////////////////////////////////////////////////////

Flag mcache_access(MCache *c, Addr addr)
{
  // printf("Count: %llu, Displacement Overflow: %llu\n", c->s_count, c->s_displacement_overflow);
  Addr tag = addr; // full tags
  // uns set = mcache_get_index(c, addr);
  if(debug) printf("Accessing addr: %llu, ", addr);

  c->s_count++;
  // Flag res = MISS;

  // Access all the skews until we get a HIT
  for(uns i = 0; i < c->num_skews; i++) {
    MCache_Skew *skew = &c->skews[i];

    // get lines from set and set + 1
    uns set1 = line_to_set_mapping(addr, skew->key, skew->sets);
    uns start1 = set1 * skew->assocs;
    uns end1 = start1 + skew->assocs;

    uns set2 = (set1 + 1) % c->skews->sets;
    uns start2 = set2 * skew->assocs;
    uns end2 = start2 + skew->assocs;

    for(uns i = start1; i < end1; i++) {
      MCache_Line *entry = &skew->entries[i];
      if(entry->valid && (entry->tag == tag)) {
        // HIT
        entry->last_access = c->s_count;
        entry->ripctr = MCACHE_SRRIP_MAX;
        c->touched_wayid = (i - start1);
        c->touched_setid = set1;
        c->touched_lineid = i;
        if(debug)  printf("HIT\n");
        return HIT;
      }
    }

    for(uns i = start2; i < end2; i++) {
      MCache_Line *entry = &skew->entries[i];
      if(entry->valid && (entry->tag == tag)) {
        // HIT
        entry->last_access = c->s_count;
        entry->ripctr = MCACHE_SRRIP_MAX;
        c->touched_wayid = (i - start2);
        c->touched_setid = set2;
        c->touched_lineid = i;
        if(debug)  printf("HIT\n");
        return HIT;
      }
    }
  }

  // even on a miss, we need to know which set was accessed
  c->touched_wayid = 0;
  // c->touched_setid = set;
  // c->touched_lineid = start;

  c->s_miss++;
  if(debug) printf("MISS\n");
  return MISS;

  // uns start = set * c->assocs;
  // uns end = start + c->assocs;
  // uns ii;


  // for (ii = start; ii < end; ii++)
  // {
  //   MCache_Line *entry = &c->entries[ii];

  //   if (entry->valid && (entry->tag == tag))
  //   {
  //     entry->last_access = c->s_count;
  //     entry->ripctr = MCACHE_SRRIP_MAX;
  //     c->touched_wayid = (ii - start);
  //     c->touched_setid = set;
  //     c->touched_lineid = ii;
  //     return HIT;
  //   }
  // }

  // // even on a miss, we need to know which set was accessed
  // c->touched_wayid = 0;
  // c->touched_setid = set;
  // c->touched_lineid = start;

  // c->s_miss++;
  // return MISS;
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

Flag mcache_probe(MCache *c, Addr addr)
{
  Addr tag = addr; // full tags

  // Flag res = FALSE;

  for(uns i = 0; i < c->num_skews; i++) {
    MCache_Skew *skew = &c->skews[i];
    
    // get lines from set and set + 1
    uns set1 = line_to_set_mapping(addr, skew->key, skew->sets);
    uns start1 = set1 * skew->assocs;
    uns end1 = start1 + skew->assocs;

    uns set2 = (set1 + 1) % c->skews->sets;
    uns start2 = set2 * skew->assocs;
    uns end2 = start2 + skew->assocs;

    for(uns i = start1; i < end1; i++) {
      MCache_Line *entry = &skew->entries[i];
      if(entry->valid && (entry->tag == tag)) {
        return TRUE;
      }
    }

    for(uns i = start2; i < end2; i++) {
      MCache_Line *entry = &skew->entries[i];
      if(entry->valid && (entry->tag == tag)) {
        return TRUE;
      }
    }
  }
  return FALSE;
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

Flag mcache_invalidate(MCache *c, Addr addr)
{
  Addr tag = addr; // full tags
  for(uns i = 0; i < c->num_skews; i++) {
    MCache_Skew *skew = &c->skews[i];

    // get lines from set and set + 1
    uns set1 = line_to_set_mapping(addr, skew->key, skew->sets);
    uns start1 = set1 * skew->assocs;
    uns end1 = start1 + skew->assocs;

    uns set2 = (set1 + 1) % c->skews->sets;
    uns start2 = set2 * skew->assocs;
    uns end2 = start2 + skew->assocs;

    for (uns ii = start1; ii < end1; ii++)
    {
      MCache_Line *entry = &c->skews[i].entries[ii];
      if (entry->valid && (entry->tag == tag))
      {
        entry->valid = FALSE;
        return TRUE;
      }
    }

    for (uns ii = start2; ii < end2; ii++)
    {
      MCache_Line *entry = &c->skews[i].entries[ii];
      if (entry->valid && (entry->tag == tag))
      {
        entry->valid = FALSE;
        return TRUE;
      }
    }
    
  }

  return FALSE;



  // uns set = mcache_get_index(c, addr);
  // uns start = set * c->assocs;
  // uns end = start + c->assocs;
  // uns ii;

  // for (ii = start; ii < end; ii++)
  // {
  //   MCache_Line *entry = &c->entries[ii];
  //   if (entry->valid && (entry->tag == tag))
  //   {
  //     entry->valid = FALSE;
  //     return TRUE;
  //   }
  // }

  // return FALSE;
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

Flag mcache_mark_dirty(MCache *c, Addr addr)
{
  Addr tag = addr; // full tags
  for(uns i = 0; i < c->num_skews; i++) {
    MCache_Skew *skew = &c->skews[i];
    
    // get lines from set and set + 1
    uns set1 = line_to_set_mapping(addr, skew->key, skew->sets);
    uns start1 = set1 * skew->assocs;
    uns end1 = start1 + skew->assocs;

    uns set2 = (set1 + 1) % c->skews->sets;
    uns start2 = set2 * skew->assocs;
    uns end2 = start2 + skew->assocs;

    for (uns ii = start1; ii < end1; ii++)
    {
      MCache_Line *entry = &c->skews[i].entries[ii];
      if (entry->valid && (entry->tag == tag))
      {
        entry->dirty = TRUE;
        return TRUE;
      }
    }

    for (uns ii = start2; ii < end2; ii++)
    {
      MCache_Line *entry = &c->skews[i].entries[ii];
      if (entry->valid && (entry->tag == tag))
      {
        entry->dirty = TRUE;
        return TRUE;
      }
    }
    
  }

  return FALSE;

  // Addr tag = addr; // full tags
  // uns set = mcache_get_index(c, addr);
  // uns start = set * c->assocs;
  // uns end = start + c->assocs;
  // uns ii;

  // for (ii = start; ii < end; ii++)
  // {
  //   MCache_Line *entry = &c->entries[ii];
  //   if (entry->valid && (entry->tag == tag))
  //   {
  //     entry->dirty = TRUE;
  //     return TRUE;
  //   }
  // }

  // return FALSE;
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

void mcache_install(MCache *c, Addr addr)
{
  Addr tag = addr; // full tags
  // uns sets[NUM_SKEWS];
  MCache_Line *entry = NULL;
  Flag update_lrubits = TRUE;
  
  uns ii = 0;

  for(uns i = 0; i < c->num_skews; i++) {
    uns set1 = line_to_set_mapping(addr, c->skews[i].key, c->skews[i].sets);
    uns start1 = set1 * c->skews[i].assocs;
    uns end1 = start1 + c->skews[i].assocs;

    uns set2 = set1 + 1;
    uns start2 = set2 * c->skews[i].assocs;
    uns end2 = start2 + c->skews[i].assocs;



    for(ii = start1; ii < end1; ii++) {
      entry = &c->skews[i].entries[ii];
      if(!entry->valid) {
      if(debug) printf("Installing in Skew: %u, Index: %u\n", i, ii);
        break;
      }
      if (entry->valid && (entry->tag == tag))
      {
        if(debug) printf("Installed entry already with addr:%llx present in set:%u\n", addr, set1);
        exit(-1);
      }
    }
    if(!entry->valid) break;

    for(ii = start2; ii < end2; ii++) {
      entry = &c->skews[i].entries[ii];
      if(!entry->valid) {
      if(debug) printf("Installing in Skew: %u, Index: %u\n", i, ii);
        break;
      }
      if (entry->valid && (entry->tag == tag))
      {
        if(debug) printf("Installed entry already with addr:%llx present in set:%u\n", addr, set2);
        exit(-1);
      }
    }
  }
  // victim = mcache_find_victim(c, set);
  if(entry->valid)
    entry = mcache_find_victim_skew(c, addr);
  // entry = &c->skews[i].entries[victim];

  if (entry->valid)
  {
    c->s_evict++;
  }
  uns ripctr_val = MCACHE_SRRIP_INIT;

  // if (c->repl_policy == REPL_DRRIP)
  // {
  //   ripctr_val = mcache_drrip_get_ripctrval(c, set);
  // }

  // if (c->repl_policy == REPL_DIP)
  // {
  //   update_lrubits = mcache_dip_check_lru_update(c, set);
  // }

  // put new information in
  entry->tag = tag;
  entry->valid = TRUE;
  entry->dirty = FALSE;
  entry->ripctr = ripctr_val;
  entry->nID = ii / c->assocs;

  if (update_lrubits)
  {
    entry->last_access = c->s_count;
  }

  // c->fifo_ptr[set] = (c->fifo_ptr[set] + 1) % c->assocs; // fifo update

  // c->touched_lineid = victim;
  // c->touched_setid = set;
  // c->touched_wayid = victim - (set * c->assocs);

}






  // uns set = mcache_get_index(c, addr);
  // uns start = set * c->assocs;
  // uns end = start + c->assocs;
  // uns ii, victim;

  // Flag update_lrubits = TRUE;

  // MCache_Line *entry;

  // for (ii = start; ii < end; ii++)
  // {
  //   entry = &c->entries[ii];
  //   if (entry->valid && (entry->tag == tag))
  //   {
  //     printf("Installed entry already with addr:%llx present in set:%u\n", addr, set);
  //     exit(-1);
  //   }
  // }

  // // find victim and install entry
  // victim = mcache_find_victim(c, set);
  // entry = &c->entries[victim];

  // if (entry->valid)
  // {
  //   c->s_evict++;
  // }

  // // udpate DRRIP info and select value of ripctr
  // uns ripctr_val = MCACHE_SRRIP_INIT;

  
// }

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
Flag mcache_dip_check_lru_update(MCache *c, uns set)
{
  Flag update_lru = TRUE;

  if (c->is_leader_p0[set])
  {
    if (c->psel < MCACHE_PSEL_MAX)
    {
      c->psel++;
    }
    update_lru = FALSE;
    if (rand() % 100 < 5)
      update_lru = TRUE; // BIP
  }

  if (c->is_leader_p1[set])
  {
    if (c->psel)
    {
      c->psel--;
    }
    update_lru = 1;
  }

  if ((c->is_leader_p0[set] == FALSE) && (c->is_leader_p1[set] == FALSE))
  {
    if (c->psel >= (MCACHE_PSEL_MAX + 1) / 2)
    {
      update_lru = 1; // policy 1 wins
    }
    else
    {
      update_lru = FALSE; // policy 0 wins
      if (rand() % 100 < 5)
        update_lru = TRUE; // BIP
    }
  }

  return update_lru;
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
uns mcache_drrip_get_ripctrval(MCache *c, uns set)
{
  uns ripctr_val = MCACHE_SRRIP_INIT;

  if (c->is_leader_p0[set])
  {
    if (c->psel < MCACHE_PSEL_MAX)
    {
      c->psel++;
    }
    ripctr_val = 0;
    if (rand() % 100 < 5)
      ripctr_val = 1; // BIP
  }

  if (c->is_leader_p1[set])
  {
    if (c->psel)
    {
      c->psel--;
    }
    ripctr_val = 1;
  }

  if ((c->is_leader_p0[set] == FALSE) && (c->is_leader_p1[set] == FALSE))
  {
    if (c->psel >= (MCACHE_PSEL_MAX + 1) / 2)
    {
      ripctr_val = 1; // policy 1 wins
    }
    else
    {
      ripctr_val = 0; // policy 0 wins
      if (rand() % 100 < 5)
        ripctr_val = 1; // BIP
    }
  }

  return ripctr_val;
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

MCache_Line* mcache_find_victim_skew(MCache *c, Addr addr) {
  // get random sets from all the skews and find lru out of them
  MCache_Line *victim = NULL;
  uns set_nums[8];
  if(debug) printf("Random sets: ");
  for(uns i = 0; i < c->num_skews; i = i + 2) {
    set_nums[i] = rand() % c->skews[i].sets;    // get line to set mapping
    set_nums[i + 1] = set_nums[i] + 1;          // get next set number
    if(debug) printf("%u, %u, ", set_nums[i], set_nums[i + 1]);
  }
  if(debug) printf("\n");

  uns skew_num = 0, index = 0;
  for(uns i = 0; i < c->num_skews; i++) {
    MCache_Skew *skew = &c->skews[i];
    uns start = set_nums[i] * skew->assocs;
    uns end = start + skew->assocs;

    for(uns j = start; j < end; j++) {
      if(skew->entries[j].valid == false) {
        return &skew->entries[j];
      }
      if(victim == NULL) {
        victim = &skew->entries[j];
      }
      else {
        if(skew->entries[j].last_access < victim->last_access) {
          victim = &skew->entries[j];
          skew_num = i;
          index = j;
        }
      }
    }
  }

  // find displacement path for the eviction set
  get_line_displacement_graph(c, addr, skew_num, index);

  if(debug) printf("Victim skew: %u, index: %u\n", skew_num, index);
  return victim;
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

void get_line_displacement_graph(MCache *c, Addr addr, uns victim_skew, uns victim_index) {

  // incoming line and pathnode
  PathNode *incoming_node = (PathNode *)calloc(1, sizeof(PathNode));
  MCache_Line *incoming_line = (MCache_Line *)calloc(1, sizeof(MCache_Line));
  incoming_line->tag = addr;
  incoming_line->dirty = false;
  incoming_line->valid = true;
  incoming_line->last_access = c->s_count;
  incoming_node->line = incoming_line;
  incoming_node->parent = NULL;   // incoming line parent = NULL

  // add accessed lines to frontier
  stack<PathNode*> frontier;
  for(uns i = 0; i < c->num_skews; i++) {
    int set1 = line_to_set_mapping(addr, c->skews[i].key, c->skews->sets);
    uns start1 = set1 * c->assocs;
    uns end1 = start1 + c->assocs;
    uns set2 = (set1 + 1) % c->skews->sets;
    uns start2 = set2 * c->assocs;
    uns end2 = start2 + c->assocs;

    MCache_Skew *skew = &c->skews[i];

    for(uns j = start1; j < end1; j++) {
      PathNode *node = (PathNode *)calloc(1, sizeof(PathNode));
      MCache_Line *line = &skew->entries[j];
      node->line = line;
      node->parent = incoming_node; // parent = incoming node
      node->skew_num = i;
      node->set_num = j / c->assocs;
      frontier.push(node);
    }

    for(uns j = start2; j < end2; j++) {
      PathNode *node = (PathNode *)calloc(1, sizeof(PathNode));
      MCache_Line *line = &skew->entries[j];
      node->line = line;
      node->parent = incoming_node; // parent = incoming node
      node->skew_num = i;
      node->set_num = j / c->assocs;
      frontier.push(node);
    }
  }

  uns victim_set = victim_index / c->assocs;
  unordered_set<uns> vicinity = {(victim_set - 1) % c->skews->sets, (victim_set + 1) % c->skews->sets};

  unordered_set<uns64> visited_lines;
  const int MAX_DISPLACEMENTS = 16;
  uns num_displacements = 0;

  // victim node
  PathNode *victim_node = (PathNode *)calloc(1, sizeof(PathNode));
  victim_node->line = &c->skews[victim_skew].entries[victim_index];
  victim_node->skew_num = victim_skew;
  victim_node->set_num = victim_set;

  bool found_path = false;
  while(!frontier.empty() && !found_path) {
    // pop from frontier
    PathNode *current_node = frontier.top();
    MCache_Line *current_line = current_node->line;
    frontier.pop();

    // TODO: IF DISPLACEMENTS > MAX DISPLACEMENTS, COMPLETE THE PATH AND BREAK
    if(num_displacements >= MAX_DISPLACEMENTS) {
      victim_node->parent = current_node;
      found_path = true;
      c->s_displacement_overflow++;
      break;
    }

    /**
     * TODO: 1) GET SET NUMBER OF THIS LINE IN THE EVICTION SKEW.
     * 2) IF SET NUMBER IS IN VICINITY, PERFORM VERTICAL MOVES.
     *    I) PERFORM VERTICAL MOVES: GET LINES FROM SET NUMBER/nID (LINES IN SET_NUM AND SET_NUM + 1) AND SEE IF ANY LINE CAN BE MAPPED TO THE VICTIM SET. IF YES, GRAPH = THIS_LINE => LINE THAT CAN BE MAPPED => VICTIM
     *    II) IF NONE CAN BE MAPPED, CONTINUE
     * 3) IF SET NUMBER IF NOT IN VICINITY, GET A RANDOM SKEW AND ADD THE MAPPED LINES TO THE FRONTIER.
    */
    if(visited_lines.find(current_line->tag) == visited_lines.end()) {
      num_displacements++;
      visited_lines.insert(current_line->tag);  // add tag to visited lines

      // get set number of current line in eviction set
      int set_in_eviction = line_to_set_mapping(current_line->tag, c->skews[victim_skew].key, c->skews->sets);

      if(
        vicinity.find(set_in_eviction) != vicinity.end()
        or
        vicinity.find((set_in_eviction + 1) % c->skews->sets) != vicinity.end()
      ) {
        // TODO: PERFORM VERTICAL MOVES
        
        MCache_Line *mapped_line = perform_vertical_moves(c, victim_node->line->nID, victim_skew, set_in_eviction);

        if(mapped_line != NULL) {
          // TODO: FOUND A MAPPED LINE. COMPLETE THE GRAPH
          PathNode *node = (PathNode *)calloc(1, sizeof(PathNode));
          node->line = mapped_line;
          node->parent = current_node;
          victim_node->parent = node;
          found_path = true;
          displace_lines(c, victim_node);
          break;
        }
      }
      else {
        // get a random skew, add mapped lines to frontier
        uns rand_skew = random_skew(c->num_skews);
        uns set1 = line_to_set_mapping(current_line->tag, c->skews[victim_skew].key, c->skews->sets);
        uns start1 = set1 * c->assocs;
        uns end1 = start1 + c->assocs;
        uns set2 = (set1 + 1) % c->skews->sets;
        uns start2 = set2 * c->assocs;
        uns end2 = start2 + c->assocs;

        for(uns i = start1; i < end1; i++) {
          PathNode *node = (PathNode *)calloc(1, sizeof(PathNode));
          node->parent = current_node;
          node->line = &c->skews[rand_skew].entries[i];
          frontier.push(node);
        }

        for(uns i = start2; i < end2; i++) {
          PathNode *node = (PathNode *)calloc(1, sizeof(PathNode));
          node->parent = current_node;
          node->line = &c->skews[rand_skew].entries[i];
          frontier.push(node);
        }
      }
    }
  }

  // TODO: FREE ALL PATHNODES
  while(!frontier.empty()) {
    PathNode *temp = frontier.top();
    frontier.pop();
    free(temp);
  }
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

void displace_lines(MCache *c, PathNode *victim_node) {
  // TODO: SWAP ALL THE LINES IN THE LINE DISPLACEMENT GRAPH, UPDATE NIDS

  PathNode *current_node = victim_node;
  PathNode *parent_node = current_node->parent;
  // MCache_Line *current_line = victim_node->line;
  // MCache_Line *parent_line = parent_node->line;

  /*
    10 -> 11 -> 12 -> 13 -> 14
    current_line = current_node->line
    parent_line = parent_node->line
    current_line = parent_line;
    current_line = 13
    
  */

  while(parent_node != NULL) {
    MCache_Line *current_line = current_node->line;
    MCache_Line *parent_line = parent_node->line;

    // TODO: COPY PARENT LINE CONTENTS INTO CURRENT LINE CONTENTS
    current_line->dirty = parent_line->dirty;
    current_line->last_access = parent_line->last_access;
    current_line->orig_lineaddr = parent_line->orig_lineaddr;
    current_line->ripctr = parent_line->ripctr;
    current_line->tag = parent_line->tag;
    current_line->valid = parent_line->valid;

    // TODO: UPDATE CURRENT AND PARENT NODE
    current_node = parent_node;
    parent_node = parent_node->parent;
  }
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

MCache_Line* perform_vertical_moves(MCache *c, uns victim_nID, uns victim_skew, uns mapped_set) {
  // TODO: GET LINES FROM MAPPED SET, MAPPED SET + 1. SEE IF ANY OF THESE LINES CAN BE SHIFTED TO THE VICTIM SET (BY CHECKING IF THEIR NID = NID OF VICTIM)

  MCache_Skew *skew = &c->skews[victim_skew];

  uns start1 = mapped_set * c->assocs;
  uns end1 = start1 + c->assocs;

  uns start2 = (((mapped_set + 1) * c->assocs) % c->skews->sets) * c->assocs;
  uns end2 = start2 + c->assocs;

  for(uns i = start1; i < end1; i++) {
    if(skew->entries[i].nID == victim_nID) {
      return &skew->entries[i];
    }
  }

  for(uns i = start2; i < end2; i++) {
    if(skew->entries[i].nID == victim_nID) {
      return &skew->entries[i];
    }
  }

  return NULL;
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

uns mcache_find_victim(MCache *c, uns set)
{
  int ii;
  int start1 = set * c->assocs;
  int end1 = start1 + c->assocs;

  int start2 = ((set + 1) % c->skews->sets) * c->assocs;
  int end2 = start2 + c->assocs;

  // search for invalid first
  for (ii = start1; ii < end1; ii++)
  {
    if (!c->entries[ii].valid)
    {
      return ii;
    }
  }

  // search for invalid first
  for (ii = start2; ii < end2; ii++)
  {
    if (!c->entries[ii].valid)
    {
      return ii;
    }
  }

  switch (c->repl_policy)
  {
  case REPL_LRU:
    return mcache_find_victim_lru(c, set);
  case REPL_RND:
    return mcache_find_victim_rnd(c, set);
  case REPL_SRRIP:
    return mcache_find_victim_srrip(c, set);
  case REPL_DRRIP:
    return mcache_find_victim_srrip(c, set);
  case REPL_DIP:
    return mcache_find_victim_lru(c, set);
  default:
    assert(0);
  }

  return -1;
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

uns mcache_find_victim_lru(MCache *c, uns set)
{
  uns start = set * c->assocs;
  uns end = start + c->assocs;
  uns lowest = start;
  uns ii;

  for (ii = start; ii < end; ii++)
  {
    if (c->entries[ii].last_access < c->entries[lowest].last_access)
    {
      lowest = ii;
    }
  }

  return lowest;
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

uns mcache_find_victim_rnd(MCache *c, uns set)
{
  uns start = set * c->assocs;
  uns victim = start + rand() % c->assocs;

  return victim;
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

uns mcache_find_victim_srrip(MCache *c, uns set)
{
  uns start = set * c->assocs;
  uns end = start + c->assocs;
  uns ii;
  uns victim = end; // init to impossible

  while (victim == end)
  {
    for (ii = start; ii < end; ii++)
    {
      if (c->entries[ii].ripctr == 0)
      {
        victim = ii;
        break;
      }
    }

    if (victim == end)
    {
      for (ii = start; ii < end; ii++)
      {
        c->entries[ii].ripctr--;
      }
    }
  }

  return victim;
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

uns mcache_get_index(MCache *c, Addr addr)
{
  uns retval;

  switch (c->index_policy)
  {
  case 0:
    retval = addr % c->sets;
    break;

  default:
    exit(-1);
  }

  return retval;
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

void mcache_print_stats(MCache *c, char *header)
{
  double missrate = 100.0 * (double)c->s_miss / (double)c->s_count;

  printf("\n%s_ACCESS                 \t : %llu", header, c->s_count);
  printf("\n%s_MISS                   \t : %llu", header, c->s_miss);
  printf("\n%s_MISSRATE               \t : %6.3f", header, missrate);
  printf("\n%s_SAE                    \t : %llu", header, c->s_sae);
  printf("\n%s_DISPLACEMENT_OVERFLOW  \t : %llu", header, c->s_displacement_overflow);
  printf("\n");
}