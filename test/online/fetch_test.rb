require 'test_helper'

class OnlineFetchTest < Rugged::OnlineTestCase
  def setup
    @repo = FixtureRepo.empty
  end

  if git_creds?
    def test_fetch_over_git
      @repo.remotes.create("origin", ENV['GITTEST_REMOTE_GIT_URL'])

      @repo.fetch("origin")
    end
  end

  if Rugged.features.include?(:https)
    def test_fetch_over_https
      @repo.remotes.create("origin", "https://github.com/libgit2/TestGitRepository.git")

      @repo.fetch("origin")

      assert_equal [
        "refs/remotes/origin/first-merge",
        "refs/remotes/origin/master",
        "refs/remotes/origin/no-parent",
        "refs/tags/annotated_tag",
        "refs/tags/blob",
        "refs/tags/commit_tree"
      ], @repo.refs.map(&:name).sort
    end

    def test_fetch_over_https_with_certificate_callback
      result = {}
      @repo.remotes.create("origin", "https://github.com/libgit2/TestGitRepository.git")

      @repo.fetch("origin", {
        certificate_check: lambda { |valid, host|
         result[:valid] = valid
         true
        }
      })
      assert_equal result[:valid], 1
    end

    def test_fetch_over_https_with_certificate_callback_fail
      result = {}
      @repo.remotes.create("origin", "https://github.com/libgit2/TestGitRepository.git")

      exception = assert_raises Rugged::NetworkError do
        @repo.fetch("origin", {
          certificate_check: lambda { |valid, host|
            result[:valid] = valid
            false
          }
        })
      end
      assert_equal "user cancelled certificate check", exception.message
    end

    def test_fetch_over_https_with_certificate_callback_exception
      result = {}
      @repo.remotes.create("origin", "https://github.com/libgit2/TestGitRepository.git")

      exception = assert_raises RuntimeError do
        @repo.fetch("origin", {
          certificate_check: lambda { |valid, host|
            result[:valid] = valid
            raise "Exception from callback"
          }
        })
      end
      assert_equal "Exception from callback", exception.message
    end
  end

  if Rugged.features.include?(:ssh) && ssh_creds?
    def test_fetch_over_ssh_with_credentials
      @repo.remotes.create("origin", ENV['GITTEST_REMOTE_SSH_URL'])

      @repo.fetch("origin", {
        credentials: ssh_key_credential
      })
    end

    def test_fetch_over_ssh_with_credentials_from_agent
      @repo.remotes.create("origin", ENV['GITTEST_REMOTE_SSH_URL'])

      @repo.fetch("origin", {
        credentials: ssh_key_credential_from_agent
      })
    end

    def test_fetch_over_ssh_with_credentials_callback
      @repo.remotes.create("origin", ENV['GITTEST_REMOTE_SSH_URL'])

      @repo.fetch("origin", {
        credentials: lambda { |url, username, allowed_types|
          return ssh_key_credential
        }
      })
    end
  end
end
