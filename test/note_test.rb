require 'test_helper'

class NoteTest < Rugged::TestCase
  include Rugged::RepositoryAccess

  def test_read_note_for_object
    oid = "36060c58702ed4c2a40832c51758d5344201d89a"
    obj = @repo.lookup(oid)
    notes = obj.notes
    assert_equal "note text\n", notes[:message]
    assert_equal "94eca2de348d5f672faf56b0decafa5937e3235e", notes[:oid]
  end

  def test_read_note_for_object_from_ref
    oid = "36060c58702ed4c2a40832c51758d5344201d89a"
    obj = @repo.lookup(oid)
    notes = obj.notes('refs/notes/commits')
    assert_equal "note text\n", notes[:message]
    assert_equal "94eca2de348d5f672faf56b0decafa5937e3235e", notes[:oid]
  end

  def test_object_without_note
    oid = "8496071c1b46c854b31185ea97743be6a8774479"
    obj = @repo.lookup(oid)
    assert_nil obj.notes
  end

  def test_nil_ref_lookup
    oid = "36060c58702ed4c2a40832c51758d5344201d89a"
    obj = @repo.lookup(oid)
    assert_nil obj.notes('refs/notes/missing')
  end

  def test_iterate_over_notes
    @repo.each_note('refs/notes/commits') do |note_blob, annotated_object|
      assert_equal "note text\n", note_blob.content
      assert_equal "36060c58702ed4c2a40832c51758d5344201d89a", annotated_object.oid
    end
  end

  def test_each_note_enumerable
    enum = @repo.each_note('refs/notes/commits')
    assert enum.kind_of? Enumerable
  end
end
