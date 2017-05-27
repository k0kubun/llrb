# coding: utf-8
lib = File.expand_path('../lib', __FILE__)
$LOAD_PATH.unshift(lib) unless $LOAD_PATH.include?(lib)
require 'llrb/version'

Gem::Specification.new do |spec|
  spec.name          = 'llrb'
  spec.version       = LLRB::VERSION
  spec.authors       = ['Takashi Kokubun']
  spec.email         = ['takashikkbn@gmail.com']

  spec.summary       = 'LLRB is LLVM-based JIT compiler for Ruby'
  spec.description   = 'LLRB is LLVM-based JIT compiler for Ruby'
  spec.homepage      = 'https://github.com/k0kubun/llrb'
  spec.license       = 'MIT'

  spec.files         = (Dir.exist?('.git') ? `git ls-files -z`.split("\x0") : `find .`.split("\n")).reject do |f|
    f.match(%r{^(test|spec|features)/})
  end
  spec.bindir        = 'exe'
  spec.executables   = spec.files.grep(%r{^exe/}) { |f| File.basename(f) }
  spec.require_paths = ['lib']
  spec.extensions    = ['ext/llrb/extconf.rb']

  spec.add_development_dependency 'benchmark-ips'
  spec.add_development_dependency 'bundler'
  spec.add_development_dependency 'rake'
  spec.add_development_dependency 'rake-compiler'
  spec.add_development_dependency 'rspec', '~> 3.0'
end
