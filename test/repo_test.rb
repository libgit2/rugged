require 'test_helper'
require 'base64'

class RepositoryTest < Rugged::SandboxedTestCase
  def setup
    super

    @repo = sandbox_init "testrepo.git"
  end

  def teardown
    @repo.close

    super
  end

  def test_last_commit
    assert @repo.respond_to? :last_commit
    assert "36060c58702ed4c2a40832c51758d5344201d89a", @repo.last_commit.oid
  end

  def test_fails_to_open_unexisting_repos
    assert_raises IOError, Rugged::OSError do
      Rugged::Repository.new("fakepath/123/")
    end

    assert_raises Rugged::RepositoryError do
      Rugged::Repository.new("test")
    end
  end

  def test_can_check_if_objects_exist
    assert @repo.exists?("8496071c1b46c854b31185ea97743be6a8774479")
    assert @repo.exists?("1385f264afb75a56a5bec74243be9b367ba4ca08")
    assert !@repo.exists?("ce08fe4884650f067bd5703b6a59a8b3b3c99a09")
    assert !@repo.exists?("8496071c1c46c854b31185ea97743be6a8774479")
  end

  def test_can_read_a_raw_object
    rawobj = @repo.read("8496071c1b46c854b31185ea97743be6a8774479")
    assert_match 'tree 181037049a54a1eb5fab404658a3a250b44335d7', rawobj.data
    assert_equal 172, rawobj.len
    assert_equal :commit, rawobj.type
  end

  def test_can_read_object_headers
    hash = @repo.read_header("8496071c1b46c854b31185ea97743be6a8774479")
    assert_equal 172, hash[:len]
    assert_equal :commit, hash[:type]
  end

  def test_check_reads_fail_on_missing_objects
    assert_raises Rugged::OdbError do
      @repo.read("a496071c1b46c854b31185ea97743be6a8774471")
    end
  end

  def test_check_read_headers_fail_on_missing_objects
    assert_raises Rugged::OdbError do
      @repo.read_header("a496071c1b46c854b31185ea97743be6a8774471")
    end
  end

  def test_walking_with_block
    oid = "a4a7dce85cf63874e984719f4fdd239f5145052f"
    list = []
    @repo.walk(oid) { |c| list << c }
    assert list.map {|c| c.oid[0,5] }.join('.'), "a4a7d.c4780.9fd73.4a202.5b5b0.84960"
  end

  def test_walking_without_block
    commits = @repo.walk('a4a7dce85cf63874e984719f4fdd239f5145052f')

    assert commits.kind_of?(Enumerable)
    assert commits.count > 0
  end

  def test_lookup_object
    object = @repo.lookup("8496071c1b46c854b31185ea97743be6a8774479")
    assert object.kind_of?(Rugged::Commit)
  end

  def test_find_reference
    ref = @repo.ref('refs/heads/master')

    assert ref.kind_of?(Rugged::Reference)
    assert_equal 'refs/heads/master', ref.name
  end

  def test_match_all_refs
    refs = @repo.refs 'refs/heads/*'
    assert_equal 2, refs.count
  end

  def test_return_all_ref_names
    refs = @repo.ref_names
    refs.each {|name| assert name.kind_of?(String)}
    assert_equal 5, refs.count
  end

  def test_return_all_tags
    tags = @repo.tags
    assert_equal 2, tags.count
  end

  def test_return_matching_tags
    tags = @repo.tags 'v0.9'
    assert_equal 1, tags.count
  end

  def test_return_all_remotes
    remotes = @repo.remotes
    assert_equal 2, remotes.count
  end

  def test_lookup_head
    head = @repo.head
    assert_equal "refs/heads/master", head.name
    assert_equal "36060c58702ed4c2a40832c51758d5344201d89a", head.target
    assert_equal :direct, head.type
  end

  def test_set_head_ref
    @repo.head = "refs/heads/packed"
    assert_equal "refs/heads/packed", @repo.head.name
  end

  def test_set_head_invalid
    assert_raises Rugged::ReferenceError do
      @repo.head = "36060c58702ed4c2a40832c51758d5344201d89a"
    end
  end

  def test_access_a_file
    sha = '36060c58702ed4c2a40832c51758d5344201d89a'
    blob = @repo.blob_at(sha, 'new.txt')
    assert_equal "new file\n", blob.content
  end

  def test_access_a_missing_file
    sha = '36060c58702ed4c2a40832c51758d5344201d89a'
    blob = @repo.blob_at(sha, 'file-not-found.txt')
    assert_nil blob
  end

  def test_garbage_collection
    Rugged::Repository.new(@repo.path)
    ObjectSpace.garbage_collect
  end

  def test_enumerate_all_objects
    assert_equal 37, @repo.each_id.to_a.length
  end

  def test_loading_alternates
    alt_path = File.dirname(__FILE__) + '/fixtures/alternate/objects'
    repo = Rugged::Repository.new(@repo.path, :alternates => [alt_path])
    begin
      assert_equal 40, repo.each_id.to_a.length
      assert repo.read('146ae76773c91e3b1d00cf7a338ec55ae58297e2')
    ensure
      repo.close
    end
  end

  def test_alternates_with_invalid_path_type
    assert_raises TypeError do
      Rugged::Repository.new(@repo.path, :alternates => [:invalid_input])
    end
  end

  def test_find_merge_base_between_oids
    commit1 = 'a4a7dce85cf63874e984719f4fdd239f5145052f'
    commit2 = '36060c58702ed4c2a40832c51758d5344201d89a'
    base    = '5b5b025afb0b4c913b4c338a42934a3863bf3644'
    assert_equal base, @repo.merge_base(commit1, commit2)
  end

  def test_find_merge_base_between_commits
    commit1 = @repo.lookup('a4a7dce85cf63874e984719f4fdd239f5145052f')
    commit2 = @repo.lookup('36060c58702ed4c2a40832c51758d5344201d89a')
    base    = '5b5b025afb0b4c913b4c338a42934a3863bf3644'
    assert_equal base, @repo.merge_base(commit1, commit2)
  end

  def test_find_merge_base_between_ref_and_oid
    commit1 = 'a4a7dce85cf63874e984719f4fdd239f5145052f'
    commit2 = "refs/heads/master"
    base    = '5b5b025afb0b4c913b4c338a42934a3863bf3644'
    assert_equal base, @repo.merge_base(commit1, commit2)
  end

  def test_find_merge_base_between_many
    commit1 = 'a4a7dce85cf63874e984719f4fdd239f5145052f'
    commit2 = "refs/heads/packed"
    commit3 = @repo.lookup('36060c58702ed4c2a40832c51758d5344201d89a')

    base    = '5b5b025afb0b4c913b4c338a42934a3863bf3644'
    assert_equal base, @repo.merge_base(commit1, commit2, commit3)
  end

  def test_ahead_behind_with_oids
    ahead, behind = @repo.ahead_behind(
      'a4a7dce85cf63874e984719f4fdd239f5145052f',
      '36060c58702ed4c2a40832c51758d5344201d89a'
    )
    assert_equal 1, ahead
    assert_equal 4, behind
  end

  def test_ahead_behind_with_commits
    ahead, behind = @repo.ahead_behind(
      @repo.lookup('a4a7dce85cf63874e984719f4fdd239f5145052f'),
      @repo.lookup('36060c58702ed4c2a40832c51758d5344201d89a')
    )
    assert_equal 1, ahead
    assert_equal 4, behind
  end
