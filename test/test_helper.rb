require 'tempfile'
require 'tmpdir'
require 'minitest/autorun'
require 'rugged'
require 'pp'

module Rugged
  class TestCase < MiniTest::Unit::TestCase
    # Automatically clean up created fixture repos after each test run
    def after_teardown
      Rugged::TestCase::FixtureRepo.eager_teardown
      super
    end

    module FixtureRepo
      # Create a new, empty repository.
      def self.empty(*args)
        path = Dir.mktmpdir("rugged-empty")
        with_cleanup(Rugged::Repository.init_at(path, *args), path)
      rescue
        FileUtils.remove_entry_secure(path)
        raise
      end

      # Create a repository based on a rugged fixture repo.
      def self.from_rugged(name, *args)
        path = Dir.mktmpdir("rugged-#{name}")

        FileUtils.cp_r(File.join(TestCase::TEST_DIR, "fixtures", name, "."), path)

        prepare(path)

        with_cleanup(Rugged::Repository.new(path, *args), path)
      rescue
        FileUtils.remove_entry_secure(path)
        raise
      end

      # Create a repository based on a libgit2 fixture repo.
      def self.from_libgit2(name, *args)
        path = Dir.mktmpdir("rugged-libgit2-#{name}")

        FileUtils.cp_r(File.join(TestCase::LIBGIT2_FIXTURE_DIR, name, "."), path)

        prepare(path)

        with_cleanup(Rugged::Repository.new(path, *args), path)
      rescue
        FileUtils.remove_entry_secure(path)
        raise
      end

      # Create a repository cloned from another Rugged::Repository instance.
      def self.clone(repository)
        path = Dir.mktmpdir("rugged")

        `git clone --quiet -- #{repository.path} #{path}`

        with_cleanup(Rugged::Repository.new(path), path)
      rescue
        FileUtils.remove_entry_secure(path)
        raise
      end

      def self.prepare(path)
        Dir.chdir(path) do
          File.rename(".gitted", ".git") if File.exist?(".gitted")
          File.rename("gitattributes", ".gitattributes") if File.exist?("gitattributes")
          File.rename("gitignore", ".gitignore") if File.exist?("gitignore")
        end
      end

      def self.finalize_cleanup(path)
        proc { FileUtils.remove_entry_secure(path) if File.exist?(path) }
      end

      # Try to eagerly delete directories containing fixture repos.
      def self.eager_teardown
        while path = self.directories.pop
          FileUtils.remove_entry_secure(path) rescue nil
        end
      end

      def self.directories
        @directories ||= []
      end

      # Schedule the given +path+ to be deleted, either when
      # +FixtureRepo.eager_teardown+ is called or when the given +repo+
      # gets gc'ed.
      def self.with_cleanup(repo, path)
        ObjectSpace.define_finalizer(repo, finalize_cleanup(path))
        self.directories << path
        repo
      end
    end

    TEST_DIR = File.dirname(File.expand_path(__FILE__))
    LIBGIT2_FIXTURE_DIR = File.expand_path("../../vendor/libgit2/tests/resources", __FILE__)
  end

  class OnlineTestCase < TestCase
    def reset_remote_repo
      remote_repo = Rugged::Repository.new(ENV['GITTEST_REMOTE_REPO_PATH'])
      remote_repo.references.each do |ref|
        remote_repo.references.delete(ref)
      end
      remote_repo.close
    end

    def self.ssh_creds?
      %w{URL USER KEY PUBKEY PASSPHRASE}.all? { |key| ENV["GITTEST_REMOTE_SSH_#{key}"] }
    end

    def self.git_creds?
      ENV['GITTEST_REMOTE_GIT_URL']
    end

    def ssh_key_credential
      Rugged::Credentials::SshKey.new({
        username:   ENV["GITTEST_REMOTE_SSH_USER"],
        publickey:  ENV["GITTEST_REMOTE_SSH_PUBKEY"],
        privatekey: ENV["GITTEST_REMOTE_SSH_KEY"],
        passphrase: ENV["GITTEST_REMOTE_SSH_PASSPHASE"],
      })
    end

    def ssh_key_credential_from_agent
      Rugged::Credentials::SshKeyFromAgent.new({
        username: ENV["GITTEST_REMOTE_SSH_USER"]
      })
    end
  end

  # Rugged reuses libgit2 fixtures and needs the same setup code.
  #
  # This should be the same as the libgit2 fixture
  # setup in vendor/libgit2/tests/submodule/submodule_helpers.c
  class SubmoduleTestCase < Rugged::TestCase
    def setup_submodule
      repository = FixtureRepo.from_libgit2('submod2')

      rewrite_gitmodules(repository)

      Dir.chdir(repository.workdir) do
        File.rename(
          File.join('not-submodule', '.gitted'),
          File.join('not-submodule', '.git')
        )

        File.rename(
          File.join('not', '.gitted'),
          File.join('not', '.git')
        )
      end

      repository
    end

    def rewrite_gitmodules(repo)
      workdir = repo.workdir

      input_path = File.join(workdir, 'gitmodules')
      output_path = File.join(workdir, '.gitmodules')
      submodules = []

      File.open(input_path, 'r') do |input|
        File.open(output_path, 'w') do |output|
          input.each_line do |line|
            if %r{path = (?<submodule>.+$)} =~ line
              submodules << submodule.strip
            elsif %r{url = \.\.\/(?<url>.+$)} =~ line
              # Copy repositories pointed to by relative urls
              # and replace the relative url by the absolute path to the
              # copied repo.
              path = Dir.mktmpdir(url)
              FileUtils.cp_r(File.join(TestCase::LIBGIT2_FIXTURE_DIR, url, "."), path)
              ObjectSpace.define_finalizer(repo, FixtureRepo.finalize_cleanup(path) )
              line = "url = #{path}\n"
            end
            output.write(line)
          end
        end
      end

      FileUtils.remove_entry_secure(input_path)

      # rename .gitted -> .git in submodule dirs
      submodules.each do |submodule|
        submodule_path = File.join(workdir, submodule)
        if File.exist?(File.join(submodule_path, '.gitted'))
          Dir.chdir(submodule_path) do
            File.rename('.gitted', '.git')
          end
        end
      end
    end
  end
end
