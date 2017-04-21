FROM k0kubun/llvm38-ruby:latest
MAINTAINER Takashi Kokubun <takashikkbn@gmail.com>

RUN mkdir -p /tmp/lib/llruby
COPY lib/llruby/version.rb /tmp/lib/llruby/
COPY Gemfile Gemfile.lock llruby.gemspec /tmp/
RUN cd /tmp && bundle -j4 --path=vendor/bundle

RUN mkdir -p /llruby
COPY . /llruby/
RUN mv /tmp/vendor /llruby/vendor && mv /tmp/.bundle /llruby/.bundle
WORKDIR /llruby

RUN bundle exec rake compile
CMD bin/console
