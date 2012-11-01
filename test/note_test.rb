require 'test_helper'

describe 'Rugged::Note' do
  setup do
    @path = File.dirname(__FILE__) + '/fixtures/testrepo.git/'
    @repo = Rugged::Repository.new(@path)
  end

  test 'can read note for oid' do
    oid = "36060c58702ed4c2a40832c51758d5344201d89a"
    note = Rugged::Note.lookup(@repo, oid)

    assert_kind_of Rugged::Note, note
  end

  test 'can read note for oid from specific note ref' do
    oid = "36060c58702ed4c2a40832c51758d5344201d89a"
    note = Rugged::Note.lookup(@repo, oid, 'refs/notes/commits')

    assert_kind_of Rugged::Note, note
  end
end
