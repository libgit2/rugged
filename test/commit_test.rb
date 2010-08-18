require File.dirname(__FILE__) + '/test_helper'

context "Ribbit::Commit tests" do
  setup do
    path = File.dirname(__FILE__) + '/fixtures/testrepo.git/objects'
    @repo = Ribbit::Repository.new(path)
  end

  test "can read the commit data" do
    sha = "8496071c1b46c854b31185ea97743be6a8774479"
    obj = Ribbit::Commit.new(@repo, sha)

    assert_equal obj.sha, sha
    assert_equal obj.message, "testing\n"
    assert_equal obj.message_short, "testing"
  end
end
