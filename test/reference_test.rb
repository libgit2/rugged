require File.expand_path "../test_helper", __FILE__

context "Rugged::Reference stuff" do
  setup do
    @path = File.dirname(__FILE__) + '/fixtures/testrepo.git'
    @repo = Rugged::Repository.new(@path)
  end

  test "can open reference" do
    ref = Rugged::Reference.lookup(@repo, "refs/heads/master")
    assert_equal "36060c58702ed4c2a40832c51758d5344201d89a", ref.target
    assert_equal "commit", ref.type
    assert_equal "refs/heads/master", ref.name
  end

  test "can create reference from symbolic reference" do
    ref = Rugged::Reference.create(@repo, "refs/heads/unit_test", "refs/heads/master")
    assert_equal "refs/heads/master", ref.target
    assert_equal "tree", ref.type
    assert_equal "refs/heads/unit_test", ref.name
    ref.delete
  end

  test "can create reference from sha" do
    ref = Rugged::Reference.create(@repo, "refs/heads/unit_test", "36060c58702ed4c2a40832c51758d5344201d89a")
    assert_equal "36060c58702ed4c2a40832c51758d5344201d89a", ref.target
    assert_equal "commit", ref.type
    assert_equal "refs/heads/unit_test", ref.name
    ref.delete
  end

  test "can rename ref" do
    ref = Rugged::Reference.create(@repo, "refs/heads/unit_test", "36060c58702ed4c2a40832c51758d5344201d89a")
    assert_equal "36060c58702ed4c2a40832c51758d5344201d89a", ref.target
    assert_equal "commit", ref.type
    assert_equal "refs/heads/unit_test", ref.name

    ref.name = "refs/heads/new_name"
    assert_equal "refs/heads/new_name", ref.name
    ref.delete
  end

  test "can set target on reference" do
    ref = Rugged::Reference.create(@repo, "refs/heads/unit_test", "36060c58702ed4c2a40832c51758d5344201d89a")
    assert_equal "36060c58702ed4c2a40832c51758d5344201d89a", ref.target
    assert_equal "commit", ref.type
    assert_equal "refs/heads/unit_test", ref.name

    ref.target = "5b5b025afb0b4c913b4c338a42934a3863bf3644"
    assert_equal "5b5b025afb0b4c913b4c338a42934a3863bf3644", ref.target
    ref.delete
  end
end
