require 'mkmf'

dir_config("ribbit")

have_library("git2")
have_library('z')

create_makefile("ribbit")

