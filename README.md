Rugged - libgit2 bindings in Ruby
===================================

Rugged is a Ruby bindings to the libgit2 linkable C Git library. This is
for testing and using the libgit2 library in a language that is awesome.

INSTALLING AND RUNNING
========================

This is a self-contained gem and you should be able to easily install it
by using Rubygems:

    $ gem install rugged

API 
==============

Here are some of the ways you can use Rugged:

General Methods
---------------

There is a general library for some basic Gitty methods.  So far, just converting
a raw sha (20 bytes) into a readable hex sha (40 char).

    raw = Rugged.hex_to_raw(hex_sha)
    hex = Rugged.raw_to_hex(20_byte_raw_sha)


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
      bool   = repo.exists(sha)
      rawobj = repo.read(sha)
        len    = rawobj.len
        data   = rawobj.data
                 rawobj.type (Rugged::OBJ_COMMIT)
      sha    = repo.hash(content, type)
      sha    = repo.write(content, type)
      ref    = repo.head
        sha    = ref.target
                 ref.type ("commit")
      bool   = repo.bare?
      bool   = repo.empty?
      bool   = repo.head_detatched?
      bool   = repo.head_orphan?
      path   = repo.workdir
      path   = repo.path

      Rugged::Repository.init_at(path)

      Rugged::Repository.discover("/Users/schacon/projects/work/repo/lib/subdir/")
      => "/Users/schacon/projects/work/repo/.git/"

The 'path' argument must point to an actual git repository folder. The library
will automatically guess if it's a bare repository or a '.git' folder inside a
working repository, and locate the working path accordingly.

Object Access
-------------

Object is the main object class - it shouldn't be created directly,
but all of these methods should be useful in it's derived classes

    obj = @repo.lookup(sha)
      obj.oid  # object sha
      obj.type [OBJ_COMMIT, OBJ_TREE, OBJ_BLOB, OBJ_TAG]

    robj = obj.read_raw
      str   = robj.data
      int   = robj.len

The next classes are for consuming and creating the 4 base
git object types.

### Commit Objects

    gobjc = @repo.lookup(commit_sha)
      str   = gobjc.message
      time  = gobjc.time       # author time
      prsn  = gobjc.author
         str   = prsn[:name]
         str   = prsn[:time]
         str   = prsn[:email]
      prsn  = gobjc.committer
      gobjr = gobjc.tree
      sha   = gobjc.tree.oid
      arr   = gobjc.parents 

You can also write new objects to the database this way:

    person = Rugged::Signature.new('Scott', 'schacon@gmail.com', Time.now)

    gobjc = Rugged::Commit.new(@repo)
    gobjc.message = "my message"
    gobjc.author = person
    gobjc.tree = gobjt
    gobjc.write

### Tag Objects

    gobtg = @repo.lookup(tag_sha)
      gobj  = gobtg.target
      sha   = gobtg.target.oid
      str   = gobtg.target_type # "commit", "tag", "blob"
      str   = gobtg.name        # "v1.0"
      str   = gobtg.message
      prsn  = gobtg.tagger
         str   = prsn[:name]
         str   = prsn[:time]
         str   = prsn[:email]

### Tree Objects

    gobtr = @repo.lookup(tree_sha)
      int   = gobtr.count        # number of tree entries
      ent   = gobtr[0]           # get object of first entry
      ent   = gobtr.get_entry(0) # get object of first entry
          str  = ent[:name]      # "README.txt"
          str  = ent[:type]      # :blob
          sha  = ent[:oid]       # object sha

The tree object is an Enumerable, so you can also do stuff like this:

    gobjr.each { |e| puts e[:oid] }
    gobjr.sort { |a, b| a[:oid] <=> b[:oid] }.map { |e| e[:name] }.join(':')

And there are some Rugged specific methods, too:

    gobjr.each_tree { |entry| puts entry[:name] }  # list subdirs
    gobjr.each_blob { |entry| puts entry[:name] }  # list only files

You can also write trees with the TreeBuilder:

    entry = {:type => :blob,
             :path => "README.txt",
             :oid  => "1385f264afb75a56a5bec74243be9b367ba4ca08",
             :attributes => 33188}

    builder = Rugged::Tree::Builder.new
    builder << entry
    sha = builder.write(@repo)

Commit Walker
-----------------

There is also a Walker class that currently takes a repo object. You can push 
head SHAs onto the walker, then call next to get a list of the reachable commit 
objects, one at a time. You can also hide() commits if you are not interested in
anything beneath them (useful for a `git log master ^origin/master` type deal).

    walker = Rugged::Walker.new(repo)
         walker.sorting(Rugged::SORT_TOPO | Rugged::SORT_REVERSE) # optional
         walker.push(hex_sha_interesting)
         walker.hide(hex_sha_uninteresting)
         walker.each { |c| puts c.inspect } 
         walker.reset


Index/Staging Area
-------------------

We can inspect and manipulate the Git Index as well. To work with the index inside
of an existing repository, instantiate it by using the `Repository.index` method instead
of manually opening the Index by its path.

    index =
    Rugged::Index.new(path)
            index.reload              # re-read the index file from disk
      int = index.count # count of index entries
            index.entries # collection of index entries
            index.each { |i| puts i.inspect }
      ent = index.get_entry(i/path)
            index.remove(i/path)
            index.add(ientry)    # also updates existing entry if there is one
            index.add(path)      # create ientry from file in path, update index
 

Ref Management
--------------

The RefList class allows you to list, create and delete packed and loose refs.

    ref = @repo.head

    ref = Rugged::Reference.lookup(@repo, "refs/heads/master")
      sha = ref.target
      str = ref.type   # "commit"
      str = ref.name   # "refs/heads/master"

    @repo.refs.each do |ref_name|
      ref = Rugged::Reference.lookup(@repo, ref_name)
    end

    ref = Rugged::Reference.create(@repo, "refs/heads/unit_test", "36060c58702ed4c2a40832c51758d5344201d89a")


CONTRIBUTING
==============

Fork libgit2/rugged on GitHub, make it awesomer (preferably in a branch named
for the topic), send a pull request.

INSTALL FOR DEVELOPING
----------------------

First you need to install libgit2:

    $ git clone git://github.com/libgit2/libgit2.git
    $ cd libgit2
    $ mdkir build && cd build
    $ cmake ..
    $ make
    $ make install

Now that those are installed, you can install Rugged:

    $ git clone git://github.com/libgit2/rugged.git
    $ cd rugged
    $ bundle install
    $ rake compile
    $ rake test


AUTHORS 
==============

* Vicent Marti <tanoku@gmail.com>
* Scott Chacon <schacon@gmail.com>


LICENSE
==============

MIT.  See LICENSE file.

