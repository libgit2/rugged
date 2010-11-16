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
    Ribbit::Repository.new(path, git_dir=nil, index_path=nil)
      ctnt, type = repo.read(sha)
      gobj = repo.lookup(sha, type[?])  # optional type argument for checking
      sha  = repo.write(content, type)
      sha  = repo.hash(content, type)
      bool = repo.exists(sha)

If Repository is initialized without `git_dir`, path + '.git' will be assumed
and path will be assumed to be the working directory.  If `path` is a git 
directory, then `git_dir` will be set to that and none of the Ribbit functions
that need a working directory will work. If the `index_path` is not specified, 
the `git_dir` path plus '/index' will be assumed.

Object is the main object class - it shouldn't be created directly,
but all of these methods should be useful in it's derived classes

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

// * Person information is returned as a hash table

There is also a Walker class that currently takes a repo object. You can push 
head SHAs onto the walker, then call next to get a list of the reachable commit 
objects, one at a time. You can also hide() commits if you are not interested in
anything beneath them (useful for a `git log master ^origin/master` type deal).

    walker = 
    Ribbit::Walker.new(repo) 
             walker.push(hex_sha_interesting)
             walker.hide(hex_sha_uninteresting)
      cmt  = walker.next # false if none left
             walker.reset


We can inspect and manipulate the Git Index as well.

    # the remove and add functions immediately flush to the index file on disk
    index =
    Ribbit::Index.new(repo, path=nil)
            index.refresh              # re-read the index file from disk
      int = index.entry_count # count of index entries
      ent = index.get_entry(i/path)
            index.remove(i/path)
            index.add(ientry)    # also updates existing entry if there is one
            index.add(path)      # create ientry from file in path, update index
      #TODO index.read_tree(gobtr, path='/')
      #TODO index.status # how does the index differ from the work tree and the HEAD commit
      #TODO index.write_tree
    
    # >> pp stat
    # [ ['file1', :staged],
    #   ['file2', :modified],
    #   ['file3', :deleted],
    #   ['file4', :untracked],
    #   ['file4', :unmerged],
    # ]

    ientry = 
    Ribbit::IndexEntry.new(index, offset)
      str = ientry.path  # TODO
     time = ientry.ctime # TODO
     time = ientry.mtime # TODO
      str = ientry.sha   # TODO
      int = ientry.dev
      int = ientry.ino
      int = ientry.mode
      int = ientry.uid
      int = ientry.gid
      int = ientry.file_size
      int = ientry.flags # (what flags are available?)
      int = ientry.flags_extended # (what flags are available?)

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

* Scott Chacon <schacon@gmail.com>
* Vicent Marti <tanoku@gmail.com>


LICENSE
==============

MIT.  See LICENSE file.

