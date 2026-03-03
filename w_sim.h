/* w_sim.h - Force-include for s_sim.c compilation only
 * Renames conflicting functions that WinTown reimplements
 */
#define _W_SIM_H

#define SimFrame _orig_SimFrame
#define Simulate _orig_Simulate
#define DoSimInit _orig_DoSimInit
#define MapScan _orig_MapScan
#define GetBoatDis _orig_GetBoatDis
#define DoMeltdown _orig_DoMeltdown
#define Rand _orig_Rand
#define Rand16 _orig_Rand16
#define Rand16Signed _orig_Rand16Signed
#define RandomlySeedRand _orig_RandomlySeedRand
#define SeedRand _orig_SeedRand

#define sim_rand rand
#define sim_srand srand

#define doAllGraphs()

static short NewMapFlags[16];
static short NewMap;
static short NewGraph;
