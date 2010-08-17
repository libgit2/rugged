require File.dirname(__FILE__) + '/test_helper'

context "Ribbit::Object tests" do
  setup do
    path = File.dirname(__FILE__) + '/fixtures/testrepo.git/objects'
    @repo = Ribbit::Repository.new(path)
  end

  test "read fails if the sha does not exist" do
    sha = "9496071c1b46c854b31185ea97743be6a8774479"
    obj = Ribbit::Object.new(@repo, sha)
    assert_equal obj.size, nil
    assert_equal obj.read, false
    assert_equal obj.size, nil
  end

  test "can read the sha, data, type and size" do
    sha = "8496071c1b46c854b31185ea97743be6a8774479"
    obj = Ribbit::Object.new(@repo, sha)
    assert_equal obj.size, nil
    assert_equal obj.read, true
    assert_equal obj.sha, sha
    assert_equal obj.object_type, "commit"
    assert_equal obj.size, 172
    assert_equal obj.data.split("\n")[0], "tree 181037049a54a1eb5fab404658a3a250b44335d7"
  end
end
