require 'test_helper'

class OnlineFetchTest < Rugged::SandboxedTestCase
  def setup
    super
    @repo = Rugged::Repository.init_at(File.join(@_sandbox_path, "repo"))
  end

  def teardown
    @repo.close
    super
  end

  def ssh_creds?
    %w{URL USER KEY PUBKEY PASSPHRASE}.all? { |key| ENV["GITTEST_REMOTE_SSH_#{key}"] }
  end

  def git_creds?
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

  def test_fetch_over_https
    Rugged::Remote.add(@repo, "origin", "https://github.com/libgit2/TestGitRepository.git")

    @repo.fetch("origin")

    assert_equal 5, @repo.refs.count
    assert_equal [
      "refs/remotes/origin/first-merge",
      "refs/remotes/origin/master",
      "refs/remotes/origin/no-parent",
      "refs/tags/blob",
      "refs/tags/commit_tree"
    ], @repo.refs.map(&:name).sort
  end

  def test_fetch_over_git
    skip unless git_creds?

    Rugged::Remote.add(@repo, "origin", ENV['GITTEST_REMOTE_GIT_URL'])

    @repo.fetch("origin")
  end

  if Rugged.features.include? :ssh
    def test_fetch_over_ssh_with_credentials
      skip unless ssh_creds?

      Rugged::Remote.add(@repo, "origin", ENV['GITTEST_REMOTE_SSH_URL'])

      @repo.fetch("origin", {
        credentials: ssh_key_credential
      })
    end

    def test_fetch_over_ssh_with_credentials_from_agent
      skip unless ssh_creds?

      Rugged::Remote.add(@repo, "origin", ENV['GITTEST_REMOTE_SSH_URL'])

      @repo.fetch("origin", {
        credentials: ssh_key_credential_from_agent
      })
    end

    def test_fetch_over_ssh_with_credentials_callback
      skip unless ssh_creds?

      Rugged::Remote.add(@repo, "origin", ENV['GITTEST_REMOTE_SSH_URL'])

      @repo.fetch("origin", {
        credentials: lambda { |url, username, allowed_types|
          return ssh_key_credential
        }
      })
    end
  end
end
