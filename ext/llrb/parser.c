/*
 * parser.c: Constructs Control Flow Graph from encoded YARV instructions in ISeq.
 */

#include "cruby.h"

struct llrb_parser {
  const struct rb_iseq_constant_body *body;
};

void
llrb_parse_iseq(const rb_iseq_t *iseq)
{
  // TODO: implement
}
