require 'test_helper'
require 'base64'

describe Rugged::Repository do
  before do
    @path = test_repo_path("testrepo.git")
    @repo = Rugged::Repository.new(@path)

    @test_content = "my test data\n"
    @test_content_type = 'blob'
  end

  it "last_commit returns the most recent commit" do
    assert @repo.respond_to? :last_commit
    assert "36060c58702ed4c2a40832c51758d5344201d89a", @repo.last_commit.oid
  end

  it "fails to open unexisting repositories" do
    assert_raises IOError, Rugged::OSError do
      Rugged::Repository.new("fakepath/123/")
    end

    assert_raises Rugged::RepositoryError do
      Rugged::Repository.new("/home/")
    end
  end

  it "can tell if an object exists or not" do
    assert @repo.exists?("8496071c1b46c854b31185ea97743be6a8774479")
    assert @repo.exists?("1385f264afb75a56a5bec74243be9b367ba4ca08")
    assert !@repo.exists?("ce08fe4884650f067bd5703b6a59a8b3b3c99a09")
    assert !@repo.exists?("8496071c1c46c854b31185ea97743be6a8774479")
  end

  it "can read an object from the db" do
    rawobj = @repo.read("8496071c1b46c854b31185ea97743be6a8774479")
    assert_match 'tree 181037049a54a1eb5fab404658a3a250b44335d7', rawobj.data
    assert_equal 172, rawobj.len
    assert_equal :commit, rawobj.type
  end

  it "can read an object's headers from the db" do
    hash = @repo.read_header("8496071c1b46c854b31185ea97743be6a8774479")
    assert_equal 172, hash[:len]
    assert_equal :commit, hash[:type]
  end

  it "checks that reading fails on unexisting objects" do
    assert_raises Rugged::OdbError do
      @repo.read("a496071c1b46c854b31185ea97743be6a8774471")
    end
  end

  it "checks that reading headers fails on unexisting objects" do
    assert_raises Rugged::OdbError do
      @repo.read_header("a496071c1b46c854b31185ea97743be6a8774471")
    end
  end

  it "can hash data" do
    oid = Rugged::Repository::hash(@test_content, @test_content_type)
    assert_equal "76b1b55ab653581d6f2c7230d34098e837197674", oid
  end

  it "can write to the db" do
    oid = @repo.write(@test_content, @test_content_type)
    assert_equal "76b1b55ab653581d6f2c7230d34098e837197674", oid
    assert @repo.exists?("76b1b55ab653581d6f2c7230d34098e837197674")

    rm_loose(oid)
  end

  it "can walk in a block" do
    oid = "a4a7dce85cf63874e984719f4fdd239f5145052f"
    list = []
    @repo.walk(oid) { |c| list << c }
    assert list.map {|c| c.oid[0,5] }.join('.'), "a4a7d.c4780.9fd73.4a202.5b5b0.84960"
  end

  it "can walk without a block" do
    commits = @repo.walk('a4a7dce85cf63874e984719f4fdd239f5145052f')

    assert commits.kind_of?(Enumerable)
    assert commits.count > 0
  end

  it "can lookup an object" do
    object = @repo.lookup("8496071c1b46c854b31185ea97743be6a8774479")

    assert object.kind_of?(Rugged::Commit)
  end

  it "can find a ref" do
    ref = @repo.ref('refs/heads/master')

    assert ref.kind_of?(Rugged::Reference)
    assert_equal 'refs/heads/master', ref.name
  end

  it "can return all refs" do
    refs = @repo.refs

    assert_equal 5, refs.length
  end

  it "can return all refs that match" do
    refs = @repo.refs 'refs/heads'

    assert_equal 3, refs.length
  end

  it "can return the names of all refs" do
    refs = @repo.ref_names

    refs.each {|name| assert name.kind_of?(String)}
    assert_equal 5, refs.count
  end

  it "can return all tags" do
    tags = @repo.tags

    assert_equal 2, tags.count
  end

  it "can return all tags that match" do
    tags = @repo.tags 'v0.9'

    assert_equal 1, tags.count
  end

  it "can return all remotes" do
    remotes = @repo.remotes

    assert_equal 1, remotes.count
  end

  it "can lookup head from repo" do
    head = @repo.head
    assert_equal "36060c58702ed4c2a40832c51758d5344201d89a", head.target
    assert_equal :direct, head.type
  end

  it "can access a file" do
    sha = '36060c58702ed4c2a40832c51758d5344201d89a'
    blob = @repo.blob_at(sha, 'new.txt')
    assert_equal "new file\n", blob.content
  end

  it "can be garbage collected" do
    Rugged::Repository.new(@path)
    ObjectSpace.garbage_collect
  end

  it "can enumerate all objects in the odb" do
    assert_equal 34, @repo.each_id.to_a.length
  end

  it "can load alternates" do
    alt_path = File.dirname(__FILE__) + '/fixtures/alternate/objects'
    repo = Rugged::Repository.new(@path, :alternates => [alt_path])
    assert_equal 37, repo.each_id.to_a.length
    assert repo.read('146ae76773c91e3b1d00cf7a338ec55ae58297e2')
  end

  it "can successfully clone a repository" do
    # Remote clone
    tmpdir = Dir.mktmpdir("clone-target-dir")
    begin
      repo = Rugged::Repository.clone_repo("git://github.com/libgit2/libgit2.git", tmpdir, :strategy => [:safe])
      assert_kind_of Rugged::Commit, repo.lookup("1a628100534a315bd00361fc3d32df671923c107")
    ensure
      FileUtils.rm_rf(tmpdir)
    end

    # Bare repository
    tmpdir = Dir.mktmpdir("clone-target-dir")
    begin
      repo = Rugged::Repository.clone_repo_bare("git://github.com/libgit2/libgit2.git", tmpdir)
      assert_kind_of Rugged::Commit, repo.lookup("1a628100534a315bd00361fc3d32df671923c107")
    ensure
      FileUtils.rm_rf(tmpdir)
    end

    # Local clone
    tmpdir = Dir.mktmpdir("clone-target-dir")
    begin
      repo = Rugged::Repository.clone_repo("file://#@path", tmpdir, :strategy => [:safe])
      assert_equal "36060c58702ed4c2a40832c51758d5344201d89a", repo.last_commit.oid
      assert_kind_of Rugged::Commit, repo.lookup("8496071c1b46c854b31185ea97743be6a8774479")

      refs = repo.refs
      assert_equal 6, refs.length
    ensure
      FileUtils.rm_rf(tmpdir)
    end
  end

  it "calls transfer and checkout callbacks on cloning" do
    Dir.mktmpdir("clone-target-dir") do |tmpdir|
      transfer_called = false
      progress_called = false

      pcb = proc{progress_called = true}

      Rugged::Repository.clone_repo("git://github.com/libgit2/libgit2.git", tmpdir, :strategy => [:safe], :progress_cb => pcb) do
        transfer_called = true
      end

      assert(transfer_called, "Didn't call transfer callback")
      assert(progress_called, "Didn't call progress callback")

      conflict_called = false
      ccb = proc{conflict_called = true}
      FileUtils.rm_rf(tmpdir)
      Rugged::Repository.clone_repo("git://github.com/libgit2/libgit2.git", tmpdir, :strategy => [:default], :conflict_cb => ccb)
      assert(conflict_called, "Didn't call conflict callback")
    end
  end

