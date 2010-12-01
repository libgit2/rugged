require File.expand_path "../test_helper", __FILE__

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
    assert_equal c.name, "Scott Chacon"
    assert_equal c.time.to_i, 1273360386
    assert_equal c.email, "schacon@gmail.com"
    c = obj.author
    assert_equal c.name, "Scott Chacon"
    assert_equal c.time.to_i, 1273360386
    assert_equal c.email, "schacon@gmail.com"
    assert_equal obj.tree.sha, "181037049a54a1eb5fab404658a3a250b44335d7"
    assert_equal [], obj.parents
  end
  
  test "can have multiple parents" do
    sha = "a4a7dce85cf63874e984719f4fdd239f5145052f"
    obj = @repo.lookup(sha)
    parents = obj.parents.map {|c| c.sha }
    assert parents.include?("9fd738e8f7967c078dceed8190330fc8648ee56a")
    assert parents.include?("c47800c7266a2be04c571c04d5a6614691ea99bd")
  end
  
  test "can write the commit data" do
    sha = "8496071c1b46c854b31185ea97743be6a8774479"
    obj = @repo.lookup(sha)
    obj.message = 'new message'
    obj.write
  end

  test "can write new commit data" do
    tsha = "c4dc1555e4d4fa0e0c9c3fc46734c7c35b3ce90b"
    tree = @repo.lookup(tsha)

    obj = Rugged::Commit.new(@repo)
    person = Rugged::Person.new('Scott', 'schacon@gmail.com', Time.now)

    obj.message = 'new message'
    obj.author = person
    obj.committer = person
    obj.tree = tree
    obj.write
  end

end
