require 'mkmf'

gem "mini_portile2", "~> 2.0.0"
require "mini_portile2"

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

class Libgit2Recipe < MiniPortile
  # No need to extract anything
  def extract
  end

  # No need to download anything
  def downloaded?
    true
  end

  def configure_defaults
    [
      "-DBUILD_CLAR=OFF",
      "-DTHREADSAFE=ON",
      "-DBUILD_SHARED_LIBS=OFF",
      "-DCMAKE_C_FLAGS=-fPIC",
      "-DCMAKE_BUILD_TYPE=RelWithDebInfo",
      "-G", "Unix Makefiles"
    ]
  end

  def configure_prefix
    "-DCMAKE_INSTALL_PREFIX=\"#{path}\""
  end

  def configured?
    false
  end

  def work_path
    tmp_path
  end

  def configure
    FileUtils.mkdir_p tmp_path

    execute('configure', ["cmake", LIBGIT2_DIR, *computed_options])
  end

  def activate
    super

    ENV['PKG_CONFIG_PATH'] = ((ENV['PKG_CONFIG_PATH'] || '').split(File::PATH_SEPARATOR) << File.join(path, "lib", "pkgconfig")).join(File::PATH_SEPARATOR)

    # MINGW does not have
    if windows?
      $LDFLAGS << " -L#{File.join(tmp_path, "deps", "winhttp")}"
    end
  end
end

class Libssh2Recipe < MiniPortile
  def initialize(name, version)
    super

    @files << "https://github.com/libssh2/libssh2/releases/download/libssh2-#{version}/libssh2-#{version}.tar.gz"
  end

  def activate
    super

    ENV['PKG_CONFIG_PATH'] = ((ENV['PKG_CONFIG_PATH'] || '').split(File::PATH_SEPARATOR) << File.join(path, "lib", "pkgconfig")).join(File::PATH_SEPARATOR)
  end
end

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

  if !find_executable('pkg-config')
    if windows?
      pkg_config = MiniPortile.new "pkg-config-lite", "0.28-1"
      pkg_config.files << "http://downloads.sourceforge.net/project/pkgconfiglite/#{pkg_config.version}/pkg-config-lite-#{pkg_config.version}.tar.gz"
      pkg_config.cook
      pkg_config.activate
    else
      abort "ERROR: pkg-config is required to build Rugged."
    end
  end

  if with_config("ssh") || with_config("ssh").nil?
    libssh2 = Libssh2Recipe.new "libssh2", "1.6.0"
    libssh2.cook
    libssh2.activate
  end

  libgit2 = Libgit2Recipe.new "libgit2", "bundled"
  libgit2.cook
  libgit2.activate

  $PKGCONFIG = "pkg-config --static"
  pkg_config("libgit2")
end

unless have_library 'git2' and have_header 'git2.h'
  abort "ERROR: Failed to build libgit2"
end

create_makefile("rugged/rugged")