end

describe "Repository checkouts" do

  before do
    @repo_path             = temp_repo("testrepo.git")
    `cd '#@repo_path' && git branch new-file refs/remotes/origin/new-file` # checkout doesn't do tracking (without this, we cannot checkout a remote branch however)
    @repo                  = Rugged::Repository.new(@repo_path)
    @path_to_branched_file = File.join(@repo_path, "another_file.txt") # This file only exists in the new-file branch
    @path_to_other_file    = File.join(@repo_path, "stuff")            # This file doesn't exist in any branch
    @last_master_commit    = Rugged::Commit.lookup(@repo, Rugged::Reference.lookup(@repo, "refs/heads/master").target)
    @last_new_file_commit  = Rugged::Commit.lookup(@repo, Rugged::Reference.lookup(@repo, "refs/heads/new-file").target)
  end

  it "can checkout back and fourth" do
    File.open(@path_to_other_file, "w"){|f| f.write("more stuff")}
    assert !File.file?(@path_to_branched_file)

    @repo.checkout_tree(@last_new_file_commit, :strategy => [:safe, :remove_untracked])
    assert File.file?(@path_to_branched_file)
    assert !File.file?(@path_to_other_file) # :remove_untracked

    @repo.checkout_tree(@last_master_commit, :strategy => [:safe, :remove_untracked])
    assert !File.file?(@path_to_branched_file)

    @repo.checkout_tree(@last_new_file_commit, :strategy => [:update_unmodified])
    #### bug in libgit2 prevents this --> #### assert !File.file?(@path_to_branched_file) # :update_missing in :force ommited
    @repo.checkout_tree(@last_master_commit, :strategy => [:safe, :remove_untracked])

    File.open(@path_to_other_file, "w"){|f| f.write("more stuff")}
    @repo.checkout_tree(@last_new_file_commit, :strategy => [:safe])
    assert File.file?(@path_to_other_file) # :remove_untracked ommited

    File.open(@path_to_branched_file, "w"){|f| f.write("Modified this file.")}
    @repo.checkout_tree(@last_master_commit, :strategy => [:safe])
    assert_equal("Modified this file.", File.read(@path_to_branched_file)) # :update_modified in :force ommited

    assert_nil @repo.checkout_tree(@last_new_file_commit, :strategy => [:force])
  end

  it "can checkout the index" do
    # Read the new-file branch's latest commit's tree into the index, then check
    # it out removing anything not related to that tree.
    assert !File.file?(@path_to_branched_file)
    `cd '#@repo_path' && git read-tree #{@last_new_file_commit.tree_oid}`
    assert_nil @repo.checkout_index(:strategy => [:force])
    assert File.file?(@path_to_branched_file)
  end

  it "can checkout the HEAD" do
    File.open(@path_to_other_file, "w"){|f| f.write("This stuff is to be discarded.")}
    # Now reset the working direcory
    assert_nil @repo.checkout_head(:strategy => [:remove_untracked])
    assert !File.file?(@path_to_other_file)
  end

  it "calls progress and conflict callbacks" do
    progress_called = false
    pcb = proc{progress_called = true}
    @repo.checkout_tree(@last_new_file_commit, :progress_cb => pcb, :strategy => [:safe])
    assert(progress_called, "Didn't call progress callback")

    conflict_called = false
    ccb = proc{conflict_called = true}
    `cd '#@repo_path' && rm -rf *`
    @repo.checkout_tree(@last_master_commit, :conflict_cb => ccb, :strategy => [:default])
    assert(conflict_called, "Didn't call conflict callback")
  end

end
