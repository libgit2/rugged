require 'mkmf'

RbConfig::MAKEFILE_CONFIG['CC'] = ENV['CC'] if ENV['CC']

$CFLAGS << " #{ENV["CFLAGS"]}"
$CFLAGS << " -g"

if RbConfig::MAKEFILE_CONFIG['CC'] =~ /gcc|clang/
  $CFLAGS << " -O3" unless $CFLAGS[/-O\d/]
  $CFLAGS << " -Wall"
end

MAKE_PROGRAM = find_executable('gmake') || find_executable('make')

if MAKE_PROGRAM.nil?
  abort 'GNU make is required to build Rugged'
end

dir_config('libgit2')

unless have_library 'git2' and have_header 'git2.h'
  abort 'libgit2 is missing.  please install libgit2'
end

create_makefile("rugged/rugged")
