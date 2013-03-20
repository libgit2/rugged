require 'rubygems'

require 'tempfile'
require 'tmpdir'
require 'minitest/autorun'
require 'rugged'
require 'pp'

module Rugged
  class TestCase < MiniTest::Unit::TestCase
    TEST_DIR = File.dirname(File.expand_path(__FILE__))

    protected
    def with_default_encoding(encoding, &block)
      old_encoding = Encoding.default_internal

      new_encoding = Encoding.find(encoding)
      Encoding.default_internal = new_encoding

      yield new_encoding

      Encoding.default_internal = old_encoding
    end
  end

  module RepositoryAccess
    def setup
      @path = File.dirname(__FILE__) + '/fixtures/testrepo.git/'
      @repo = Rugged::Repository.open(@path)
    end
  end

  module TempRepositoryAccess
    def setup 
      @path = temp_repo("testrepo.git")
      @repo = Rugged::Repository.open(@path)
    end

    def teardown 
      destroy_temp_repo(@path)
    end

    protected
    def temp_repo(repo)
      dir = Dir.mktmpdir 'dir'
      repo_dir = File.join(TestCase::TEST_DIR, (File.join('fixtures', repo, '.')))
      `git clone #{repo_dir} #{dir}`
      dir
    end

    def destroy_temp_repo(path)
      FileUtils.remove_entry_secure(path)
    end
  end
end
