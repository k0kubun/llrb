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
  sh "mkdir -p tmp"
  incdirs = [
    "#{__dir__}/ext/llrb",
    "#{__dir__}/ext/llrb/cruby",
    "#{__dir__}/ext/llrb/dynamic",
    RbConfig::CONFIG['rubyhdrdir'],
    RbConfig::CONFIG['rubyarchhdrdir'],
  ]
  ruby_cflags = "#{incdirs.map {|d| "-I#{d}" }.join(' ')} #{RbConfig::CONFIG['cflags']}"
  sh "clang #{ruby_cflags} -O0 -S -emit-llvm -o #{__dir__}/ext/insns.ll #{__dir__}/ext/insns.c"
end

desc 'Compilec'
task :module_ll => [:insns_ll] do
  #sh "llvm-link -S -o #{__dir__}/ext/module.ll #{__dir__}/ext/insns.ll #{__dir__}/ext/llrb_exec.ll"
  sh "cp #{__dir__}/ext/insns.ll #{__dir__}/ext/module.ll"
end

desc 'Compile insns.ll to insns.bc'
task :module_bc => :module_ll do
  #sh "opt -O3 -S -o #{__dir__}/ext/module_opt.ll #{__dir__}/ext/module.ll"
  #sh "llvm-as -o #{__dir__}/ext/module.bc #{__dir__}/ext/module_opt.ll"
  sh "llvm-as -o #{__dir__}/ext/module.bc #{__dir__}/ext/module.ll"
end

task :default => [:clobber, :compile, :spec]
