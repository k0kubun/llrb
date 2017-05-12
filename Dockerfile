FROM k0kubun/llvm38-ruby:latest
MAINTAINER Takashi Kokubun <takashikkbn@gmail.com>

RUN mkdir -p /tmp/lib/llrb
COPY lib/llrb/version.rb /tmp/lib/llrb/
COPY Gemfile Gemfile.lock llrb.gemspec /tmp/
RUN cd /tmp && bundle -j4 --path=vendor/bundle

RUN mkdir -p /llrb
COPY . /llrb/
RUN mv /tmp/vendor /llrb/vendor && mv /tmp/.bundle /llrb/.bundle
WORKDIR /llrb

RUN bundle exec rake compile
CMD bin/console
