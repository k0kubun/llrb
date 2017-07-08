require 'bundler/gem_tasks'
require 'rspec/core/rake_task'

RSpec::Core::RakeTask.new(:spec)

require 'rake/extensiontask'

task :build => :compile

Rake::ExtensionTask.new('llrb') do |ext|
  ext.lib_dir = 'lib/llrb'
end

task :default => [:insns_bc, :compile, :spec]

desc 'Compile insns.ll to insns.bc'
task :insns_bc do
  require_relative 'ext/llrb/llrb_extconf.rb'
  LLRBExtconf.compile_bitcodes
end

desc 'Clone optcarrot'
file :optcarrot do
  sh 'git clone --depth 1 https://github.com/mame/optcarrot'
  Dir.chdir("#{__dir__}/optcarrot") do
    File.write('Gemfile', "#{File.read('Gemfile')}\ngem 'llrb', path: '..'\n")
  end
end

desc 'Run optcarrot benchmark'
task 'optcarrot:run' => :optcarrot do
  Dir.chdir("#{__dir__}/optcarrot") do
    sh "bundle check || bundle install -j#{`nproc`.rstrip}"
    sh 'bundle exec ruby -I../lib -rllrb/start bin/optcarrot --benchmark examples/Lan_Master.nes'
  end
end
