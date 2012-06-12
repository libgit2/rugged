require "test_helper"

context "Rugged::Blob tests" do
  setup do
    @path = File.dirname(__FILE__) + '/fixtures/testrepo.git/'
    @repo = Rugged::Repository.new(@path)
    @oid = "fa49b077972391ad58037050f2a75f74e3671e92"
  end

  test "can read the blob data" do
    blob = @repo.lookup(@oid)
    assert_equal 9, blob.size
    assert_equal "new file\n", blob.content
    assert_equal :blob, blob.type
    assert_equal @oid, blob.oid
  end

  test "can write new blob data" do
    oid = Rugged::Blob.create(@repo, "a new blob content")
    rm_loose(oid)
  end

  test "gets the complete content if it has nulls" do
    content = "100644 example_helper.rb\x00\xD3\xD5\xED\x9DA4_"+
               "\xE3\xC3\nK\xCD<!\xEA-_\x9E\xDC=40000 examples\x00"+
               "\xAE\xCB\xE9d!|\xB9\xA6\x96\x024],U\xEE\x99\xA2\xEE\xD4\x92"

    content.force_encoding('binary') if content.respond_to?(:force_encoding)

    oid = @repo.write(content, 'tree')
    blob = @repo.lookup(oid)
    assert_equal content, blob.read_raw.data
    rm_loose(oid)
  end

end
