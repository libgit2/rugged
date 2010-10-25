Ribbit - libgit2 bindings in Ruby
===================================

Ribbit is a Ruby bindings to the libgit2 linkable C Git library. This is
for testing and using the libgit2 library in a language that is awesome.

INSTALLING AND RUNNING
========================

First you need to install libgit2:

    $ git clone git://github.com/libgit2/libgit2.git
    $ cd libgit2
    $ make
    $ make install

Next, you need to install rake-compiler:

    $ sudo gem install rake-compiler

Now that those are installed, you can install Ribbit:

    $ git clone git://github.com/libgit2/ribbit.git
    $ cd ribbit
    $ rake compile
    $ rake test


API 
==============

There is a general library for some basic Gitty methods.  So far, just converting
a raw sha (20 bytes) into a readable hex sha (40 char).

    raw = Ribbit::Lib.hex_to_raw(hex_sha)
    hex = Ribbit::Lib.raw_to_hex(20_byte_raw_sha)

There is an Odb class that you can instantiate with a path (currently the 'objects'
path in the .git directory, but I'll probably change that soon - patch, anybody?).
This lets you check for objects, read raw object data, write raw object data and
get a hash (SHA1 checksum) of what contents would be without writing them out.

    odb = Ribbit::Odb.new("/opt/repo.git/objects")  # takes the object path, currently

                  bool = odb.exists(hex_sha)
    data, length, type = odb.read(hex_sha)  # or false if object does not exist
               hex_sha = odb.hash(content, type) # 'commit', 'blob', 'tree', 'tag'
               hex_sha = odb.write("my content\n", "blob")

Finally, there is a Walker class that currently takes an object path (probably will
eventually change this to just be instantiated from an Odb, like `walker = odb.walker` 
rather than seperately instantiated with the same path).  You can push head SHAs
onto the walker, then call next to get a list of the reachable commit objects, one
at a time. You can also hide() commits if you are not interested in anything beneath
them (useful for a `git log master ^origin/master` type deal).

    walker = Ribbit::Walker.new(path)  # also takes object path

      walker.push(hex_sha_interesting)
      walker.hide(hex_sha_uninteresting)
      hex_sha = walker.next # false if none left
      walker.reset


TODO
==============

I will try to keep this up to date with the working public API available in
the libgit2 linkable library.  Whatever is available there should be here
as well.


CONTRIBUTING
==============

Fork schacon/ribbit on GitHub, make it awesomer (preferably in a branch named
for the topic), send a pull request.


AUTHORS 
==============

Scott Chacon <schacon@gmail.com>


LICENSE
==============

MIT.

