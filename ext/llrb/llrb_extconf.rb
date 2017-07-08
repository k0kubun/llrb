module LLRBExtconf
  class << self
    def configure
      remove_invalid_warnflags
      add_cflags
      link_llvm
    end

    def compile_bitcodes
      Dir.chdir(extdir) { Dir.glob('*.c') }.each do |c_file|
        compile_bitcode(c_file, c_file.sub(/\.c\z/, '.bc'))
      end
    end

    private

    def remove_invalid_warnflags
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
    end

    def add_cflags
      # To include ccan/*, add ext/llrb/cruby under include path. "cruby_extra" dir has CRuby's dynamic headers.
      $INCFLAGS = "#{$INCFLAGS} -I$(srcdir)/cruby -I$(srcdir)/cruby_extra"

      bitcode_dir = File.expand_path("#{__dir__}/..")
      $CFLAGS = "#{$CFLAGS} -DLLRB_BITCODE_DIR='\"#{bitcode_dir}\"' -Wall -Werror -W" # remove -Werror later
      $CXXFLAGS = "#{$CXXFLAGS} -Wall -Werror -W" # remove -Werror later
    end

    def link_llvm
      unless system('which llvm-config 2>&1 >/dev/null')
        raise "llvm-config(1) must be available!\nNot found in PATH='#{ENV['PATH']}'"
      end

      $CFLAGS = "#{$CFLAGS} #{`llvm-config --cflags`.rstrip}"
      $CXXFLAGS = "#{$CXXFLAGS} #{`llvm-config --cxxflags`.rstrip}"
      $LDFLAGS = "#{$LDFLAGS} #{`llvm-config --ldflags`.rstrip} #{`llvm-config --libs core engine passes`}"
    end

    def compile_bitcode(c_file, bc_file)
      unless bc_file.end_with?('.bc')
        raise ArgumentError, "bitcode file should end with .bc but got '#{bc_file}'"
      end
      ll_file = bc_file.sub(/\.bc\z/, '.ll')

      incflags = [
        __dir__,
        "#{__dir__}/cruby",
        "#{__dir__}/cruby_extra",
        RbConfig::CONFIG['rubyhdrdir'],
        RbConfig::CONFIG['rubyarchhdrdir'],
      ].map {|d| "-I#{d}" }.join(' ')

      ruby_cflags = RbConfig::CONFIG['cflags'].dup.tap do |flags|
        %w[
          -Wno-packed-bitfield-compat
          -Wsuggest-attribute=noreturn
          -Wsuggest-attribute=format
          -Wno-maybe-uninitialized
        ].each { |f| flags.sub!(f, '') } # maybe caused by compiling ruby with gcc and this with clang.
      end

      debug_flags = "-Xclang -print-stats" if false

      sh "clang #{incflags} #{ruby_cflags} #{debug_flags} -Werror -O2 -S -emit-llvm -o #{extdir}/#{ll_file} #{extdir}/#{c_file}" # remove -Werror later
      sh "llvm-as -o #{extdir}/#{bc_file} #{extdir}/#{ll_file}"
    end

    def sh(command)
      $stderr.puts command
      system(command) || raise("Failed to execute '#{command}'")
    end

    def extdir
      File.expand_path("#{__dir__}/..")
    end
  end
end
