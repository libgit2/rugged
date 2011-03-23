require File.expand_path "../test_helper", __FILE__

context "Rugged::Tree tests" do
  setup do
    path = File.dirname(__FILE__) + '/fixtures/testrepo.git/'
    @repo = Rugged::Repository.new(path)
    @sha = "c4dc1555e4d4fa0e0c9c3fc46734c7c35b3ce90b"
    @tree = @repo.lookup(@sha)
  end

  test "can read the tree data" do
    assert_equal @sha, @tree.sha
	  assert_equal "tree", @tree.type
	  assert_equal 3, @tree.entry_count
	  assert_equal "1385f264afb75a56a5bec74243be9b367ba4ca08", @tree[0].sha
	  assert_equal "fa49b077972391ad58037050f2a75f74e3671e92", @tree[1].sha
  end

  test "can read the tree entry data" do
    bent = @tree[0]
    tent = @tree[2]

    assert_equal "README", bent.name
    assert_equal bent.to_object(@repo).sha, bent.sha
    # assert_equal 33188, bent.attributes

    assert_equal "subdir", tent.name
    assert_equal "619f9935957e010c419cb9d15621916ddfcc0b96", tent.to_object(@repo).sha
    assert_equal "tree", tent.to_object(@repo).type

    if defined? Encoding
      with_default_encoding('utf-8') do |enc|
        assert_equal enc, bent.name.encoding
        assert_equal enc, bent.sha.encoding
      end

      with_default_encoding('ascii') do |enc|
        assert_equal enc, bent.name.encoding
        assert_equal enc, bent.sha.encoding
      end
    end
  end

  test "can iterate over the tree" do
    enum_test = @tree.sort { |a, b| a.sha <=> b.sha }.map { |e| e.name }.join(':')
    assert_equal "README:subdir:new.txt", enum_test
  end

  xtest "can write the tree data" do
  end

end
