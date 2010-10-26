require File.dirname(__FILE__) + '/test_helper'

context "Ribbit::Tree tests" do
  setup do
    path = File.dirname(__FILE__) + '/fixtures/testrepo.git/objects'
    @repo = Ribbit::Repository.new(path)
  end

  test "can read the tree data" do
    sha = "c4dc1555e4d4fa0e0c9c3fc46734c7c35b3ce90b"
    obj = @repo.lookup(sha)

    assert_equal sha, obj.sha
	  assert_equal "tree", obj.type
	  assert_equal 3, obj.entry_count
	  assert_equal "1385f264afb75a56a5bec74243be9b367ba4ca08", obj[0].sha
	  assert_equal "fa49b077972391ad58037050f2a75f74e3671e92", obj[1].sha
  end

  test "can read the tree entry data" do
    sha = "c4dc1555e4d4fa0e0c9c3fc46734c7c35b3ce90b"
    obj = @repo.lookup(sha)
    bent = obj[0]
    tent = obj[2]

    assert_equal "README", bent.name
    assert_equal bent.to_object.sha, bent.sha
    # assert_equal 33188, bent.attributes

    assert_equal "subdir", tent.name
    assert_equal "619f9935957e010c419cb9d15621916ddfcc0b96", tent.to_object.sha
    assert_equal "tree", tent.to_object.type
  end

  xtest "can write the tree data" do
  end

end
