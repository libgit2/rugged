require 'mkmf'

def sys(cmd)
  puts " -- #{cmd}"
  unless ret = xsystem(cmd)
    raise "ERROR: '#{cmd}' failed"
  end
  ret
end

if `which make`.strip.empty?
  STDERR.puts "ERROR: GNU make is required to build Rugged"
  exit(1)
end

if p = ENV['LIBGIT2_PATH']
  $INCFLAGS[0,0] = " -I#{File.join(p, 'include')} "
  $LDFLAGS << " -L#{p} "
  unless have_library 'git2' and have_header 'git2.h'
    STDERR.puts "ERROR: Invalid `LIBGIT2_PATH` environment"
    exit(1)
  end
else
  CWD = File.expand_path(File.dirname(__FILE__))
  LIBGIT2_DIR = File.join(CWD, '..', '..', 'vendor', 'libgit2')
  LIBGIT2_LIB_PATH = "#{CWD}/libgit2_embed.a"

  if !File.exists?(LIBGIT2_LIB_PATH)
    Dir.chdir(LIBGIT2_DIR) do
      sys("make -f Makefile.embed")
      FileUtils.cp 'libgit2.a', LIBGIT2_LIB_PATH
    end
  end

  $INCFLAGS[0,0] = " -I#{LIBGIT2_DIR}/include "
  $LDFLAGS << " -L#{CWD} "

  unless have_library 'git2_embed' and have_header 'git2.h'
    STDERR.puts "ERROR: Failed to build libgit2"
    exit(1)
  end
end

create_makefile("rugged/rugged")
