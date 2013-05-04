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
    assert_equal "new file\n", blob.text
  end

  def test_blob_sloc
    oid = "7771329dfa3002caf8c61a0ceb62a31d09023f37"
    blob = @repo.lookup(oid)
    assert_equal 328, blob.sloc
  end

  def test_blob_content_with_size
    oid = "7771329dfa3002caf8c61a0ceb62a31d09023f37"
    blob = @repo.lookup(oid)
    content =  blob.content(10)
    assert_equal "# Rugged\n*", content
    assert_equal 10, content.size
  end

  def test_blob_content_with_size_gt_file_size
    oid = "7771329dfa3002caf8c61a0ceb62a31d09023f37"
    blob = @repo.lookup(oid)
    content =  blob.content(1000000)
    assert_equal blob.size, content.size
  end

  def test_blob_content_with_zero_size
    oid = "7771329dfa3002caf8c61a0ceb62a31d09023f37"
    blob = @repo.lookup(oid)
    content =  blob.content(0)
    assert_equal '', content
  end

  def test_blob_content_with_negative_size
    oid = "7771329dfa3002caf8c61a0ceb62a31d09023f37"
    blob = @repo.lookup(oid)
    content =  blob.content(-100)
    assert_equal blob.size, content.size
  end

  def test_blob_text_with_max_lines
    oid = "7771329dfa3002caf8c61a0ceb62a31d09023f37"
    blob = @repo.lookup(oid)
    assert_equal "# Rugged\n", blob.text(1)
  end

  def test_blob_text_with_lines_gt_file_lines
    oid = "7771329dfa3002caf8c61a0ceb62a31d09023f37"
    blob = @repo.lookup(oid)
    text = blob.text(1000000)
    assert_equal 464, text.lines.count
  end

  def test_blob_text_with_zero_lines
    oid = "7771329dfa3002caf8c61a0ceb62a31d09023f37"
    blob = @repo.lookup(oid)
    text = blob.text(0)
    assert_equal '', text
  end

  def test_blob_text_with_negative_lines
    oid = "7771329dfa3002caf8c61a0ceb62a31d09023f37"
    blob = @repo.lookup(oid)
    text = blob.text(-100)
    assert_equal 464, text.lines.count
  end

  def test_blob_text_default_encoding
    skip unless String.method_defined?(:encoding)
    oid = "7771329dfa3002caf8c61a0ceb62a31d09023f37"
    blob = @repo.lookup(oid)
    assert_equal Encoding::UTF_8, blob.text.encoding
  end

  def test_blob_text_set_encoding
    skip unless String.method_defined?(:encoding)
    oid = "7771329dfa3002caf8c61a0ceb62a31d09023f37"
    blob = @repo.lookup(oid)
    assert_equal Encoding::ASCII_8BIT, blob.text(0, Encoding::ASCII_8BIT).encoding
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
    assert_equal '1d83f106355e4309a293e42ad2a2c4b8bdbe77ae',
      Rugged::Blob.create(@repo, "a new blob content")
  end

  def test_write_blob_from_workdir
    assert_equal '1385f264afb75a56a5bec74243be9b367ba4ca08',
      Rugged::Blob.from_workdir(@repo, "README")
  end

  def test_write_blob_from_disk
    file_path = File.join(TEST_DIR, (File.join('fixtures', 'archive.tar.gz')))
    File.open(file_path, 'rb') do |file|
      oid = Rugged::Blob.from_disk(@repo, file.path)
      assert oid

      blob = @repo.lookup(oid)
      file.rewind
      assert_equal file.read, blob.content
    end
  end
end
