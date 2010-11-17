require File.dirname(__FILE__) + '/test_helper'

context "Rugged::Commit tests" do
  setup do
    path = File.dirname(__FILE__) + '/fixtures/testrepo.git/'
    @repo = Rugged::Repository.new(path)
  end

  test "can read the commit data" do
    sha = "8496071c1b46c854b31185ea97743be6a8774479"
    obj = @repo.lookup(sha)

    assert_equal obj.sha, sha
	  assert_equal obj.type, "commit"
    assert_equal obj.message, "testing\n"
    assert_equal obj.message_short, "testing"
    assert_equal obj.time.to_i, 1273360386
    c = obj.committer
    assert_equal c['name'], "Scott Chacon"
    assert_equal c['time'].to_i, 1273360386
    assert_equal c['email'], "schacon@gmail.com"
    c = obj.author
    assert_equal c['name'], "Scott Chacon"
    assert_equal c['time'].to_i, 1273360386
    assert_equal c['email'], "schacon@gmail.com"
    assert_equal obj.tree.sha, "181037049a54a1eb5fab404658a3a250b44335d7"
  end
  
  test "can write the commit data" do
    sha = "8496071c1b46c854b31185ea97743be6a8774479"
    obj = @repo.lookup(sha)
    obj.message = 'new message'
    obj.write
  end

end
