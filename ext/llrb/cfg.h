/*
 * cfg.h: Has a Control Flow Graph struct definition shared by parser.c and compiler.c.
 */

#ifndef LLRB_CFG_H
#define LLRB_CFG_H

#include <stdbool.h>

struct llrb_basic_block {
  unsigned int start;            // Start index of ISeq body's iseq_encoded.
  unsigned int end;              // End index of ISeq body's iseq_encoded.
  unsigned int incoming_size;    // Size of incoming_starts.
  unsigned int *incoming_starts; // Start indices of incoming basic blocks.
  bool traversed; // Used by `llrb_set_incoming_blocks_by` to prevent infinite recursion by circular reference.
};

// Holds Control-Flow-Graph-like data structure. Actually it's a buffer of graph nodes.
struct llrb_cfg {
  struct llrb_basic_block* blocks;
  unsigned int size;
};

#endif // LLRB_CFG_H
