#include <algorithm>
#include <cassert>
#include "parser.h"

namespace llrb {

std::vector<Entry> Parser::Parse() {
  std::vector<Entry> result;

  for (size_t i = 0; i < bytecode.size(); i++) {
    const Object& raw_entry = bytecode.at(i);
    Entry entry = BuildEntry(raw_entry);

    if (entry.type == ENTRY_INSN && entry.insn[0].symbol == "branchunless") {
      int branched_first = FindLabelIndex(entry.insn[1].symbol);
      int branched_last  = FindBranchEnd(branched_first, i);
      assert((branched_first >= 0) && (branched_last >= 0));

      entry.fallthrough = ParseSlice(i + 1, branched_first - 1);
      entry.branched    = ParseSlice(branched_first, branched_last);
      i += RangeSize(i + 1, branched_first - 1) + RangeSize(branched_first, branched_last);
    }

    result.push_back(entry);
  }

  return result;
}

int Parser::FindBranchEnd(int branch_start, int min_index) {
  const Object& jump_or_br = bytecode[branch_start - 1];
  assert(jump_or_br.klass == "Array" &&
      (jump_or_br.array[0].symbol == "jump"
       || jump_or_br.array[0].symbol == "branchunless"));

  // pass min_index to prevent detecting loop
  return std::max(
      FindLabelIndex(jump_or_br.array[1].symbol),
      FindLastJumpTo(jump_or_br.array[1].symbol, min_index));
}

Entry Parser::BuildEntry(const Object& raw_entry) {
  if (raw_entry.klass == "Array") {
    Entry entry(ENTRY_INSN);
    entry.insn = raw_entry.array;
    return entry;
  } else if (raw_entry.klass == "Symbol") {
    Entry entry(ENTRY_LABEL);
    entry.label = raw_entry.symbol;
    return entry;
  } else if (raw_entry.klass == "Fixnum" || raw_entry.klass == "Integer") {
    Entry entry(ENTRY_LINENO);
    entry.lineno = raw_entry.integer;
    return entry;
  } else {
    Entry entry(ENTRY_UNKNOWN);
    return entry;
  }
}

int Parser::FindLabelIndex(const std::string& label) {
  for (size_t i = 0; i < bytecode.size(); i++) {
    if (bytecode[i].klass == "Symbol" && bytecode[i].symbol == label) {
      return (int)i;
    }
  }
  return -1;
}

int Parser::FindLastJumpTo(const std::string& label, size_t min_index) {
  for (size_t i = bytecode.size() - 1; min_index < i; i--) {
    if (bytecode[i].klass == "Array") {
      const std::vector<Object>& insn = bytecode[i].array;
      if (insn[0].symbol == "jump" && insn[1].symbol == label) {
        return (int)i;
      }
    }
  }
  return -1;
}

int Parser::RangeSize(int from_index, int to_index) {
  return to_index - from_index + 1;
}

std::vector<Entry> Parser::ParseSlice(int from_index, int to_index) {
  std::vector<Object> slice;
  for (int i = from_index; i <= to_index; i++) {
    slice.push_back(bytecode[i]);
  }

  std::vector<Entry> ret = Parser(slice).Parse();
  if (ret.back().type == ENTRY_INSN && ret.back().insn[0].symbol == "jump") {
    ret.pop_back(); // trim last jump after parse
  }
  assert(ret.size() > 0);
  return ret;
}

void Entry::Dump(int indent) const {
  PrintIndent(indent);
  switch (type) {
    case ENTRY_LABEL:
      printf("Label :%s\n", label.c_str());
      break;
    case ENTRY_LINENO:
      printf("Lineno %d\n", lineno);
      break;
    case ENTRY_INSN:
      printf("Insn [");
      for (const Object& obj : insn) {
        printf("%s,", RSTRING_PTR(rb_inspect(obj.raw)));
      }
      printf("]\n");

      if (!fallthrough.empty()) {
        PrintIndent(indent + 1);
        printf("fallthrough:\n");
        for (const Entry& entry : fallthrough) {
          entry.Dump(indent + 2);
        }
      }

      if (!branched.empty()) {
        PrintIndent(indent + 1);
        printf("branched:\n");
        for (const Entry& entry : branched) {
          entry.Dump(indent + 2);
        }
      }
      break;
    default:
      fprintf(stderr, "unexpected type: %d\n", type);
      exit(1);
  }
}

void Entry::PrintIndent(int indent) const {
  for (int i = 0; i < indent; i++) {
    printf(" ");
  }
}

void Entry::Dump(const std::vector<Entry>& parsed) {
  for (const Entry& entry : parsed) {
    entry.Dump(0);
  }
}

} // namespace llrb
