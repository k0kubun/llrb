#ifndef LLRB_PARSER_H
#define LLRB_PARSER_H

#include <vector>
#include "iseq.h"

namespace llrb {
  enum EntryType {
    ENTRY_LABEL,
    ENTRY_LINENO,
    ENTRY_INSN,
    ENTRY_UNKNOWN,
  };

  // Iseq's bytecode entry
  class Entry {
   public:
    EntryType type;
    int lineno;
    std::string label;
    std::vector<Object> insn;
    std::vector<Entry> fallthrough;
    std::vector<Entry> branched;

    Entry(EntryType t) { type = t; }
    void Dump(int indent) const;
    static void Dump(const std::vector<Entry>& parsed);

   private:
    void PrintIndent(int indent) const;
  };

  // This Parser parses instructions and build a tree with control flow structure.
  // It drops :jump instructions and add branches to :branchunless.
  class Parser {
   private:
    const std::vector<Object>& bytecode;

   public:
    Parser(const std::vector<Object>& b) : bytecode(b) {}
    std::vector<Entry> Parse();

   private:
    Entry BuildEntry(const Object& raw_entry);
    int FindBranchEnd(int branch_start, int min_index);
    int FindLabelIndex(const std::string& label);
    int FindLastJumpTo(const std::string& label, size_t min_index);
    int RangeSize(int from_index, int to_index);
    std::vector<Entry> ParseSlice(int from_index, int to_index);
  };
}

#endif // LLRB_PARSER_H
