require 'mkmf'

dir_config("rugged")

have_library("git2")
have_library('z')

create_makefile("rugged")

