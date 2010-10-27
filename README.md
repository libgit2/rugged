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

There is a Repository class that you can instantiate with a path.
This lets you check for objects, read raw object data, write raw object data and
get a hash (SHA1 checksum) of what contents would be without writing them out.
You also use it to lookup Git objects from it.

Repository is the main repository object that everything
else will emanate from.

    repo =
    Ribbit::Repository.new(path)
      ctnt, type = repo.read(sha)
      gobj = repo.lookup(sha, type[?])  # optional type argument for checking
      sha  = repo.write(content, type)
      sha  = repo.hash(content, type)
      bool = repo.exists(sha)

Object is the main object class - it shouldn't be created directly,
but all of these methods should be useful in it's derived classes

# TODO: how do we prevent instantation of the Object?

    object = 
    # Constructor is inherited by all the repository objects
    # 'sha' is the ID of the object; 
    # 'repo' is the repository where the object resides
    # If both 'sha' and 'repo' exist, the object will be looked up on
    # the repository and instantiated
    # If the 'sha' ID of the object is missing, the object will be
    # created in memory and can be written later on to the repository
    Ribbit::Object(repo, sha)
      obj.sha
      obj.type

      str = obj.read_raw	# read the raw data of the object
      sha = obj.write		# write the object to a repository

The next classes are for consuming and creating the 4 base
git object types.  just about every method should be able to take
of each should be able to take a parameter to change the value
so the object can be re-written slightly differently or no parameter
to simply read the current value out

    gobjc =
    Ribbit::Commit.new < Ribbit::Object
      str   = gobjc.message
      str   = gobjc.message_short
      str   = gobjc.message_body # TODO
      prsn  = gobjc.author
      prsn  = gobjc.committer
      gobjr = gobjc.tree
      sha   = gobjc.tree_sha
      arr   = gobjc.parents [*] # TODO

    gobtg =
    Ribbit::Tag.new < Ribbit::Object
      gobj  = gobtg.target
      int   = gobtg.target_type
      str   = gobtg.name
      prsn  = gobtg.tagger
      str   = gobtg.message

    gobtr =
    Ribbit::Tree.new < Ribbit::Object
              gobtr.add(ent) # TODO
              gobtr.remove(name) # TODO
      int   = gobtr.entry_count
      ent   = gobtr.get_entry

    ent =
    Ribbit::TreeEntry.new(attributes, name, sha)
      int  = ent.attributes
      str  = ent.name
      sha  = ent.sha
      gobj = ent.to_object

# Person information is returned as a hash table

Finally, there is a Walker class that currently takes a repo object. You can push 
head SHAs onto the walker, then call next to get a list of the reachable commit 
objects, one at a time. You can also hide() commits if you are not interested in
anything beneath them (useful for a `git log master ^origin/master` type deal).

    walker = 
    Ribbit::Walker.new(repo) 
             walker.push(hex_sha_interesting)
             walker.hide(hex_sha_uninteresting)
      cmt  = walker.next # false if none left
             walker.reset


TODO
==============

I will try to keep this up to date with the working public API available in
the libgit2 linkable library.  Whatever is available there should be here
as well.  The latest libgit2 commit known to link and build successfully will
be listed in the LIBGIT2_VERSION file.


CONTRIBUTING
==============

Fork libgit2/ribbit on GitHub, make it awesomer (preferably in a branch named
for the topic), send a pull request.


AUTHORS 
==============

Scott Chacon <schacon@gmail.com>
Vicent Marti <tanoku@gmail.com>


LICENSE
==============

(The MIT License)

Copyright (c) 2010 Scott Chacon

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
'Software'), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

