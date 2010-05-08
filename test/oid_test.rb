require File.dirname(__FILE__) + '/test_helper'
require 'base64'

context "Ribbit::Lib oid stuff" do
  setup do
  end

  test "can convert hex into raw sha" do
    oid = Ribbit::Lib.hex_to_oid("ce08fe4884650f067bd5703b6a59a8b3b3c99a09")
    b64oid = Base64.encode64(oid).strip
    assert_equal "zgj+SIRlDwZ71XA7almos7PJmgk=", b64oid
  end

  test "can convert raw sha into hex" do
    oid = Base64.decode64("FqASNFZ4mrze9Ld1ITwjqL109eA=")
    hex = Ribbit::Lib.oid_to_hex(oid)
    assert_equal "16a0123456789abcdef4b775213c23a8bd74f5e0", hex
  end

end
