require 'mkmf'

dir_config("git2")
dir_config("z")

def asplode(missing)
  abort <<-error
#{missing} is missing, Try installing or compiling it first.
You can provide configuration options to alternate places:

  --with-git2-dir=...
  --with-z-dir=...

error
end

asplode('libgit2') unless have_library("git2")
asplode('zlib') unless have_library('z')

create_makefile("rugged/rugged")
