#ifndef OS_H
#define OS_H

#include "global_types.h"
#include "hash_lib.h"

#define OS_MAX_THREADS 16


typedef struct OS                OS;
typedef struct InvPageTableEntry InvPageTableEntry;
typedef struct PageTableEntry    PageTableEntry;
typedef struct PageTable         PageTable;
typedef struct InvPageTable      InvPageTable;
typedef struct VirtualPhysicalPair VirtualPhysicalPair;


struct PageTableEntry{
    uns  pfn;
};


struct VirtualPhysicalPair{
  uns vpn;
  uns pfn;
};


struct PageTable{
    Hash_Table  *entries;
    VirtualPhysicalPair last_xlation[OS_MAX_THREADS];
    uns64        max_entries;
    uns64        miss_count;
    uns64        total_evicts;
    uns64        evicted_dirty_page;
};

struct InvPageTableEntry{
    Flag valid;
    Flag dirty;
    Flag ref;
    uns  vpn;
};

struct InvPageTable{
  InvPageTableEntry  *entries;
  uns          num_entries;
  uns          refptr;
};


//////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


struct OS {
    PageTable        *pt;
    InvPageTable     *ipt;

    uns               lines_in_page;
    uns               num_threads;
    uns64             num_pages;
};



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

OS*     os_new(uns num_pages, uns num_threads);
uns     os_vpn_to_pfn(OS *os, uns vpn, uns tid, Flag *hit);
void    os_print_stats(OS *os, FILE *fptr=stdout);

uns     os_get_victim_from_ipt(OS *os);
Addr    os_v2p_lineaddr(OS *os, Addr lineaddr, uns tid);

//////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

#endif // OS_H
