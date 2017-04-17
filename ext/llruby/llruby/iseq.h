#ifndef LLRUBY_ISEQ_H
#define LLRUBY_ISEQ_H

#include <stdexcept>
#include <string>
#include <vector>
#include <map>
#include "ruby.h"

namespace llruby {

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
}; // class Object

class Iseq : public Object {
 public:
  std::vector<Object> bytecode;

  Iseq(VALUE ruby_obj):Object(ruby_obj) {
    if (TYPE(ruby_obj) != T_ARRAY || RARRAY_LEN(ruby_obj) != 14) {
      throw std::runtime_error(std::string("unexpected object is given!: ") + RSTRING_PTR(rb_inspect(ruby_obj)));
    }
    bytecode = array[13].array;
  };
}; // class Iseq

}; // namespace llruby

#endif // LLRUBY_ISEQ_H
