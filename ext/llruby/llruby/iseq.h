#ifndef LLRUBY_ISEQ_H
#define LLRUBY_ISEQ_H

#include <vector>
#include "ruby.h"

namespace llruby {

class Object {
 public:
  const char *klass;

 private:
  VALUE raw;

 public:
  Object(VALUE ruby_obj) {
    raw = ruby_obj;
    klass = rb_obj_classname(ruby_obj);
  }
}; // class Object

class Iseq : Object {
 public:
  std::vector<Object> bytecode;

  Iseq(VALUE ruby_obj):Object(ruby_obj) {
    SetBytecode();
  };

 private:
  void SetBytecode();
}; // class Iseq

}; // namespace llruby

#endif // LLRUBY_ISEQ_H