end

class ShallowRepositoryTest < Rugged::SandboxedTestCase
  def setup
    super

    @repo = sandbox_init("testrepo.git")
    @shallow_repo = sandbox_init("shallow.git")
  end

  def teardown
    @repo.close
    @shallow_repo.close

    super
  end

  def test_is_shallow
    refute @repo.shallow?
    assert @shallow_repo.shallow?
  end
end

class RepositoryWriteTest < Rugged::TestCase
  include Rugged::TempRepositoryAccess

  TEST_CONTENT = "my test data\n"
  TEST_CONTENT_TYPE = 'blob'

  def test_can_hash_data
    oid = Rugged::Repository.hash_data(TEST_CONTENT, TEST_CONTENT_TYPE)
    assert_equal "76b1b55ab653581d6f2c7230d34098e837197674", oid
  end

  def test_write_to_odb
    oid = @repo.write(TEST_CONTENT, TEST_CONTENT_TYPE)
    assert_equal "76b1b55ab653581d6f2c7230d34098e837197674", oid
    assert @repo.exists?("76b1b55ab653581d6f2c7230d34098e837197674")
  end

  def test_no_merge_base_between_unrelated_branches
    info = @repo.rev_parse('HEAD').to_hash
    baseless = Rugged::Commit.create(@repo, info.merge(:parents => []))
    assert_nil @repo.merge_base('HEAD', baseless)
  end

  def test_default_signature
    name = 'Rugged User'
    email = 'rugged@example.com'
    @repo.config['user.name'] = name
    @repo.config['user.email'] = email
    assert_equal name, @repo.default_signature[:name]
    assert_equal email, @repo.default_signature[:email]
  end
