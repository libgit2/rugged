require 'tempfile'
require 'tmpdir'
require 'minitest/autorun'
require 'rugged'
require 'pp'

module Rugged
  class TestCase < MiniTest::Unit::TestCase
    TEST_DIR = File.dirname(File.expand_path(__FILE__))
    LIBGIT2_FIXTURE_DIR = File.expand_path("../../vendor/libgit2/tests/resources", __FILE__)

    protected
    def with_default_encoding(encoding, &block)
      old_encoding = Encoding.default_internal

      new_encoding = Encoding.find(encoding)
      Encoding.default_internal = new_encoding

      yield new_encoding

      Encoding.default_internal = old_encoding
    end
  end

  class SandboxedTestCase < TestCase
    def setup
      super
      @_sandbox_path = Dir.mktmpdir("rugged_sandbox")
    end

    def teardown
      FileUtils.remove_entry_secure @_sandbox_path
      super
    end

    # Fills the current sandbox folder with the files
    # found in the given repository
    def sandbox_init(repository)
      FileUtils.cp_r(File.join(TestCase::LIBGIT2_FIXTURE_DIR, repository), @_sandbox_path)

      Dir.chdir(File.join(@_sandbox_path, repository)) do
        File.rename(".gitted", ".git") if File.exist?(".gitted")
        File.rename("gitattributes", ".gitattributes") if File.exist?("gitattributes")
        File.rename("gitignore", ".gitignore") if File.exist?("gitignore")
      end

      Rugged::Repository.new(File.join(@_sandbox_path, repository))
    end

    def sandbox_clone(repository, name)
      Dir.chdir(@_sandbox_path) do
        `git clone --quiet -- #{repository} #{name}`
      end

      Rugged::Repository.new(File.join(@_sandbox_path, name))
    end
  end

  module RepositoryAccess
    def setup
      @path = File.dirname(__FILE__) + '/fixtures/testrepo.git/'
      @repo = Rugged::Repository.new(@path)
    end
  end

  module TempRepositoryAccess
    def setup
      @path = temp_repo("testrepo.git")
      @repo = Rugged::Repository.new(@path)
    end

    def teardown
      @repo.close
      GC.start
      destroy_temp_repo(@path)
    end

    protected
    def temp_repo(repo)
      dir = Dir.mktmpdir 'dir'
      repo_dir = File.join(TestCase::TEST_DIR, (File.join('fixtures', repo, '.')))
      `git clone --quiet -- #{repo_dir} #{dir}`
      dir
    end

    def destroy_temp_repo(path)
      FileUtils.remove_entry_secure(path)
    end
  end
end
