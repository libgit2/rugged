require 'test_helper'
require 'base64'

class RepositoryCleanTest < Rugged::TestCase
  def setup
    @source_repo = FixtureRepo.from_rugged("testrepo.git")
    @repo = FixtureRepo.clone(@source_repo)
  end

  def repo_file_path; File.join('subdir', 'README') end
  def file_path;      File.join(@repo.workdir, 'subdir', 'README') end

  def test_clean_with_no_arguments
    new_file = File.join(@repo.workdir, 'subdir', 'README.md')
    File.open(new_file, 'w') do |f|
      f.puts "test content"
    end
    @repo.clean
    refute File.exist?(new_file)
  end

  def test_clean_directories
    directory = File.join(@repo.workdir, 'clean_dir')
    FileUtils.mkdir_p(directory)
    @repo.clean(directories: true)
    refute File.exist?(directory)
  end

  def test_clean_force
    directory = File.join(@repo.workdir, 'clean_dir')
    FileUtils.mkdir_p(directory)
    @repo.clean(force: true)
    refute File.exist?(directory)
  end

  def test_clean_ignored
    FileUtils.mkdir_p(File.join(@repo.workdir, 'clean_dir'))
    @repo.clean(ignored: true)
  end
end
