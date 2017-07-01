require "bundler/gem_tasks"
require "rspec/core/rake_task"

RSpec::Core::RakeTask.new(:spec)

require "rake/extensiontask"

task :build => :compile

Rake::ExtensionTask.new("llrb") do |ext|
  ext.lib_dir = "lib/llrb"
end

desc 'Compile insns.c to insns.ll'
task :insns_ll do
  incdirs = [
    "#{__dir__}/ext/llrb",
    "#{__dir__}/ext/llrb/cruby",
    "#{__dir__}/ext/llrb/dynamic",
    RbConfig::CONFIG['rubyhdrdir'],
    RbConfig::CONFIG['rubyarchhdrdir'],
  ]
  ruby_cflags = "#{incdirs.map {|d| "-I#{d}" }.join(' ')} #{RbConfig::CONFIG['cflags']}"

  %w[
    -Wno-packed-bitfield-compat
    -Wsuggest-attribute=noreturn
    -Wsuggest-attribute=format
    -Wno-maybe-uninitialized
  ].each { |f| ruby_cflags.sub!(f, '') } # caused by compiling ruby with gcc and this with clang.

  sh "clang #{ruby_cflags} -O3 -S -emit-llvm -o #{__dir__}/ext/insns.ll #{__dir__}/ext/insns.c"
end

desc 'Compile insns.ll to insns.bc'
task :insns_bc => :insns_ll do
  sh "llvm-as -o #{__dir__}/ext/insns.bc #{__dir__}/ext/insns.ll"
end

task :default => [:insns_bc, :clobber, :compile, :spec]