end

class RepositoryInitTest < Rugged::TestCase
  def setup
    @tmppath = Dir.mktmpdir
  end

  def teardown
    FileUtils.remove_entry_secure(@tmppath)
  end

  def test_init_bare_false
    repo = Rugged::Repository.init_at(@tmppath, false)
    begin
      refute repo.bare?
    ensure
      repo.close
    end
  end

  def test_init_bare_true
    repo = Rugged::Repository.init_at(@tmppath, true)
    begin
      assert repo.bare?
    ensure
      repo.close
    end
  end

  def test_init_bare_truthy
    repo = Rugged::Repository.init_at(@tmppath, :bare)
    begin
      assert repo.bare?
    ensure
      repo.close
    end
  end

  def test_init_non_bare_default
    repo = Rugged::Repository.init_at(@tmppath)
    begin
      refute repo.bare?
    ensure
      repo.close
    end
  end
end

class RepositoryCloneTest < Rugged::TestCase
  def setup
    @tmppath = Dir.mktmpdir
    @source_path = File.join(Rugged::TestCase::TEST_DIR, 'fixtures', 'testrepo.git')
  end

  def teardown
    FileUtils.remove_entry_secure(@tmppath)
  end

  def test_clone
    repo = Rugged::Repository.clone_at(@source_path, @tmppath)
    begin
      assert_equal "hey", File.read(File.join(@tmppath, "README")).chomp
      assert_equal "36060c58702ed4c2a40832c51758d5344201d89a", repo.head.target
      assert_equal "36060c58702ed4c2a40832c51758d5344201d89a", repo.ref("refs/heads/master").target
      assert_equal "36060c58702ed4c2a40832c51758d5344201d89a", repo.ref("refs/remotes/origin/master").target
      assert_equal "41bc8c69075bbdb46c5c6f0566cc8cc5b46e8bd9", repo.ref("refs/remotes/origin/packed").target
    ensure
      repo.close
    end
  end

  def test_clone_bare
    repo = Rugged::Repository.clone_at(@source_path, @tmppath, :bare => true)
    begin
      assert repo.bare?
    ensure
      repo.close
    end
  end

  def test_clone_with_progress
    total_objects = indexed_objects = received_objects = received_bytes = nil
    callsback = 0
    repo = Rugged::Repository.clone_at(@source_path, @tmppath,
      :progress => lambda { |*args| callsback += 1 ; total_objects, indexed_objects, received_objects, received_bytes = args })
    repo.close
    assert_equal 22,   callsback
    assert_equal 19,   total_objects
    assert_equal 19,   indexed_objects
    assert_equal 19,   received_objects
    assert_equal 1563, received_bytes
  end

  def test_clone_quits_on_error
    begin
      Rugged::Repository.clone_at(@source_path, @tmppath, :progress => lambda { |*_| raise 'boom' })
    rescue => e
      assert_equal 'boom', e.message
    end
    assert_no_dotgit_dir(@tmppath)
  end

  def test_clone_with_bad_progress_callback
    assert_raises ArgumentError do
      Rugged::Repository.clone_at(@source_path, @tmppath, :progress => Object.new)
    end
    assert_no_dotgit_dir(@tmppath)
  end

  def assert_no_dotgit_dir(path)
    assert_equal [], Dir[File.join(path, ".git/**")], "new repository's .git dir should not exist"
  end
end

