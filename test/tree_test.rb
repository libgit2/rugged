require "test_helper"

context "Rugged::Tree tests" do
  setup do
    path = File.dirname(__FILE__) + '/fixtures/testrepo.git/'
    @repo = Rugged::Repository.new(path)
    @oid = "c4dc1555e4d4fa0e0c9c3fc46734c7c35b3ce90b"
    @tree = @repo.lookup(@oid)
  end

  test "can read the tree data" do
    assert_equal @oid, @tree.oid
    assert_equal :tree, @tree.type
    assert_equal 3, @tree.count
    assert_equal "1385f264afb75a56a5bec74243be9b367ba4ca08", @tree[0][:oid]
    assert_equal "fa49b077972391ad58037050f2a75f74e3671e92", @tree[1][:oid]
  end

  test "can read the tree entry data" do
    bent = @tree[0]
    tent = @tree[2]

    assert_equal "README", bent[:name]
    assert_equal :blob, bent[:type]
    # assert_equal 33188, bent.attributes

    assert_equal "subdir", tent[:name]
    assert_equal :tree, tent[:type]
    assert_equal "619f9935957e010c419cb9d15621916ddfcc0b96", tent[:oid]
    assert_equal :tree, @repo.lookup(tent[:oid]).type
  end

  test "can iterate over the tree" do
    enum_test = @tree.sort { |a, b| a[:oid] <=> b[:oid] }.map { |e| e[:name] }.join(':')
    assert_equal "README:subdir:new.txt", enum_test

    enum = @tree.each
    assert enum.kind_of? Enumerable
  end

  test "can walk the tree, yielding only trees" do
    @tree.walk_trees {|root, entry| assert_equal :tree, entry[:type]}
  end

  test "can walk the tree, yielding only blobs" do
    @tree.walk_blobs {|root, entry| assert_equal :blob, entry[:type]}
  end

  test "can iterate over the subtrees a tree" do
    @tree.each_tree {|tree| assert_equal :tree, tree[:type]}
  end

  test "can iterate over the subtrees a tree" do
    @tree.each_blob {|tree| assert_equal :blob, tree[:type]}
  end

  test "can write the tree data" do
    entry = {:type => :blob,
             :name => "README.txt",
             :oid  => "1385f264afb75a56a5bec74243be9b367ba4ca08",
             :filemode => 33188}

    builder = Rugged::Tree::Builder.new
    builder << entry
    sha = builder.write(@repo)
    obj = @repo.lookup(sha)
    assert_equal 38, obj.read_raw.len
  end

end
