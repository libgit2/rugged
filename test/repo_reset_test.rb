class RepositoryResetTest < Rugged::TestCase
  include Rugged::TempRepositoryAccess

  def repo_file_path; File.join('subdir', 'README') end
  def file_path;      File.join(@path, 'subdir', 'README') end

  def test_reset_with_tree_oid
    assert_raises Rugged::ObjectError do
      @repo.reset('688a8f4ef7496901d15322972f96e212a9e466cc', :soft)
    end
  end

  def test_reset_with_rugged_tag
    tag = @repo.lookup('0c37a5391bbff43c37f0d0371823a5509eed5b1d')
    @repo.reset(tag, :soft)
    assert_equal tag.target.oid , @repo.head.target
  end

  def test_reset_with_invalid_mode
    assert_raises ArgumentError do
      @repo.reset('441034f860c1d5d90e4188d11ae0d325176869a8', :tender)
    end
  end

  def test_reset_soft
    original_content = File.open(file_path) { |f| f.read }

    @repo.reset('441034f860c1d5d90e4188d11ae0d325176869a8', :soft)
    assert_equal '441034f860c1d5d90e4188d11ae0d325176869a8', @repo.head.target
    assert_equal [:index_modified], @repo.status(repo_file_path)

    new_content = File.open(file_path) { |f| f.read }

    assert_equal original_content, new_content
  end

  def test_reset_mixed
    original_content = File.open(file_path) { |f| f.read }

    @repo.reset('441034f860c1d5d90e4188d11ae0d325176869a8', :mixed)
    assert_equal [:worktree_modified], @repo.status(repo_file_path)

    new_content = File.open(file_path) { |f| f.read }

    assert_equal original_content, new_content
  end

  def test_reset_hard
    original_content = File.open(file_path) { |f| f.read }

    @repo.reset('441034f860c1d5d90e4188d11ae0d325176869a8', :hard)
    assert_empty @repo.status(repo_file_path)

    new_content = File.open(file_path) { |f| f.read }

    refute_equal original_content, new_content
  end

  def test_reset_hard_removes_fs_entries
    @repo.reset('5b5b025afb0b4c913b4c338a42934a3863bf3644', :hard)
    assert_raises Rugged::InvalidError do
      assert_equal @repo.status('subdir'), [:worktree_new]
    end
  end
end
