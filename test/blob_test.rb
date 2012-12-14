require "test_helper"

class BlobTest < Rugged::TestCase
  include Rugged::RepositoryAccess

  def test_read_blob_data
    oid = "fa49b077972391ad58037050f2a75f74e3671e92"
    blob = @repo.lookup(oid)
    assert_equal 9, blob.size
    assert_equal "new file\n", blob.content
    assert_equal :blob, blob.type
    assert_equal oid, blob.oid
  end
end

class BlobWriteTest < Rugged::TestCase
  include Rugged::TempRepositoryAccess

  def test_fetch_blob_content_with_nulls
    content = "100644 example_helper.rb\x00\xD3\xD5\xED\x9DA4_"+
               "\xE3\xC3\nK\xCD<!\xEA-_\x9E\xDC=40000 examples\x00"+
               "\xAE\xCB\xE9d!|\xB9\xA6\x96\x024],U\xEE\x99\xA2\xEE\xD4\x92"

    content.force_encoding('binary') if content.respond_to?(:force_encoding)

    oid = @repo.write(content, 'tree')
    blob = @repo.lookup(oid)
    assert_equal content, blob.read_raw.data
  end

  def test_write_blob_data
    oid = Rugged::Blob.create(@repo, "a new blob content")
  end
end
