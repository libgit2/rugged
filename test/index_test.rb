require File.dirname(__FILE__) + '/test_helper'
require 'base64'
require 'tempfile'
require 'fileutils'

def new_index_entry
    now = Time.now
    e = Ribbit::IndexEntry.new
    e.path = "new_path"
    e.sha = "d385f264afb75a56a5bec74243be9b367ba4ca08"
    e.mtime = now
    e.ctime = now
    e.file_size = 1000
    e.dev = 234881027
    e.ino = 88888
    e.mode = 33199
    e.uid = 502
    e.gid = 502
    e.flags = 5
    e.flags_extended = 5
    e
end

context "Ribbit::Index reading stuff" do
  setup do
    path = File.dirname(__FILE__) + '/fixtures/testrepo.git/index'
    @index = Ribbit::Index.new(path)
    @index.refresh
  end

  test "can count index entries" do
    assert_equal 2, @index.entry_count
  end

  test "can clear the in-memory index" do
    @index.clear
    assert_equal 0, @index.entry_count
  end

  test "can get all data from an entry" do
    e = @index.get_entry(0)
    assert_equal 'README', e.path
    assert_equal '1385f264afb75a56a5bec74243be9b367ba4ca08', e.sha
    assert_equal 1273360380, e.mtime.to_i
    assert_equal 1273360380, e.ctime.to_i
    assert_equal 4, e.file_size
    assert_equal 234881026, e.dev 
    assert_equal 6674088, e.ino 
    assert_equal 33188, e.mode
    assert_equal 501, e.uid
    assert_equal 0, e.gid
    assert_equal 6, e.flags
    assert_equal 0, e.flags_extended

    e = @index.get_entry(1)
    assert_equal 'new.txt', e.path
    assert_equal 'fa49b077972391ad58037050f2a75f74e3671e92', e.sha
  end

  test "can iterate over the entries" do
    itr_test = @index.sort { |a, b| a.sha <=> b.sha }.map { |e| e.path }.join(':')
    assert_equal "README:new.txt", itr_test
  end

  test "can update entries" do
    now = Time.now
    e = @index.get_entry(0)

    e.path = "new_path"
    e.sha = "12ea3153a78002a988bb92f4123e7e831fd1138a"
    e.mtime = now
    e.ctime = now
    e.file_size = 1000
    e.dev = 234881027
    e.ino = 88888
    e.mode = 33199
    e.uid = 502
    e.gid = 502
    e.flags = 5
    e.flags_extended = 5

    assert_equal 'new_path', e.path
    assert_equal '12ea3153a78002a988bb92f4123e7e831fd1138a', e.sha
    assert_equal now, e.mtime
    assert_equal now, e.ctime
    assert_equal 1000, e.file_size
    assert_equal 234881027, e.dev 
    assert_equal 88888, e.ino 
    assert_equal 33199, e.mode
    assert_equal 502, e.uid
    assert_equal 502, e.gid
    assert_equal 5, e.flags
    assert_equal 5, e.flags_extended
  end

  test "can add new entries" do
    e = new_index_entry
    @index.add(e)
    assert_equal 3, @index.entry_count
    itr_test = @index.sort { |a, b| a.sha <=> b.sha }.map { |e| e.path }.join(':')
    assert_equal "README:new_path:new.txt", itr_test
  end


end

context "Ribbit::Index writing stuff" do
  setup do
    path = File.dirname(__FILE__) + '/fixtures/testrepo.git/index'
    @tmppath = Tempfile.new('index').path
    FileUtils.copy(path, @tmppath)
    @index = Ribbit::Index.new(@tmppath)
    @index.refresh
  end

  test "add raises if it gets something weird" do
    assert_raise TypeError do
      @index.add(21)
    end
  end

  test "can write a new index" do
    e = new_index_entry
    @index.add(e)
    e.path = "else.txt"
    @index.add(e)
    @index.write

    index2 = Ribbit::Index.new(@tmppath)
    index2.refresh

    itr_test = index2.sort { |a, b| a.sha <=> b.sha }.map { |e| e.path }.join(':')
    assert_equal "README:else.txt:new_path:new.txt", itr_test
    assert_equal 4, index2.entry_count
  end
end