class RepositoryNamespaceTest < Rugged::SandboxedTestCase
  def setup
    super

    @repo = sandbox_init("testrepo.git")
  end

  def teardown
    @repo.close

    super
  end

  def test_no_namespace
    assert_nil @repo.namespace
  end

  def test_changing_namespace
    @repo.namespace = "foo"
    assert_equal "foo", @repo.namespace

    @repo.namespace = "bar"
    assert_equal "bar", @repo.namespace

    @repo.namespace = "foo/bar"
    assert_equal "foo/bar", @repo.namespace

    @repo.namespace = nil
    assert_equal nil, @repo.namespace
  end

  def test_refs_in_namespaces
    @repo.namespace = "foo"
    assert_equal [], @repo.refs.to_a
  end
end

class RepositoryPushTest < Rugged::SandboxedTestCase
  def setup
    super
    @remote_repo = sandbox_init("testrepo.git")
    # We can only push to bare repos
    @remote_repo.config['core.bare'] = 'true'

    @repo = sandbox_clone("testrepo.git", "testrepo")
    Rugged::Reference.create(@repo,
      "refs/heads/unit_test",
      "8496071c1b46c854b31185ea97743be6a8774479")
  end

  def teardown
    @repo.close
    @remote_repo.close

    super
  end

  def test_push_single_ref
    result = @repo.push("origin", ["refs/heads/master", "refs/heads/master:refs/heads/foobar", "refs/heads/unit_test"])
    assert_equal({}, result)

    assert_equal "36060c58702ed4c2a40832c51758d5344201d89a", @remote_repo.ref("refs/heads/foobar").target
    assert_equal "8496071c1b46c854b31185ea97743be6a8774479", @remote_repo.ref("refs/heads/unit_test").target
  end

  def test_push_to_remote_instance
    origin = Rugged::Remote.lookup(@repo, "origin")
    result = @repo.push(origin, ["refs/heads/master"])
    assert_equal({}, result)
  end

  def test_push_to_non_bare_raise_error
    @remote_repo.config['core.bare'] = 'false'

    assert_raises Rugged::Error do
      @repo.push("origin", ["refs/heads/master"])
    end
  end

  def test_push_non_forward_raise_error
    assert_raises Rugged::Error do
      @repo.push("origin", ["refs/heads/unit_test:refs/heads/master"])
    end

    assert_equal "36060c58702ed4c2a40832c51758d5344201d89a", @remote_repo.ref("refs/heads/master").target
  end

  def test_push_non_forward_forced_raise_no_error
    result = @repo.push("origin", ["+refs/heads/unit_test:refs/heads/master"])
    assert_equal({}, result)

    assert_equal "8496071c1b46c854b31185ea97743be6a8774479", @remote_repo.ref("refs/heads/master").target
  end
end

