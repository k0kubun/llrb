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

# Link LLVM
unless system('which llvm-config 2>&1 >/dev/null')
  raise "llvm-config(1) must be available!\nNot found in PATH='#{ENV['PATH']}'"
end
bitcode_dir = File.expand_path("#{__dir__}/..")
$CFLAGS = "#{$CFLAGS} -DLLRB_BITCODE_DIR='\"#{bitcode_dir}\"' -Wall -Werror -W #{`llvm-config --cflags`.rstrip}" # remove -Werror later
$CXXFLAGS = "#{$CXXFLAGS} -Wall -Werror -W #{`llvm-config --cxxflags`.rstrip}" # remove -Werror later
$LDFLAGS = "#{$LDFLAGS} #{`llvm-config --ldflags`.rstrip} #{`llvm-config --libs core engine passes`}"

create_makefile('llrb/llrb')
