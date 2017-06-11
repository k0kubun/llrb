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

# Link LLVM
unless system('which llvm-config 2>&1 >/dev/null')
  raise "llvm-config(1) must be available!\nNot found in PATH='#{ENV['PATH']}'"
end
$CFLAGS = "#{$CFLAGS} -Wall -W #{`llvm-config --cflags`.rstrip}"
#$CXXFLAGS = "#{$CXXFLAGS} -Wall -W #{`llvm-config --cxxflags`.rstrip}"
#$LDFLAGS  = "#{$LDFLAGS} #{`llvm-config --ldflags`.rstrip} #{`llvm-config --libs core engine`}"
#$LDFLAGS  = "#{$LDFLAGS} #{`llvm-config --cxxflags --ldflags --libs core executionengine interpreter analysis native bitwriter --system-libs`.rstrip}"
#RbConfig::MAKEFILE_CONFIG['LDSHARED']
$LDFLAGS  = "#{$LDFLAGS} #{`llvm-config --ldflags`.rstrip} #{`llvm-config --libs core engine native`}"

# To include ccan/*, add ext/llrb/cruby under include path
$INCFLAGS = "#{$INCFLAGS} -I$(srcdir)/cruby"

create_makefile('llrb/llrb')
