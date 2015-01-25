require 'mkmf'

RbConfig::MAKEFILE_CONFIG['CC'] = ENV['CC'] if ENV['CC']

$CFLAGS << " #{ENV["CFLAGS"]}"
$CFLAGS << " -g"
$CFLAGS << " -O3" unless $CFLAGS[/-O\d/]
$CFLAGS << " -Wall -Wno-comment"

def sys(cmd)
  puts " -- #{cmd}"
  unless ret = xsystem(cmd)
    raise "ERROR: '#{cmd}' failed"
  end
  ret
end

if !(MAKE = find_executable('gmake') || find_executable('make'))
  abort "ERROR: GNU make is required to build Rugged."
end

CWD = File.expand_path(File.dirname(__FILE__))
LIBGIT2_DIR = File.expand_path(File.join('..', '..', 'vendor', 'libgit2'), CWD)
LIBGIT2_BUILD_DIR = File.join(LIBGIT2_DIR, 'build')
LIBGIT2_INCLUDE_DIR = File.join(LIBGIT2_DIR, 'include')

if arg_config("--use-system-libraries", !!ENV['RUGGED_USE_SYSTEM_LIBRARIES'])
  puts "Building Rugged using system libraries.\n"

  dir_config('git2').any? or pkg_config('libgit2')

  major = minor = nil

  File.readlines(File.join(LIBGIT2_DIR, "include", "git2", "version.h")).each do |line|
    if !major && (matches = line.match(/^#define LIBGIT2_VER_MAJOR ([0-9]+)$/))
      major = matches[1]
      next
    end

    if !minor && (matches = line.match(/^#define LIBGIT2_VER_MINOR ([0-9]+)$/))
      minor = matches[1]
      next
    end

    break if major && minor
  end

  try_compile(<<-SRC) or abort "libgit2 version is not compatible"
#include <git2/version.h>

#if LIBGIT2_VER_MAJOR != #{major} || LIBGIT2_VER_MINOR != #{minor}
#error libgit2 version is not compatible
#endif
  SRC
else
  if !find_executable('cmake')
    abort "ERROR: CMake is required to build Rugged."
  end

  if !find_executable('pkg-config')
    abort "ERROR: pkg-config is required to build Rugged."
  end

  Dir.mkdir(LIBGIT2_BUILD_DIR) unless Dir.exists?(LIBGIT2_BUILD_DIR)
  Dir.chdir(LIBGIT2_BUILD_DIR) do
    sys("cmake .. -DBUILD_CLAR=OFF -DTHREADSAFE=ON -DBUILD_SHARED_LIBS=OFF -DCMAKE_C_FLAGS=-fPIC -DCMAKE_BUILD_TYPE=RelWithDebInfo -G \"Unix Makefiles\"")
    sys(MAKE)

    pcfile = File.join(LIBGIT2_BUILD_DIR, "libgit2.pc")
    $LDFLAGS << " " + `pkg-config --libs --static #{pcfile}`.strip
  end

  # Prepend the vendored libgit2 build dir to the $DEFLIBPATH.
  #
  # By default, $DEFLIBPATH includes $(libpath), which usually points
  # to something like /usr/lib for system ruby versions (not those
  # installed through rbenv or rvm).
  #
  # This was causing system-wide libgit2 installations to be preferred
  # over of our vendored libgit2 version when building rugged.
  #
  # By putting the path to the vendored libgit2 library at the front of
  # $DEFLIBPATH, we can ensure that our bundled version is always used.
  $DEFLIBPATH.unshift(LIBGIT2_BUILD_DIR)
  dir_config('git2', LIBGIT2_INCLUDE_DIR, LIBGIT2_BUILD_DIR)
end

unless have_library 'git2' and have_header 'git2.h'
  abort "ERROR: Failed to build libgit2"
end

create_makefile("rugged/rugged")
