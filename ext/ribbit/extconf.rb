require 'mkmf'

dir_config("ribbit")

have_library("git2")

create_makefile("ribbit")

