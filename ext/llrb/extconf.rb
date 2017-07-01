require 'mkmf'

# cc1plus: warning: command line option ‘-Wimplicit-int’ is valid for C/ObjC but not for C++
# cc1plus: warning: command line option ‘-Wdeclaration-after-statement’ is valid for C/ObjC but not for C++
# cc1plus: warning: command line option ‘-Wimplicit-function-declaration’ is valid for C/ObjC but not for C++
# cc1plus: warning: unrecognized command line option ‘-Wno-self-assign’
# cc1plus: warning: unrecognized command line option ‘-Wno-constant-logical-operand’
# cc1plus: warning: unrecognized command line option ‘-Wno-parentheses-equality’
# cc1plus: warning: unrecognized command line option ‘-Wno-tautological-compare’
%w[
  -Wimplicit-int
  -Wdeclaration-after-statement
  -Wimplicit-function-declaration
  -Wno-self-assign
  -Wno-constant-logical-operand
  -Wno-parentheses-equality
  -Wno-tautological-compare
].each do |flag|
  CONFIG['warnflags'].gsub!(flag, '')
end

# To include ccan/*, add ext/llrb/cruby under include path. "cruby_extra" dir has CRuby's dynamic headers.
$INCFLAGS = "#{$INCFLAGS} -I$(srcdir)/cruby -I$(srcdir)/cruby_extra"

create_makefile('llrb/llrb')
