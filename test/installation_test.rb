require "test_helper"

class InstallationTest < Rugged::TestCase
  def test_gnumake_program_is_detected_correctly
    require File.join(ROOT_DIR, 'ext', 'rugged', 'exthelper')
    assert_equal gnumake_for("FreeBSD"), "gmake"
    assert_equal gnumake_for("GNU/kFreeBSD"), "make"
    assert_equal gnumake_for("GNU/Linux"), "make"
    assert_equal gnumake_for("Msys"), "make"
  end
end
