require File.expand_path "../test_helper", __FILE__

context "Rugged::Tag tests" do
  setup do
    @path = File.dirname(__FILE__) + '/fixtures/testrepo.git/'
    @repo = Rugged::Repository.new(@path)
  end

  test "can read the tag data" do
    sha = "0c37a5391bbff43c37f0d0371823a5509eed5b1d"
    obj = @repo.lookup(sha)

    assert_equal sha, obj.sha
	  assert_equal "tag", obj.type
	  assert_equal "test tag message\n", obj.message
	  assert_equal "v1.0", obj.name
	  assert_equal "5b5b025afb0b4c913b4c338a42934a3863bf3644", obj.target.sha
	  assert_equal "commit", obj.target_type
	  c = obj.tagger
	  assert_equal "Scott Chacon", c['name']
    assert_equal 1288114383, c['time'].to_i
    assert_equal "schacon@gmail.com", c['email']
  end
  
  test "can write the tag data" do
    sha = "0c37a5391bbff43c37f0d0371823a5509eed5b1d"
    obj = @repo.lookup(sha)
    obj.message = 'new messages'
    obj.write
    assert_equal "3213a37f636b81c3f91c77c6dbeab47c1d38490a", obj.sha
    rm_loose('3213a37f636b81c3f91c77c6dbeab47c1d38490a')
  end

end
