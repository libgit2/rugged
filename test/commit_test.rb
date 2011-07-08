require File.expand_path "../test_helper", __FILE__

context "Rugged::Commit tests" do
  setup do
    @path = File.dirname(__FILE__) + '/fixtures/testrepo.git/'
    @repo = Rugged::Repository.new(@path)
  end

  test "can read the commit data" do
    oid = "8496071c1b46c854b31185ea97743be6a8774479"
    obj = @repo.lookup(oid)

    assert_equal obj.oid, oid
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
    assert_equal obj.tree.oid, "181037049a54a1eb5fab404658a3a250b44335d7"
    assert_equal [], obj.parents

    if defined? Encoding
      with_default_encoding('utf-8') do |enc|
        obj = @repo.lookup(oid)

        assert_equal enc, obj.oid.encoding
        assert_equal enc, obj.type.encoding
        assert_equal enc, obj.message.encoding
        assert_equal enc, obj.message_short.encoding

        c = obj.committer
        assert_equal enc, c.name.encoding
        assert_equal enc, c.email.encoding

        c = obj.author
        assert_equal enc, c.name.encoding
        assert_equal enc, c.email.encoding
        assert_equal enc, obj.tree.oid.encoding
      end

      with_default_encoding('ascii') do |enc|
        obj = @repo.lookup(oid)

        assert_equal enc, obj.oid.encoding
        assert_equal enc, obj.type.encoding
        assert_equal enc, obj.message.encoding
        assert_equal enc, obj.message_short.encoding

        c = obj.committer
        assert_equal enc, c.name.encoding
        assert_equal enc, c.email.encoding

        c = obj.author
        assert_equal enc, c.name.encoding
        assert_equal enc, c.email.encoding
        assert_equal enc, obj.tree.oid.encoding
      end
    end
  end
  
  test "can have multiple parents" do
    oid = "a4a7dce85cf63874e984719f4fdd239f5145052f"
    obj = @repo.lookup(oid)
    parents = obj.parents.map {|c| c.oid }
    assert parents.include?("9fd738e8f7967c078dceed8190330fc8648ee56a")
    assert parents.include?("c47800c7266a2be04c571c04d5a6614691ea99bd")
  end

  xtest "can write new commit data" do
    toid = "c4dc1555e4d4fa0e0c9c3fc46734c7c35b3ce90b"
    tree = @repo.lookup(toid)

    obj = Rugged::Commit.new(@repo)
    person = Rugged::Signature.new('Scott', 'schacon@gmail.com', Time.now)

    obj.message = 'new message'
    obj.author = person
    obj.committer = person
    obj.tree = tree
    obj.write
    rm_loose(obj.oid)
  end

end
