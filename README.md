Rugged - libgit2 bindings in Ruby
===================================

Rugged is a Ruby bindings to the libgit2 linkable C Git library. This is
for testing and using the libgit2 library in a language that is awesome.

INSTALLING AND RUNNING
========================

First you need to install libgit2:

    $ git clone git://github.com/libgit2/libgit2.git
    $ cd libgit2
    $ mdkir build && cd build
    $ cmake ..
    $ make
    $ make install

Next, you need to install rake-compiler:

    $ sudo gem install rake-compiler

Now that those are installed, you can install Rugged:

    $ git clone git://github.com/libgit2/rugged.git
    $ cd rugged
    $ rake compile
    $ rake test


API 
==============

There is a general library for some basic Gitty methods.  So far, just converting
a raw sha (20 bytes) into a readable hex sha (40 char).

    raw = Rugged::Lib.hex_to_raw(hex_sha)
    hex = Rugged::Lib.raw_to_hex(20_byte_raw_sha)


Repository Access
-----------------

There is a Repository class that you can instantiate with a path.
This lets you check for objects, read raw object data, write raw object data and
get a hash (SHA1 checksum) of what contents would be without writing them out.
You also use it to lookup Git objects from it.

Repository is the main repository object that everything
else will emanate from.

    repo =
    Rugged::Repository.new(path)
      ctnt, type = repo.read(sha)
      gobj = repo.lookup(sha, type[?])  # optional type argument for checking
      sha  = repo.write(content, type)
      sha  = repo.hash(content, type)
      bool = repo.exists(sha)
      index = repo.index

The 'path' argument must point to an actual git repository folder. The library
will automatically guess if it's a bare repository or a '.git' folder inside a
working repository, and locate the working path accordingly.

Object Access
-----------------

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
    Rugged::Object(repo, sha)
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
    Rugged::Commit.new < Rugged::Object
      str   = gobjc.message
      str   = gobjc.message_short
      str   = gobjc.message_body # TODO
      prsn  = gobjc.author
      prsn  = gobjc.committer
      gobjr = gobjc.tree
      sha   = gobjc.tree_sha
      arr   = gobjc.parents [*] # TODO: write

    gobtg =
    Rugged::Tag.new < Rugged::Object
      gobj  = gobtg.target
      int   = gobtg.target_type
      str   = gobtg.name
      prsn  = gobtg.tagger
      str   = gobtg.message

    gobtr =
    Rugged::Tree.new < Rugged::Object
              gobtr.add(ent) # TODO
              gobtr.remove(name) # TODO
      int   = gobtr.entry_count
      ent   = gobtr.get_entry

    ent =
    Rugged::TreeEntry.new(attributes, name, sha)
      int  = ent.attributes
      str  = ent.name
      sha  = ent.sha
      gobj = ent.to_object

// * Person information is returned as a hash table


Commit Walker
-----------------

There is also a Walker class that currently takes a repo object. You can push 
head SHAs onto the walker, then call next to get a list of the reachable commit 
objects, one at a time. You can also hide() commits if you are not interested in
anything beneath them (useful for a `git log master ^origin/master` type deal).

    walker = 
    Rugged::Walker.new(repo) 
             walker.push(hex_sha_interesting)
             walker.hide(hex_sha_uninteresting)
      cmt  = walker.next # false if none left
             walker.reset



Index/Staging Area
-------------------

We can inspect and manipulate the Git Index as well. To work with the index inside
of an existing repository, instantiate it by using the `Repository.index` method instead
of manually opening the Index by its path.

    # TODO: the remove and add functions immediately flush to the index file on disk
    index =
    Rugged::Index.new(path)
            index.reload              # re-read the index file from disk
      int = index.entry_count # count of index entries
      ent = index.get_entry(i/path)
            index.remove(i/path)
            index.add(ientry)    # also updates existing entry if there is one
            index.add(path)      # create ientry from file in path, update index
      #TODO index.read_tree(gobtr, path='/')
      #TODO index.write_tree
    
    ientry = 
    Rugged::IndexEntry.new(index, offset)
      str = ientry.path
     time = ientry.ctime
     time = ientry.mtime
      str = ientry.sha
      int = ientry.dev
      int = ientry.ino
      int = ientry.mode
      int = ientry.uid
      int = ientry.gid
      int = ientry.file_size
      int = ientry.flags # (what flags are available?)
      int = ientry.flags_extended # (what flags are available?)

Index Status # TODO
-------------------

      #TODO index.status # how does the index differ from the work tree and the last commit

    # >> pp stat
    # [ ['file1', :staged],
    #   ['file2', :modified],
    #   ['file3', :deleted],
    #   ['file4', :untracked],
    #   ['file4', :unmerged],
    # ]


Ref Management # TODO
-------------------

The RefList class allows you to list, create and delete packed and loose refs.

    list =
    Rugged::RefList.new(repo)
     ref   = list.head         # can retrieve and set HEAD with this - returns ref obj or commit obj if detatched
     array = list.list([type]) # type is 'heads', 'tags', 'remotes', 'notes', et
             list.add(oref)
             list.pack
             list.unpack

    oref =
    Rugged::Ref.new(ref, sha)
             br.name # master
             br.ref  # refs/heads/master
             br.type # heads
             br.object
             br.sha
             br.delete
             br.save


Config Management # TODO
------------------------

    conf =
    Rugged::Config.new(repo)
      hash = conf.list([section])
       val = conf.get(key, [scope])
      bool = conf.set(key, value, [scope]) # scope is 'local'(default), 'global', 'system'


Client Transport # TODO
-----------------------

    client =
    Rugged::Client.new(repo)
    summry = client.fetch(url, [refs])
    summry = client.push(url, refs)
      refs = client.refs(url)  # ls-remote


Remote Management # TODO
------------------------

    remlist =
    Rugged::RemoteList.new(repo)
    array = remlist.list
      rem = remlist.add(alias, url)

    rem =
    Rugged::Remote.new(repo)
    summry = rem.fetch([refs])
    summry = rem.push(refs)
    summry = rem.remove(refs)
      bool = rem.delete
     heads = rem.heads


Server Transport # TODO
------------------------

    server =
    Rugged::Server.new
             server.listen(port = 8080, ip = 0.0.0.0)
             server.export_repo(path/repo)
             server.export_path(path)


TODO
==============

I will try to keep this up to date with the working public API available in
the libgit2 linkable library.  Whatever is available there should be here
as well.  The latest libgit2 commit known to link and build successfully will
be listed in the LIBGIT2_VERSION file.


CONTRIBUTING
==============

Fork libgit2/rugged on GitHub, make it awesomer (preferably in a branch named
for the topic), send a pull request.


AUTHORS 
==============

* Scott Chacon <schacon@gmail.com>
* Vicent Marti <tanoku@gmail.com>


LICENSE
==============

MIT.  See LICENSE file.

