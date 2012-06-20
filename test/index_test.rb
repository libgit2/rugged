require "test_helper"
require 'base64'
require 'tempfile'
require 'fileutils'

def new_index_entry
    now = Time.now
    {
      :path => "new_path",
      :oid => "d385f264afb75a56a5bec74243be9b367ba4ca08",
      :mtime => now,
      :ctime => now,
      :file_size => 1000,
      :dev => 234881027,
      :ino => 88888,
      :mode => 33199,
      :uid => 502,
      :gid => 502,
      :stage => 3,
    }
end

context "Rugged::Index reading stuff" do
  setup do
    path = File.dirname(__FILE__) + '/fixtures/testrepo.git/index'
    @index = Rugged::Index.new(path)
  end

  test "can iterate through the index" do
    enum = @index.each
    assert enum.kind_of? Enumerable

    i = 0
    @index.each { |e| i += 1 }
    assert_equal @index.count, i
  end

  test "can count index entries" do
    assert_equal 2, @index.count
  end

  test "can clear the in-memory index" do
    @index.clear
    assert_equal 0, @index.count
  end

  test "can remove entries from the index" do
    @index.remove 0
    assert_equal 1, @index.count
  end

  test "can get all data from an entry" do
    e = @index.get_entry(0)
    assert_equal 'README', e[:path]
    assert_equal '1385f264afb75a56a5bec74243be9b367ba4ca08', e[:oid]
    assert_equal 1273360380, e[:mtime].to_i
    assert_equal 1273360380, e[:ctime].to_i
    assert_equal 4, e[:file_size]
    assert_equal 234881026, e[:dev]
    assert_equal 6674088, e[:ino]
    assert_equal 33188, e[:mode]
    assert_equal 501, e[:uid]
    assert_equal 0, e[:gid]
    assert_equal false, e[:valid]
    assert_equal 0, e[:stage]

    e = @index.get_entry(1)
    assert_equal 'new.txt', e[:path]
    assert_equal 'fa49b077972391ad58037050f2a75f74e3671e92', e[:oid]
  end

  test "can iterate over the entries" do
    itr_test = @index.sort { |a, b| a[:oid] <=> b[:oid] }.map { |e| e[:path] }.join(':')
    assert_equal "README:new.txt", itr_test
  end

  test "can update entries" do
    now = Time.at Time.now.to_i
    e = @index.get_entry(0)

    e[:oid] = "12ea3153a78002a988bb92f4123e7e831fd1138a"
    e[:mtime] = now
    e[:ctime] = now
    e[:file_size] = 1000
    e[:dev] = 234881027
    e[:ino] = 88888
    e[:mode] = 33199
    e[:uid] = 502
    e[:gid] = 502
    e[:stage] = 3

    @index.add(e)
    new_e = @index[e[:path]]

    # git only sets executable bit based on owner
    e[:mode] = 33188
    assert_equal e, new_e
  end

  test "can add new entries" do
    e = new_index_entry
    @index << e
    assert_equal 3, @index.count
    itr_test = @index.sort { |a, b| a[:oid] <=> b[:oid] }.map { |x| x[:path] }.join(':')
    assert_equal "README:new_path:new.txt", itr_test
  end
end

context "Rugged::Index writing stuff" do
  setup do
    path = File.dirname(__FILE__) + '/fixtures/testrepo.git/index'
    @tmppath = Tempfile.new('index').path
    FileUtils.copy(path, @tmppath)
    @index = Rugged::Index.new(@tmppath)
  end

  test "add raises if it gets something weird" do
    assert_raise TypeError do
      @index.add(21)
    end
  end

  test "can write a new index" do
    e = new_index_entry
    @index << e

    e[:path] = "else.txt"
    @index << e

    @index.write

    index2 = Rugged::Index.new(@tmppath)

    itr_test = index2.sort { |a, b| a[:oid] <=> b[:oid] }.map { |x| x[:path] }.join(':')
    assert_equal "README:else.txt:new_path:new.txt", itr_test
    assert_equal 4, index2.count
  end
end

context "Rugged::Index with working directory" do
  setup do
    @tmppath = Dir.mktmpdir
    @repo = Rugged::Repository.init_at(@tmppath, false)
    @index = @repo.index
  end

  teardown do
    FileUtils.remove_entry_secure(@tmppath)
  end

  test "can add from a path" do
    File.open(File.join(@tmppath, 'test.txt'), 'w') do |f|
      f.puts "test content"
    end
    @index.add('test.txt')
    @index.write

    index2 = Rugged::Index.new(@tmppath + '/.git/index')
    assert_equal index2.get_entry(0)[:path], 'test.txt'
  end

  test "can reload the index" do
    File.open(File.join(@tmppath, 'test.txt'), 'w') do |f|
      f.puts "test content"
    end
    @index.add('test.txt', 2)
    @index.write

    sleep(1) # we need this sleep to sync at the FS level
    # most FSs have 1s granularity on mtimes

    rindex = Rugged::Index.new(File.join(@tmppath, '/.git/index'))
    e = rindex['test.txt']
    assert_equal 2, e[:stage]

    rindex << new_index_entry
    rindex.write

    assert_equal 1, @index.count
    @index.reload
    assert_equal 2, @index.count

    e = @index['new_path']
    assert_equal e[:mode], 33199
  end
end
