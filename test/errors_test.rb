require "test_helper"

class ErrorsTest < Rugged::TestCase 

  def test_rugged_error_classes_exist
    error_classes = [
      Rugged::Error,
      Rugged::ReferenceError,
      Rugged::ZlibError,
      Rugged::RepositoryError,
      Rugged::ConfigError,
      Rugged::RegexError,
      Rugged::OdbError,
      Rugged::IndexError,
      Rugged::ObjectError,
      Rugged::NetworkError,
      Rugged::TagError,
      Rugged::TreeError,
      Rugged::IndexerError
    ]

    error_classes.each do |err|
      assert_equal err.class, Class
    end
  end
end