class RepositoryCheckoutTest < Rugged::SandboxedTestCase
  def setup
    super

    @repo = sandbox_init("testrepo")
    @clone = sandbox_clone("testrepo", "cloned_testrepo")

    _bare = sandbox_init("testrepo.git")
    @bare = Rugged::Repository.bare(_bare.path)
    _bare.close
  end

  def teardown
    @bare.close
    @clone.close
    @repo.close

    super
  end

  def test_checkout_tree_with_commit
    @repo.checkout_tree(@repo.rev_parse("refs/heads/dir"), :strategy => :force)
    @repo.head = "refs/heads/dir"

    assert File.exists?(File.join(@repo.workdir, "README"))
    assert File.exists?(File.join(@repo.workdir, "branch_file.txt"))
    assert File.exists?(File.join(@repo.workdir, "new.txt"))
    assert File.exists?(File.join(@repo.workdir, "a/b.txt"))

    refute File.exists?(File.join(@repo.workdir, "ab"))

    @repo.checkout_tree(@repo.rev_parse("refs/heads/subtrees"), :strategy => :safe)
    @repo.head = "refs/heads/subtrees"

    assert File.exists?(File.join(@repo.workdir, "README"))
    assert File.exists?(File.join(@repo.workdir, "branch_file.txt"))
    assert File.exists?(File.join(@repo.workdir, "new.txt"))
    assert File.exists?(File.join(@repo.workdir, "ab/4.txt"))
    assert File.exists?(File.join(@repo.workdir, "ab/c/3.txt"))
    assert File.exists?(File.join(@repo.workdir, "ab/de/2.txt"))
    assert File.exists?(File.join(@repo.workdir, "ab/de/fgh/1.txt"))

    refute File.exists?(File.join(@repo.workdir, "a"))
  end

  def test_checkout_with_revspec_string
    @repo.checkout_tree("refs/heads/dir", :strategy => :force)
    @repo.head = "refs/heads/dir"

    assert File.exists?(File.join(@repo.workdir, "README"))
    assert File.exists?(File.join(@repo.workdir, "branch_file.txt"))
    assert File.exists?(File.join(@repo.workdir, "new.txt"))
    assert File.exists?(File.join(@repo.workdir, "a/b.txt"))

    refute File.exists?(File.join(@repo.workdir, "ab"))
  end

  def test_checkout_tree_raises_errors_in_notify_cb
    exception = assert_raises RuntimeError do
      @repo.checkout_tree(@repo.rev_parse("refs/heads/dir"), :strategy => :force,
        :notify_flags => :all,
        :notify => lambda { |*args| raise "fail" })
    end

    assert_equal exception.message, "fail"
  end

  def test_checkout_tree_raises_errors_in_progress_cb
    exception = assert_raises RuntimeError do
      @repo.checkout_tree(@repo.rev_parse("refs/heads/dir"), :strategy => :force,
        :progress => lambda { |*args| raise "fail" })
    end

    assert_equal exception.message, "fail"
  end

  def test_checkout_tree_subdirectory
    refute File.exists?(File.join(@repo.workdir, "ab"))

    @repo.checkout_tree(@repo.rev_parse("refs/heads/subtrees"), :strategy => :safe, :paths => "ab/de/")

    assert File.exists?(File.join(@repo.workdir, "ab"))
    assert File.exists?(File.join(@repo.workdir, "ab/de/2.txt"))
    assert File.exists?(File.join(@repo.workdir, "ab/de/fgh/1.txt"))
  end

  def test_checkout_tree_subtree_directory
    refute File.exists?(File.join(@repo.workdir, "de"))

    @repo.checkout_tree(@repo.rev_parse("refs/heads/subtrees:ab"), :strategy => :safe, :paths => "de/")

    assert File.exists?(File.join(@repo.workdir, "de"))
    assert File.exists?(File.join(@repo.workdir, "de/2.txt"))
    assert File.exists?(File.join(@repo.workdir, "de/fgh/1.txt"))
  end

  def test_checkout_tree_raises_with_bare_repo
    assert_raises Rugged::RepositoryError do
      @bare.checkout_tree("HEAD", :strategy => :safe_create)
    end
  end

  def test_checkout_tree_works_with_bare_repo_and_target_directory
    Dir.mktmpdir("alternative") do |dir|
      @bare.checkout_tree("HEAD", :strategy => :safe_create, :target_directory => dir)

      assert File.exists?(File.join(dir, "README"))
      assert File.exists?(File.join(dir, "new.txt"))
      assert File.exists?(File.join(dir, "subdir"))
    end
  end

  def test_checkout_with_branch_updates_HEAD
    @repo.checkout("dir", :strategy => :force)
    assert_equal "refs/heads/dir", @repo.head.name
  end

  def test_checkout_with_HEAD
    @repo.checkout("dir", :strategy => :force)
    File.unlink(File.join(@repo.workdir, "README"))

    @repo.checkout("HEAD", :strategy => :force)

    assert File.exists?(File.join(@repo.workdir, "README"))
    assert_equal "refs/heads/dir", @repo.head.name
  end

  def test_checkout_with_commit_detaches_HEAD
    @repo.checkout(@repo.rev_parse_oid("refs/heads/dir"), :strategy => :force)

    assert @repo.head_detached?
    assert_equal @repo.rev_parse_oid("refs/heads/dir"), @repo.head.target
  end

  def test_checkout_with_remote_branch_detaches_HEAD
    @clone.checkout("origin/dir", :strategy => :force)

    assert @clone.head_detached?
    assert_equal @clone.rev_parse_oid("refs/remotes/origin/dir"), @clone.head.target
  end
end
