# coding: utf-8
lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require 'llrb/version'

Gem::Specification.new do |spec|
  spec.name          = 'llrb'
  spec.version       = LLRB::VERSION
  spec.authors       = ['Takashi Kokubun']
  spec.email         = ['takashikkbn@gmail.com']

  spec.summary       = 'LLRB is LLVM-based JIT compiler for Ruby. Note that this gem can be only installed to fork of CRuby.'
  spec.description   = 'LLRB is LLVM-based JIT compiler for Ruby. Note that this gem can be only installed to fork of CRuby.'
  spec.homepage      = 'https://github.com/k0kubun/llrb'
  spec.license       = 'MIT'

  ccans = Dir.chdir("#{__dir__}/ext/llrb/cruby") { Dir.glob("ccan/**/*.h") }
  cruby = (ccans + %w[
    internal.h
    iseq.h
    method.h
    node.h
    probes_helper.h
    ruby_assert.h
    ruby_atomic.h
    thread_pthread.h
    vm_core.h
    vm_debug.h
    vm_exec.h
    vm_opts.h
  ]).map { |f| "ext/llrb/cruby/#{f}" }

  spec.files = `git ls-files -z`.split("\x0").reject do |f|
    f.match(%r{^(test|spec|features)/})
  end + cruby

  spec.bindir        = 'exe'
  spec.executables   = spec.files.grep(%r{^exe/}) { |f| File.basename(f) }
  spec.require_paths = ['lib']
  spec.extensions    = ['ext/llrb/extconf.rb']

  spec.add_development_dependency 'benchmark-ips'
  spec.add_development_dependency 'bundler'
  spec.add_development_dependency 'pry'
  spec.add_development_dependency 'rake'
  spec.add_development_dependency 'rake-compiler'
  spec.add_development_dependency 'rspec', '~> 3.0'
end
