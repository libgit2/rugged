require File.expand_path "../test_helper", __FILE__

context "Rugged::Blob tests" do
  setup do
    @path = File.dirname(__FILE__) + '/fixtures/testrepo.git/'
    @repo = Rugged::Repository.new(@path)
    @sha = "fa49b077972391ad58037050f2a75f74e3671e92"
  end

  test "can read the blob data" do
    blob = @repo.lookup(@sha)
    assert_equal 9, blob.size
    assert_equal "new file\n", blob.content
    assert_equal "blob", blob.type
    assert_equal @sha, blob.sha
  end

  test "can rewrite blob data" do
    blob = @repo.lookup(@sha)
    blob.content = "my new content"
    assert_equal @sha, blob.sha
    blob.write # should change the sha
    assert_equal "2dd916ea1ff086d61fbc1c286079305ffad4e92e", blob.sha
    FileUtils.rm File.join(@path, "objects/2d/d916ea1ff086d61fbc1c286079305ffad4e92e")
  end

  test "can write new blob data" do
    blob = Rugged::Blob.new(@repo)
    blob.content = "a new blob content"
    blob.write
  end

  test "gets the complete content if it has nulls" do
    @content = "100644 example_helper.rb\x00\xD3\xD5\xED\x9DA4_"+
               "\xE3\xC3\nK\xCD<!\xEA-_\x9E\xDC=40000 examples\x00"+
               "\xAE\xCB\xE9d!|\xB9\xA6\x96\x024],U\xEE\x99\xA2\xEE\xD4\x92"
    sha = @repo.write(@content, 'tree')
    blob = @repo.lookup(sha)
    assert_equal @content, blob.read_raw.first
  end

end
