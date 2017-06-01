#ifndef LLRB_ISEQ_H
#define LLRB_ISEQ_H

#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include "llrb.h"

namespace llrb {
  class Object {
   public:
    std::string klass;
    VALUE raw;
    std::string symbol;
    std::string string;
    std::vector<Object> array;
    std::map<std::string, Object> hash;
    union {
      int integer;
      bool boolean;
    };

    Object(VALUE ruby_obj = Qnil) {
      raw = ruby_obj;
      klass = rb_obj_classname(ruby_obj);
      SetTypeSpecificField();
    }
    void SetTypeSpecificField();
  };

  class Iseq : public Object {
   public:
    std::vector<Object> bytecode;
    int arg_size;
    int local_size;

    Iseq(VALUE ruby_obj):Object(ruby_obj) {
      arg_size = array[4].hash["arg_size"].integer;
      local_size = array[4].hash["local_size"].integer;
      bytecode = array[13].array;
    };
  };
}

#endif // LLRB_ISEQ_H
