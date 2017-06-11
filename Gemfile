source 'https://rubygems.org'

# Specify your gem's dependencies in llrb.gemspec
gemspec

group :development do
  if RUBY_VERSION >= '2.4.0' && ENV.key?('WERCKER_STEP_NAME').!
    gem 'pry', git: 'https://github.com/pry/pry'
  else
    gem 'pry'
  end
  gem 'pry-byebug'
end
