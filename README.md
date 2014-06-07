# Rugged
**libgit2 bindings in Ruby**

Rugged is a library for accessing [libgit2](https://github.com/libgit2/libgit2) in Ruby. It gives you the speed and
portability of libgit2 with the beauty of the Ruby language.

### libgit2

libgit2 is a pure C implementation of the Git core methods. It's designed to be
fast and portable. For more information about libgit2,
[check out libgit2's website](http://libgit2.github.com) or browse the
[libgit2 organization](https://github.com/libgit2) on GitHub.

## Install

Rugged is a self-contained gem. You can install it by running:

    $ gem install rugged

You need to have CMake installed on your system to be able to build the included version of `libgit2`. If you want to build Rugged with HTTPS and SSH support, check out the list of optional [libgit2 dependencies](https://github.com/libgit2/libgit2#optional-dependencies).

If you're using bundler and want to bundle `libgit2` with Rugged, you can use the `:submodules` option:

```ruby
gem 'rugged', git: 'git://github.com/libgit2/rugged.git', branch: 'development', submodules: true
```

To load Rugged, you'll usually want to add something like this:

```ruby
require 'rugged'
```

## Usage

Rugged gives you access to the many parts of a Git repository. You can read and
write objects, walk a tree, access the staging area, and lots more. Let's look
at each area individually.

### Repositories

#### Instantiation

The repository is naturally central to Git. Rugged has a `Repository` class that
you can instantiate with a path to open an existing repository :

```ruby
repo = Rugged::Repository.new('path/to/my/repository')
# => #<Rugged::Repository:2228536260 {path: "path/to/my/repository/.git/"}>
```

You can create a new repository with `init_at`. Add a second parameter `:bare` to make a bare repository:

```ruby
Rugged::Repository.init_at('.', :bare)
```

You can also let Rugged discover the path to the .git directory if you give it a
subdirectory.

```ruby
Rugged::Repository.discover("/Users/me/projects/repo/lib/subdir/")
# => "/Users/me/projects/repo/.git/"
```

Once your Repository instantiated (in the following examples, as `repo`), you
can access or modify it.

#### Accessing a Repository

```ruby
# Does the given SHA1 exist in this repository?
repo.exists?('07b44cbda23b726e5d54e2ef383495922c024202')
# => true

# Boolean repository state values:
repo.bare?
# => false
repo.empty?
# => true
repo.head_orphan?
# => false
repo.head_detached?
# => false

# Path accessors
repo.path
# => "path/to/my/repository/.git/"
repo.workdir
# => "path/to/my/repository/"

# The HEAD of the repository.
ref = repo.head
# => #<Rugged::Reference:2228467240 {name: "refs/heads/master", target: "07b44cbda23b726e5d54e2ef383495922c024202"}>

# From the returned ref, you can also access the `name` and `target`:
ref.name
# => "refs/heads/master"
ref.target
# => "07b44cbda23b726e5d54e2ef383495922c024202"

# Reading an object
object = repo.read('a0ae5566e3c8a3bddffab21022056f0b5e03ef07')
# => #<Rugged::OdbObject:0x109a64780>
object.len
# => 237
object.data
# => "tree 76f23f186076fc291742816721ea8c3e95567241\nparent 8e3c5c52b8f29da0adc7e8be8a037cbeaea6de6b\nauthor Vicent Mart\303\255 <tanoku@gmail.com> 1333859005 +0200\ncommitter Vicent Mart\303\255 <tanoku@gmail.com> 1333859005 +0200\n\nAdd `Repository#blob_at`\n"
object.type
# => :commit
```

#### Writing to a Repository

There's a few ways to write to a repository. To write directly from your
instantiated repository object:

```ruby
sha = repo.write(content, type)
```

You can also use the `Commit` object directly to craft a commit; this is a bit
more high-level, so it may be preferable:

```ruby
oid = repo.write("This is a blob.", :blob)
index = repo.index
index.read_tree(repo.head.target.tree)
index.add(:path => "README.md", :oid => oid, :mode => 0100644)

options = {}
options[:tree] = index.write_tree(repo)

options[:author] = { :email => "testuser@github.com", :name => 'Test Author', :time => Time.now }
options[:committer] = { :email => "testuser@github.com", :name => 'Test Author', :time => Time.now }
options[:message] ||= "Making a commit via Rugged!"
options[:parents] = repo.empty? ? [] : [ repo.head.target ].compact
options[:update_ref] = 'HEAD'

Rugged::Commit.create(repo, options)
```

---

### Objects

`Object` is the main object class - it shouldn't be created directly, but all of
these methods should be useful in their derived classes.

```ruby
obj = repo.lookup(sha)
obj.oid  # object sha
obj.type # One of :commit, :tree, :blob or :tag

robj = obj.read_raw
str  = robj.data
int  = robj.len
```

There are four base object types in Git: **blobs**, **commits**, **tags**, and
**trees**. Each of these object types have a corresponding class within Rugged.

### Commit Objects

```ruby
commit = repo.lookup('a0ae5566e3c8a3bddffab21022056f0b5e03ef07')
# => #<Rugged::Commit:2245304380>

commit.message
# => "Add `Repository#blob_at`\n"

commit.time
# => Sat Apr 07 21:23:25 -0700 2012

commit.author
# => {:email=>"tanoku@gmail.com", :name=>"Vicent Mart\303\255", :time=>Sun Apr 08 04:23:25 UTC 2012}

commit.tree
# => #<Rugged::Tree:2245269740>

commit.parents
# => [#<Rugged::Commit:2245264600 {message: "Merge pull request #47 from isaac/remotes\n\nAdd Rugged::Repository#remotes", tree: #<Rugged::Tree:2245264240 {oid: 6a2aee58a41fa007d07aa55565e2231f9b39b4a9}>]
```

You can also write new objects to the database this way:

```ruby
author = {:email=>"tanoku@gmail.com", :time=>Time.now, :name=>"Vicent Mart\303\255"}

Rugged::Commit.create(r,
	:author => author,
	:message => "Hello world\n\n",
	:committer => author,
	:parents => ["2cb831a8aea28b2c1b9c63385585b864e4d3bad1"],
	:tree => some_tree,
	:update_ref => "HEAD") #=> "f148106ca58764adc93ad4e2d6b1d168422b9796"
```

### Tag Objects

```ruby
tag  = repo.lookup(tag_sha)

object = tag.target
sha    = tag.target.oid
str    = tag.target_type # :commit, :tag, :blob
str    = tag.name        # "v1.0"
str    = tag.message
person = tag.tagger
```

### Tree Objects

```ruby
tree = repo.lookup('779fbb1e17e666832773a9825875300ea736c2da').tree
# => #<Rugged::Tree:2245194360>

# number of tree entries
tree.count

tree[0]           # or...
tree.first        # or...
tree.get_entry(0)
# => {:type=>:blob, :oid=>"99e7edb53db9355f10c6f2dfaa5a183f205d93bf", :filemode=>33188, :name=>".gitignore"}
```

The tree object is an Enumerable, so you can also do stuff like this:

```ruby
tree.each { |e| puts e[:oid] }
tree.sort { |a, b| a[:oid] <=> b[:oid] }.map { |e| e[:name] }.join(':')
```

And there are some Rugged-specific methods, too:

```ruby
tree.each_tree { |entry| puts entry[:name] }  # list subdirs
tree.each_blob { |entry| puts entry[:name] }  # list only files
```

You can also write trees with the `TreeBuilder`:

```ruby
oid = repo.write("This is a blob.", :blob)
builder = Rugged::Tree::Builder.new
builder << { :type => :blob, :name => "README.md", :oid => oid, :filemode => 0100644 }

options = {}
options[:tree] = builder.write(repo)

options[:author] = { :email => "testuser@github.com", :name => 'Test Author', :time => Time.now }
options[:committer] = { :email => "testuser@github.com", :name => 'Test Author', :time => Time.now }
options[:message] ||= "Making a commit via Rugged!"
options[:parents] = repo.empty? ? [] : [ repo.head.target ].compact
options[:update_ref] = 'HEAD'

Rugged::Commit.create(repo, options)
```

### Blob Objects

Blob objects represent the data in the files of a Tree Object.

```ruby
blob = repo.lookup('e1253910439ea902cf49be8a9f02f3c08d89ac73')
blob.content # => Gives you the content of the blob.
```

#### Streaming Blob Objects

There is currently no way to stream data from a blob, because `libgit2` itself does not (yet) support
streaming blobs out of the git object database. While there are hooks and interfaces for supporting it,
the default file system backend always loads the entire blob contents into memory. 

If you need to access a Blob object through an IO-like API, you can wrap it with the `StringIO` class.
Note that the only advantage here is a stream-compatible interface, the complete blob object will still
be loaded into memory. Below is an example for streaming a Blob using the Sinatra framework:

```ruby
# Sinatra endpoint
get "/blobs/:sha" do
  repo = Rugged::Repository.new(my_repo_path)
  blob = repo.lookup params[:sha]

  headers({
    "Vary" => "Accept",
    "Connection" => "keep-alive",
    "Transfer-Encoding" => "chunked",
    "Content-Type" => "application/octet-stream",
  })

  stream do |out|
    StringIO.new(blob.content).each(8000) do |chunk|
      out << chunk
    end
  end
end
```

---

### Commit Walker

`Rugged::Walker` is a class designed to help you traverse a set of commits over
a repository.

You first push head SHAs onto the walker, and then call next to get a list of
the reachable commit objects one at a time. You can also `hide()` commits if you
are not interested in anything beneath them (useful in situations like when
you're running something like `git log master ^origin/master`).

```ruby
walker = Rugged::Walker.new(repo)
walker.sorting(Rugged::SORT_TOPO | Rugged::SORT_REVERSE) # optional
walker.push(hex_sha_interesting)
walker.hide(hex_sha_uninteresting)
walker.each { |c| puts c.inspect }
walker.reset
```

---

### Index ("staging") area

We can inspect and manipulate the Git Index as well. To work with the index
inside an existing repository, instantiate it by using the `Repository.index`
method instead of manually opening the Index by its path.

```ruby
index = Rugged::Index.new(path)

# Re-read the index file from disk.
index.reload

# Count up index entries.
count = index.count

# The collection of index entries.
index.entries

# Iterating over index entries.
index.each { |i| puts i.inspect }

# Get a particular entry in the index.
index[path]

# Unstage.
index.remove(path)

# Stage. Also updates existing entry if there is one.
index.add(ientry)

# Stage. Create ientry from file in path, updates the index.
index.add(path)
```

---

### Refs

You can access references through the `Rugged::ReferenceCollection` object returned by `Repository#references`.

```ruby
ref = repo.references["refs/heads/master"]

sha = ref.target_id
str = ref.type   # :direct
str = ref.name   # "refs/heads/master"
```

You can also easily iterate over all references:

```ruby
repo.references.each do |ref|
  puts ref.name
end
```

Or only over references that match the given pattern (glob):

```ruby
repo.references.each("refs/tags/*") do |ref|
  puts ref.name
end
```

It is also easy to create, update, rename or delete a reference:

```ruby
ref = repo.references.create("refs/heads/unit_test", some_commit_sha)

repo.references.update(ref, new_sha) # or...
repo.references.update("refs/heads/unit_test", new_sha)

repo.references.rename(ref, "refs/heads/blead") # or...
repo.references.rename("refs/heads/unit_test", "refs/heads/blead")

repo.references.delete(ref) # or...
repo.references.delete("refs/heads/unit_test") # or...
```

Finally, you can access the reflog for any branch:

```ruby
ref = repo.references["refs/heads/master"]
entry = ref.log.first
sha   = entry[:id_old]
sha   = entry[:id_new]
str   = entry[:message]
prsn  = entry[:committer]
```

---

### Branches

The `Rugged::BranchCollection` object returned by `Repository#branches` will help
you with all of your branch-related needs.

Iterate over all branches:

```ruby
repo.branches.each_name().sort
# => ["master", "origin/HEAD", "origin/master", "origin/packed"]

repo.branches.each_name(:local).sort
# => ["master"]

repo.branches.each_name(:remote).sort
# => ["origin/HEAD", "origin/master", "origin/packed"]
```

Look up branches and get attributes:

```ruby
branch = repo.branches["master"]
branch.name # => 'master'
branch.canonical_name # => 'refs/heads/master'
```

Look up the id for the target of a branch:

```ruby
repo.branches["master"].target_id
# => "36060c58702ed4c2a40832c51758d5344201d89a"
```

Creation and deletion:

```ruby
branch = repo.branches.create("test_branch", "HEAD")

repo.branches.rename("test_branch", "new_branch") # or...
repo.branches.rename("refs/heads/test_branch", "new_branch") # or...
repo.branches.rename(ref, "new_branch") # or...

repo.branches.delete("test_branch") # or...
repo.branches.delete("refs/heads/test_branch") # or...
repo.branches.delete(ref) # or...
```

---

### Config files

It's also easy to read and manipulate the Git config file data with Rugged.

```ruby
# Read values
repo.config['core.bare']

# Set values
repo.config['user.name'] = true

# Delete values
repo.config.delete('user.name')
```

---

### General methods

Rugged also includes a general library for handling basic Git operations. One of
these is converting a raw sha (20 bytes) into a readable hex sha (40
characters).

```ruby
Rugged.hex_to_raw('bfde59cdd0dfac1d892814f66a95641abd8a1faf')
# => "\277\336Y\315\320\337\254\035\211(\024\366j\225d\032\275\212\037\257"

Rugged.raw_to_hex("\277\336Y\315\320\337\254\035\211(\024\366j\225d\032\275\212\037\257")
=> "bfde59cdd0dfac1d892814f66a95641abd8a1faf"
```

---

## Contributing

Fork libgit2/rugged on GitHub, make it awesomer (preferably in a branch named
for the topic), send a pull request.


## Development

Simply clone and install:

    $ git clone https://github.com/libgit2/rugged.git
    $ cd rugged
    $ bundle install
    $ rake compile
    $ rake test

## Support

We encourage you to use StackOverflow for any questions or concerns regarding Rugged. Please tag your questions with the [rugged](http://stackoverflow.com/questions/tagged/rugged) keyword.

For bug reports, please open a ticket on the GitHub [issue tracker](https://github.com/libgit2/rugged/issues).

## Authors

* Vicent Marti <tanoku@gmail.com>
* Scott Chacon <schacon@gmail.com>
* Arthur Schreiber <schreiber.arthur@gmail.com>


## License

MIT. See LICENSE file.
