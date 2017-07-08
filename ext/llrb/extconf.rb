require 'mkmf'

require_relative 'llrb_extconf'
LLRBExtconf.compile_bitcodes

LLRBExtconf.configure
create_makefile('llrb/llrb')
