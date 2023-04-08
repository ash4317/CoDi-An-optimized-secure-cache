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
  Addr tag = addr; // full tags
  // uns set = mcache_get_index(c, addr);
  if(debug)
    printf("Accessing addr: %llu, ", addr);

  c->s_count++;
  // Flag res = MISS;

  // Access all the skews until we get a HIT
  for(uns i = 0; i < c->num_skews; i++) {
    MCache_Skew *skew = &c->skews[i];
    uns set = line_to_set_mapping(addr, skew->key, skew->sets);
    uns start = set * skew->assocs;
    uns end = start + skew->assocs;

    for(uns i = start; i < end; i++) {
      MCache_Line *entry = &skew->entries[i];
      if(entry->valid && (entry->tag == tag)) {
        // HIT
        entry->last_access = c->s_count;
        entry->ripctr = MCACHE_SRRIP_MAX;
        c->touched_wayid = (i - start);
        c->touched_setid = set;
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
    uns set = line_to_set_mapping(addr, skew->key, skew->sets);
    uns start = set * skew->assocs;
    uns end = start + skew->assocs;

    for(uns i = start; i < end; i++) {
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
    uns set = line_to_set_mapping(addr, c->skews[i].key, c->skews[i].sets);
    uns start = set * c->skews[i].assocs;
    uns end = start + c->skews[i].assocs;

    for (uns ii = start; ii < end; ii++)
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
    uns set = line_to_set_mapping(addr, c->skews[i].key, c->skews[i].sets);
    uns start = set * c->skews[i].assocs;
    uns end = start + c->skews[i].assocs;

    for (uns ii = start; ii < end; ii++)
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
  MCache_Line *entry = (MCache_Line *)calloc(1, sizeof(MCache_Line));
  Flag update_lrubits = TRUE;

  for(uns i = 0; i < c->num_skews; i++) {
    uns set = line_to_set_mapping(addr, c->skews[i].key, c->skews[i].sets);
    uns start = set * c->skews[i].assocs;
    uns end = start + c->skews[i].assocs;

    uns ii;
    // uns victim;


    for(ii = start; ii < end; ii++) {
      entry = &c->skews[i].entries[ii];
      if(!entry->valid) {
      if(debug) printf("Installing in Skew: %u, Index: %u\n", i, ii);
        break;
      }
      if (entry->valid && (entry->tag == tag))
      {
        if(debug) printf("Installed entry already with addr:%llx present in set:%u\n", addr, set);
        exit(-1);
      }
    }
    if(!entry->valid) break;
  }
  // victim = mcache_find_victim(c, set);
  if(entry->valid)
    entry = mcache_find_victim_skew(c);
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

MCache_Line* mcache_find_victim_skew(MCache *c) {
  // get random sets from all the skews and find lru out of them
  MCache_Line *victim = (MCache_Line *)calloc(1, sizeof(MCache_Line));
  victim->valid = false;
  uns set_nums[4];
  if(debug) printf("Random sets: ");
  for(uns i = 0; i < c->num_skews; i++) {
    set_nums[i] = rand() % c->skews[i].sets;
    if(debug) printf("%u, ", set_nums[i]);
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
      if(victim->valid == false) {
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
  if(debug) printf("Victim skew: %u, index: %u\n", skew_num, index);
  return victim;
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

uns mcache_find_victim(MCache *c, uns set)
{
  int ii;
  int start = set * c->assocs;
  int end = start + c->assocs;

  // search for invalid first
  for (ii = start; ii < end; ii++)
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

  printf("\n%s_ACCESS       \t : %llu", header, c->s_count);
  printf("\n%s_MISS         \t : %llu", header, c->s_miss);
  printf("\n%s_MISSRATE     \t : %6.3f", header, missrate);
  printf("\n");
}
