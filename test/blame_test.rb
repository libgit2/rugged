require "test_helper"

class BlameTest < Rugged::SandboxedTestCase
  def setup
    super
    @repo = sandbox_init("testrepo")
  end

  def teardown
    @repo.close
    super
  end

  def test_blame_simple
    blame = Rugged::Blame.new(@repo, "branch_file.txt")
    assert_equal 2, blame.count
  end
end