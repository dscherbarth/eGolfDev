//
// snap.h
//

#define MAXSNAP	2000

void add_snap_rec (int ind, int rec, int val);

int fault_snap_get(void);
void fault_snap_set(void);
void fault_snap_clear(void);
void snap_get_start (void);
