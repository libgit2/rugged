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
    blob = Rugged::Blob.new
    blob.content = "a new blob content"
    blob.write
  end
end
