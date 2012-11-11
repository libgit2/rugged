require "test_helper"
require 'base64'
require 'tempfile'
require 'fileutils'


describe Rugged::Index do
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

  describe "when reading the index" do
    before do
      path = File.dirname(__FILE__) + '/fixtures/testrepo.git/index'
      @index = Rugged::Index.new(path)
    end

    it "can iterate through the index" do
      enum = @index.each
      assert enum.kind_of? Enumerable

      i = 0
      @index.each { |e| i += 1 }
      assert_equal @index.count, i
    end

    it "can count index entries" do
      assert_equal 2, @index.count
    end

    it "can clear the in-memory index" do
      @index.clear
      assert_equal 0, @index.count
    end

    it "can remove entries from the index" do
      @index.remove 'new.txt'
      assert_equal 1, @index.count
    end

    it "can get all data from an entry" do
      e = @index[0]
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

      e = @index[1]
      assert_equal 'new.txt', e[:path]
      assert_equal 'fa49b077972391ad58037050f2a75f74e3671e92', e[:oid]
    end

    it "can iterate over the entries" do
      itr_test = @index.sort { |a, b| a[:oid] <=> b[:oid] }.map { |e| e[:path] }.join(':')
      assert_equal "README:new.txt", itr_test
    end

    it "can update entries" do
      now = Time.at Time.now.to_i
      e = @index[0]

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
      new_e = @index.get e[:path], 3

      assert_equal e, new_e
    end

    it "can add new entries" do
      e = new_index_entry
      @index << e
      assert_equal 3, @index.count
      itr_test = @index.sort { |a, b| a[:oid] <=> b[:oid] }.map { |x| x[:path] }.join(':')
      assert_equal "README:new_path:new.txt", itr_test
    end
  end

  describe "when writing the index" do
    before do
      path = File.dirname(__FILE__) + '/fixtures/testrepo.git/index'
      @tmppath = Tempfile.new('index').path
      FileUtils.copy(path, @tmppath)
      @index = Rugged::Index.new(@tmppath)
    end

    it "add raises if it gets something weird" do
      assert_raises TypeError do
        @index.add(21)
      end
    end

    it "can write a new index" do
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

  describe "when interacting with the working directory" do
    before do
      @tmppath = Dir.mktmpdir
      @repo = Rugged::Repository.init_at(@tmppath, false)
      @index = @repo.index
    end

    after do
      FileUtils.remove_entry_secure(@tmppath)
    end

    it "can add from a path" do
      File.open(File.join(@tmppath, 'test.txt'), 'w') do |f|
        f.puts "test content"
      end
      @index.add('test.txt')
      @index.write

      index2 = Rugged::Index.new(@tmppath + '/.git/index')
      assert_equal index2[0][:path], 'test.txt'
    end

    it "can reload the index" do
      File.open(File.join(@tmppath, 'test.txt'), 'w') do |f|
        f.puts "test content"
      end
      @index.add('test.txt')
      @index.write

      rindex = Rugged::Index.new(File.join(@tmppath, '/.git/index'))
      e = rindex['test.txt']
      assert_equal 0, e[:stage]

      rindex << new_index_entry
      rindex.write

      assert_equal 1, @index.count
      @index.reload
      assert_equal 2, @index.count

      e = @index.get 'new_path', 3
      assert_equal e[:mode], 33199
    end
  end


  describe "when interacting with Rugged::Repository" do
    before do
      @path = temp_repo("testrepo.git")
      @repo = Rugged::Repository.new(@path)
      @index = @repo.index
    end

    it "idempotent read_tree/write_tree" do
      head_sha = Rugged::Reference.lookup(@repo,'HEAD').resolve.target
      tree = @repo.lookup(head_sha).tree
      @index.read_tree(tree)

      index_tree_sha = @index.write_tree
      index_tree = @repo.lookup(index_tree_sha)
      assert_equal tree.oid, index_tree.oid
    end


    it "build tree from index on non-HEAD branch" do
      head_sha = Rugged::Reference.lookup(@repo,'refs/remotes/origin/packed').resolve.target
      tree = @repo.lookup(head_sha).tree
      @index.read_tree(tree)
      @index.remove('second.txt')

      new_tree_sha = @index.write_tree
      assert head_sha != new_tree_sha
      assert_nil @repo.lookup(new_tree_sha)['second.txt']
    end
  end
end
