require File.dirname(__FILE__) + '/test_helper'
require 'base64'

context "Ribbit::Lib oid stuff" do
  setup do
  end

  test "can convert hex into raw sha" do
    oid = Ribbit::Lib.hex_to_oid("ce08fe4884650f067bd5703b6a59a8b3b3c99a09")
    b64oid = Base64.encode64(oid).strip
    assert_equal b64oid, "zgj+SIRlDwZ71XA7almos7PJmgk="
  end

end
