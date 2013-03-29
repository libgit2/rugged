# Return program name to use
# for GNU Makefiles depending on
# OS string name
#
# For example, BSD systems use "gmake" for Makefiles made for GNU make while
# Linux, Darwin and many others use "make"
#
# For more details on what happens when running "make" on
# GNU Makefile in FreeBSD, see this issue:
#
# https://github.com/libgit2/libgit2/issues/1442
#
def gnumake_for(os_string)
  case os_string.downcase
    # Regex: IF contains "bsd" and does NOT contain "gnu"
    #
    # The reason for excluding "gnu" is because "Debian GNU/kFreeBSD"
    # should be treated as Linux
    # For more details: http://en.wikipedia.org/wiki/Debian_GNU/kFreeBSD
    #
    when /^(?=.*bsd)(?!.*gnu).*/ then "gmake"
    else "make"
  end
end
