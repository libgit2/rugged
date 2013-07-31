require 'rubygems'

require 'tempfile'
require 'tmpdir'
require 'minitest/autorun'
require 'rugged'
require 'pp'

module Rugged
  class TestCase < MiniTest::Unit::TestCase
    # Ruby 1.8 / 1.9 Shim
    Enumerator = defined?(::Enumerator) ? ::Enumerator : ::Enumerable::Enumerator

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

  class SandboxedTestCase < TestCase
    def setup
      super
      @_sandbox_path = Dir.mktmpdir("rugged_sandbox")
    end

    def teardown
      FileUtils.remove_entry_secure @_sandbox_path
      super
    end

    def sandbox_fixture(repository)
      FileUtils.cp_r(File.join(TestCase::TEST_DIR, 'fixtures', repository), @_sandbox_path)
    end

    # Fills the current sandbox folder with the files
    # found in the given repository
    def sandbox_init(repository)
      sandbox_fixture(repository)

      Dir.chdir(File.join(@_sandbox_path, repository)) do
        File.rename(".gitted", ".git") if File.exists?(".gitted")
        File.rename("gitattributes", ".gitattributes") if File.exists?("gitattributes")
        File.rename("gitignore", ".gitignore") if File.exists?("gitignore")
      end

      Rugged::Repository.new(File.join(@_sandbox_path, repository))
    end

    def sandbox_clone(repository, name)
      Dir.chdir(@_sandbox_path) do
        `git clone #{repository} #{name}`
      end

      Rugged::Repository.new(File.join(@_sandbox_path, name))
    end
  end

  class SubmoduleTestCase < Rugged::SandboxedTestCase
    def setup
      super
    end

    def setup_submodule
      repository = sandbox_init('submod2')
      sandbox_fixture('submod2_target')

      Dir.chdir(@_sandbox_path) do
        File.rename(
          File.join('submod2_target', '.gitted'),
          File.join('submod2_target', '.git')
        )

        rewrite_gitmodules(repository.workdir)

        File.rename(
          File.join('submod2', 'not-submodule', '.gitted'),
          File.join('submod2', 'not-submodule', '.git')
        )

        File.rename(
          File.join('submod2', 'not', '.gitted'),
          File.join('submod2', 'not', '.git')
        )
      end

      repository
    end

    def rewrite_gitmodules(workdir)
      input_path = File.join(workdir, 'gitmodules')
      output_path = File.join(workdir, '.gitmodules')
      submodules = []

      File.open(input_path, 'r') do |input|
        File.open(output_path, 'w') do |output|
          input.each_line do |line|
            if %r{path = (?<submodule>.+$)} =~ line
              submodules << submodule.strip
            # convert relative URLs in "url =" lines
            elsif %r{url = (?<url>.+$)} =~ line
              line = "url = #{File.expand_path(File.join(workdir, url))}\n"
            end
            output.write(line)
          end
        end
        FileUtils.remove_entry_secure(input_path)

        # rename .gitted -> .git in submodule dirs
        submodules.each do |submodule|
          submodule_path = File.join(workdir, submodule)
          if File.exists?(File.join(submodule_path, '.gitted'))
            Dir.chdir(submodule_path) do
              File.rename('.gitted', '.git')
            end
          end
        end
      end
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
      `git clone #{repo_dir} #{dir}`
      dir
    end

    def destroy_temp_repo(path)
      FileUtils.remove_entry_secure(path)
    end
  end
end
