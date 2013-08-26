require 'mkmf'

RbConfig::MAKEFILE_CONFIG['CC'] = ENV['CC'] if ENV['CC']

$CFLAGS << " #{ENV["CFLAGS"]}"
$CFLAGS << " -g"

if RbConfig::MAKEFILE_CONFIG['CC'] =~ /gcc|clang/
  $CFLAGS << " -O3" unless $CFLAGS[/-O\d/]
  $CFLAGS << " -Wall"
end

def sys(cmd)
  puts " -- #{cmd}"
  unless ret = xsystem(cmd)
    raise "ERROR: '#{cmd}' failed"
  end
  ret
end

def preserving_globals
  values =
    $arg_config,
    $CFLAGS, $CPPFLAGS,
    $LDFLAGS, $LIBPATH, $libs
  yield
ensure
  $arg_config,
  $CFLAGS, $CPPFLAGS,
  $LDFLAGS, $LIBPATH, $libs =
    values
end

MAKE_PROGRAM = find_executable('gmake') || find_executable('make')

if MAKE_PROGRAM.nil?
  abort "ERROR: GNU make is required to build Rugged"
end

if preserving_globals { dir_config('git2') }.any?
  dir_config('git2')

  unless have_library 'git2' and have_header 'git2.h'
    abort "ERROR: Can not find libgit2 in the specified path"
  end
else
  CWD = File.expand_path(File.dirname(__FILE__))
  LIBGIT2_DIR = File.join(CWD, '..', '..', 'vendor', 'libgit2')

  Dir.chdir(LIBGIT2_DIR) do
    sys("#{MAKE_PROGRAM} -f Makefile.embed")
  end

  dir_config('git2', "#{LIBGIT2_DIR}/include", LIBGIT2_DIR)
  unless have_library 'git2' and have_header 'git2.h'
    abort "ERROR: Failed to build libgit2"
  end
end

create_makefile("rugged/rugged")
