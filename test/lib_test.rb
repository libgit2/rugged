require "test_helper"
require 'base64'

context "Rugged::Lib stuff" do
  setup do
  end

  test "can convert hex into raw oid" do
    raw = Rugged::hex_to_raw("ce08fe4884650f067bd5703b6a59a8b3b3c99a09")
    b64raw = Base64.encode64(raw).strip
    assert_equal "zgj+SIRlDwZ71XA7almos7PJmgk=", b64raw
  end

  test "converts hex into raw oid correctly" do
    hex = "ce08fe4884650f067bd5703b6a59a8b3b3c99a09"
    raw1 = Rugged::hex_to_raw(hex)
    raw2 = [hex].pack("H*")
    assert_equal raw1, raw2
  end

  test "can convert raw oid into hex" do
    raw = Base64.decode64("FqASNFZ4mrze9Ld1ITwjqL109eA=")
    hex = Rugged::raw_to_hex(raw)
    assert_equal "16a0123456789abcdef4b775213c23a8bd74f5e0", hex
  end

  test "converts raw into hex oid correctly" do
    raw = Rugged::hex_to_raw("ce08fe4884650f067bd5703b6a59a8b3b3c99a09")
    hex1 = Rugged::raw_to_hex(raw)
    hex2 = raw.unpack("H*")[0]
    assert_equal hex1, hex2
  end

  test "converts raw into hex with null bytes" do
    raw = Rugged::hex_to_raw("702f00394564b24052511cb69961164828bf5494")
    hex1 = Rugged::raw_to_hex(raw)
    hex2 = raw.unpack("H*")[0]
    assert_equal hex1, hex2
  end
end
