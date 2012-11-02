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

  test 'can read default ref' do
    default_ref = Rugged::Note.default_ref(@repo)
    assert_equal 'refs/notes/commits', default_ref.name
  end

  test 'can read notes oid' do
    oid = "36060c58702ed4c2a40832c51758d5344201d89a"
    note = Rugged::Note.lookup(@repo, oid, 'refs/notes/commits')
    assert_equal '94eca2de348d5f672faf56b0decafa5937e3235e', note.oid
  end

  test 'can read note message' do
    oid = "36060c58702ed4c2a40832c51758d5344201d89a"
    note = Rugged::Note.lookup(@repo, oid)

    assert_equal "note text\n", note.message
  end

  test 'can iterate over notes' do
    Rugged::Note.each(@repo) do |note_blob, annotated_object|
      assert_equal "note text\n", note_blob.content
      assert_equal "36060c58702ed4c2a40832c51758d5344201d89a", annotated_object.oid
    end
  end
end

context 'Rugged::Note.create' do
  setup do
    @path = temp_repo('testrepo.git')
    @repo = Rugged::Repository.new(@path)
  end

  test 'can create a note' do
    person = {:name => 'Scott', :email => 'schacon@gmail.com', :time => Time.now }

    oid = "8496071c1b46c854b31185ea97743be6a8774479"
    message ="This is the note message\n\nThis note is created from Rugged"

    note_oid = Rugged::Note.create(@repo,
      :message => message,
      :committer => person,
      :author => person,
      :ref => 'refs/notes/test',
      :oid => oid
    )

    assert_equal '38c3a690c474d8dcdb13088205a464a60312eec4', note_oid
    # note is actually a blob
    blob = @repo.lookup(note_oid)
    assert_equal blob.oid, note_oid
    assert_equal blob.content, message
    assert_equal blob.type, :blob

    note = Rugged::Note.lookup(@repo, oid, 'refs/notes/test')
    assert_equal note.oid, note_oid
    assert_equal note.message, message
  end

  test 'cant create note with missing oid' do
    person = {:name => 'Scott', :email => 'schacon@gmail.com', :time => Time.now }
    message ="This is the note message\n\nThis note is created from Rugged"
    oid = "9496071c1b46c854b31185ea97743be6a8774479"
    assert_raises Rugged::OdbError do
      Rugged::Note.create(@repo,
        :message => message,
        :committer => person,
        :author => person,
        :ref => 'refs/notes/test',
        :oid => oid
      )
    end
  end
end

context 'Rugged::Note.remove!' do
  setup do
    @path = temp_repo('testrepo.git')
    @repo = Rugged::Repository.new(@path)
  end

  test 'can remove a note from object' do

    person = {:name => 'Scott', :email => 'schacon@gmail.com', :time => Time.now }

    oid = "8496071c1b46c854b31185ea97743be6a8774479"
    message ="This is the note message\n\nThis note is created from Rugged"

    Rugged::Note.create(@repo,
      :message => message,
      :committer => person,
      :author => person,
      :ref => 'refs/notes/test',
      :oid => oid
    )

    assert Rugged::Note.remove!(@repo,
      :oid=> oid,
      :committer => person,
      :author => person,
      :ref => 'refs/notes/test'
    )

    assert_nil Rugged::Note.lookup(@repo, oid, 'refs/notes/test')
  end
end
