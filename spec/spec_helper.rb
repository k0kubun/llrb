require 'bundler/setup'
require 'llrb'

# If we do this via Class.new, it prints warning...
class ClassVariableTest
  def self.test_getclassvariable
    @@a = 1
    @@a
  end

  def self.test_setclassvariable
    @@b = 2
  end
end

RSpec.configure do |config|
  # Enable flags like --only-failures and --next-failure
  config.example_status_persistence_file_path = '.rspec_status'

  # Disable RSpec exposing methods globally on `Module` and `main`
  config.disable_monkey_patching!

  config.expect_with :rspec do |c|
    c.syntax = :expect
  end
end
