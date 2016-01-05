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

def windows?
  RbConfig::CONFIG['host_os'] =~ /mswin|mingw/
end

if !(MAKE = find_executable('gmake') || find_executable('make'))
  abort "ERROR: GNU make is required to build Rugged."
end

CWD = File.expand_path(File.dirname(__FILE__))
LIBGIT2_DIR = File.join(CWD, '..', '..', 'vendor', 'libgit2')

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

  try_compile(<<-SRC) or abort "libgit2 version is not compatible, expected ~> #{major}.#{minor}.0"
#include <git2/version.h>

#if LIBGIT2_VER_MAJOR != #{major} || LIBGIT2_VER_MINOR != #{minor}
#error libgit2 version is not compatible
#endif
  SRC
else
  if !find_executable('cmake')
    abort "ERROR: CMake is required to build Rugged."
  end

  if !windows? && !find_executable('pkg-config')
    abort "ERROR: pkg-config is required to build Rugged."
  end

  if windows?
    gem "mini_portile2", "~> 2.0.0"
    require "mini_portile2"

    pkg_config = MiniPortile.new "pkg-config-lite", "0.28-1"
    pkg_config.files << "http://downloads.sourceforge.net/project/pkgconfiglite/#{pkg_config.version}/pkg-config-lite-#{pkg_config.version}.tar.gz"
    pkg_config.cook
    pkg_config.activate

    libssh2 = MiniPortile.new "libssh2", "1.6.0"
    libssh2.files << "https://github.com/libssh2/libssh2/releases/download/libssh2-#{libssh2.version}/libssh2-#{libssh2.version}.tar.gz"
    libssh2.configure_options << "--with-wincng" << "--without-openssl"
    libssh2.cook
    libssh2.activate

    ENV['PKG_CONFIG_PATH'] = File.join(libssh2.path, "lib", "pkgconfig")
  end

  Dir.chdir(LIBGIT2_DIR) do
    Dir.mkdir("build") if !Dir.exists?("build")

    Dir.chdir("build") do
      sys("cmake .. -DBUILD_CLAR=OFF -DTHREADSAFE=ON -DBUILD_SHARED_LIBS=OFF -DCMAKE_C_FLAGS=-fPIC -DCMAKE_BUILD_TYPE=RelWithDebInfo -G \"Unix Makefiles\"")
      sys(MAKE)

      ENV['PKG_CONFIG_PATH'] = ENV['PKG_CONFIG_PATH'] + File::PATH_SEPARATOR + File.join(LIBGIT2_DIR, "build")

      # "normal" libraries (and libgit2 builds) get all these when they build but we're doing it
      # statically so we put the libraries in by hand. It's important that we put the libraries themselves
      # in $LIBS or the final linking stage won't pick them up
      if windows?
        $LDFLAGS << " " + "-L#{libssh2.path}/lib"
        $LDFLAGS << " " + "-L#{Dir.pwd}/deps/winhttp"
        $LIBS << " -lwinhttp -lcrypt32 -lrpcrt4 -lole32 " + `pkg-config --libs-only-l --static libgit2`.strip
      else
        $LDFLAGS << " " + `pkg-config --libs --static libgit2`.strip
      end
    end
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
  $DEFLIBPATH.unshift("#{LIBGIT2_DIR}/build")
  dir_config('git2', "#{LIBGIT2_DIR}/include", "#{LIBGIT2_DIR}/build")
end

unless have_library 'git2' and have_header 'git2.h'
  abort "ERROR: Failed to build libgit2"
end

create_makefile("rugged/rugged")
