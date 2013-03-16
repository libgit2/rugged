require 'test_helper'
require 'base64'

class RepositoryTest < Rugged::TestCase
  include Rugged::RepositoryAccess

  def test_last_commit
    assert @repo.respond_to? :last_commit
    assert "36060c58702ed4c2a40832c51758d5344201d89a", @repo.last_commit.oid
  end

  def test_fails_to_open_unexisting_repos
    assert_raises IOError, Rugged::OSError do
      Rugged::Repository.new("fakepath/123/")
    end

    assert_raises Rugged::RepositoryError do
      Rugged::Repository.new("/home/")
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
    refs = @repo.refs 'refs/heads'
    assert_equal 2, refs.length
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
    assert_equal 1, remotes.count
  end

  def test_lookup_head
    head = @repo.head
    assert_equal "36060c58702ed4c2a40832c51758d5344201d89a", head.target
    assert_equal :direct, head.type
  end

  def test_access_a_file
    sha = '36060c58702ed4c2a40832c51758d5344201d89a'
    blob = @repo.blob_at(sha, 'new.txt')
    assert_equal "new file\n", blob.content
  end

  def test_garbage_collection
    Rugged::Repository.new(@path)
    ObjectSpace.garbage_collect
  end

  def test_enumerate_all_objects
    assert_equal 33, @repo.each_id.to_a.length
  end

  def test_loading_alternates
    alt_path = File.dirname(__FILE__) + '/fixtures/alternate/objects'
    repo = Rugged::Repository.new(@path, :alternates => [alt_path])
    assert_equal 36, repo.each_id.to_a.length
    assert repo.read('146ae76773c91e3b1d00cf7a338ec55ae58297e2')
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
end

class RepositoryWriteTest < Rugged::TestCase
  include Rugged::TempRepositoryAccess

  TEST_CONTENT = "my test data\n"
  TEST_CONTENT_TYPE = 'blob'

  def test_can_hash_data
    oid = Rugged::Repository::hash(TEST_CONTENT, TEST_CONTENT_TYPE)
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
    refute repo.bare?
  end

  def test_init_bare_true
    repo = Rugged::Repository.init_at(@tmppath, true)
    assert repo.bare?
  end

  def test_init_bare_truthy
    repo = Rugged::Repository.init_at(@tmppath, :bare)
    assert repo.bare?
  end

  def test_init_non_bare_default
    repo = Rugged::Repository.init_at(@tmppath)
    refute repo.bare?
  end
end
