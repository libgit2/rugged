require 'test_helper'

class OnlineCloneTest < Rugged::TestCase
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
      passphrase: ENV["GITTEST_REMOTE_SSH_PASSPHRASE"],
    })
  end

  def ssh_key_credential_from_agent
    Rugged::Credentials::SshKeyFromAgent.new({
      username: ENV["GITTEST_REMOTE_SSH_USER"]
    })
  end

  def test_clone_over_git
    skip unless git_creds?

    Dir.mktmpdir do |dir|
      repo = Rugged::Repository.clone_at(ENV['GITTEST_REMOTE_GIT_URL'], dir)

      assert_instance_of Rugged::Repository, repo
    end
  end

  if Rugged.features.include? :ssh
    def test_clone_over_ssh_with_credentials
      skip unless ssh_creds?

      Dir.mktmpdir do |dir|
        repo = Rugged::Repository.clone_at(ENV['GITTEST_REMOTE_SSH_URL'], dir, {
          credentials: ssh_key_credential
        })

        assert_instance_of Rugged::Repository, repo
      end
    end

    def test_clone_over_ssh_with_credentials_from_agent
      skip unless ssh_creds?

      Dir.mktmpdir do |dir|
        repo = Rugged::Repository.clone_at(ENV['GITTEST_REMOTE_SSH_URL'], dir, {
          credentials: ssh_key_credential_from_agent
        })

        assert_instance_of Rugged::Repository, repo
      end
    end

    def test_clone_over_ssh_with_credentials_callback
      skip unless ssh_creds?

      Dir.mktmpdir do |dir|
        repo = Rugged::Repository.clone_at(ENV['GITTEST_REMOTE_SSH_URL'], dir, {
          credentials: lambda { |url, username, allowed_types|
            return ssh_key_credential
          }
        })

        assert_instance_of Rugged::Repository, repo
      end
    end

    def test_clone_callback_args_without_username
      Dir.mktmpdir do |dir|
        url, username, allowed_types = nil, nil, nil

        assert_raises Rugged::SshError do
          Rugged::Repository.clone_at("github.com:libgit2/TestGitRepository", dir, {
            credentials: lambda { |*args|
              url, username, allowed_types = *args
              return nil
            }
          })
        end

        assert_equal "github.com:libgit2/TestGitRepository", url
        assert_nil username
        assert_equal [:plaintext, :ssh_key].sort, allowed_types.sort
      end
    end

    def test_clone_callback_args_with_username
      Dir.mktmpdir do |dir|
        url, username, allowed_types = nil, nil, nil

        assert_raises Rugged::SshError do
          Rugged::Repository.clone_at("git@github.com:libgit2/TestGitRepository", dir, {
            credentials: lambda { |*args|
              url, username, allowed_types = *args
              return nil
            }
          })
        end

        assert_equal "git@github.com:libgit2/TestGitRepository", url
        assert_equal "git", username
        assert_equal [:plaintext, :ssh_key].sort, allowed_types.sort
      end
    end
  end
end
