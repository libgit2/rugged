require "test_helper"

class RepositoryIgnoreTest < Rugged::SandboxedTestCase
  def setup
    super

    @repo = sandbox_init "attr"
  end

  def teardown
    @repo.close

    super
  end

  def test_path_ignored
    assert_equal true, @repo.path_ignored?("ign")
    assert_equal false, @repo.path_ignored?("ignore_not")
    assert_equal true, @repo.path_ignored?("dir")
    assert_equal true, @repo.path_ignored?("dir/foo")
    assert_equal true, @repo.path_ignored?("dir/foo/bar")
    assert_equal false, @repo.path_ignored?("foo/dir")
    assert_equal true, @repo.path_ignored?("foo/dir/bar")
    assert_equal false, @repo.path_ignored?("direction")
  end
end

