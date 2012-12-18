require "test_helper"
require 'base64'

class RuggedTest < Rugged::TestCase

  @@oids = [
    'd8786bfc974aaaaaaaaaaaaaaaaaaaaaaaaaaaaa',
    'd8786bfc974bbbbbbbbbbbbbbbbbbbbbbbbbbbbb',
    'd8786bfc974ccccccccccccccccccccccccccccc',
    '68d041ee999cb07c6496fbdd4f384095de6ca9e1'
  ]

  def test_hex_to_raw_oid
    raw = Rugged::hex_to_raw("ce08fe4884650f067bd5703b6a59a8b3b3c99a09")
    b64raw = Base64.encode64(raw).strip
    assert_equal "zgj+SIRlDwZ71XA7almos7PJmgk=", b64raw

    hex = "ce08fe4884650f067bd5703b6a59a8b3b3c99a09"
    raw1 = Rugged::hex_to_raw(hex)
    raw2 = [hex].pack("H*")
    assert_equal raw1, raw2
  end

  def test_raw_to_hex
    raw = Base64.decode64("FqASNFZ4mrze9Ld1ITwjqL109eA=")
    hex = Rugged::raw_to_hex(raw)
    assert_equal "16a0123456789abcdef4b775213c23a8bd74f5e0", hex

    raw = Rugged::hex_to_raw("ce08fe4884650f067bd5703b6a59a8b3b3c99a09")
    hex1 = Rugged::raw_to_hex(raw)
    hex2 = raw.unpack("H*")[0]
    assert_equal hex1, hex2
  end

  def test_raw_to_hex_with_nulls
    raw = Rugged::hex_to_raw("702f00394564b24052511cb69961164828bf5494")
    hex1 = Rugged::raw_to_hex(raw)
    hex2 = raw.unpack("H*")[0]
    assert_equal hex1, hex2
  end

  def test_hex_to_raw_with_invalid_character_raises_invalid_error
    assert_raises Rugged::InvalidError do
      Rugged::hex_to_raw("\x16\xA0\x124VWATx\x9A\xBC\xDE\xF4") # invalid bytes
    end
  end

  def test_raw_to_hex_with_invalid_size_raises_type_error
    assert_raises TypeError do
      Rugged::raw_to_hex("702f00394564b24052511cb69961164828bf5") # invalid OID size
    end
  end

  def test_minimize_oid_with_no_block
    assert_equal Rugged::minimize_oid(@@oids), 12
  end

  def test_minimize_oid_with_min_length
    assert_equal Rugged::minimize_oid(@@oids, 20), 20
  end

  def test_minimize_oid_with_block
    minimized_oids = []
    Rugged.minimize_oid(@@oids) { |oid| minimized_oids << oid }

    expected_oids = [
      "d8786bfc974a",
    	"d8786bfc974b",
  		"d8786bfc974c",
 		  "68d041ee999c"
    ]

    assert_equal minimized_oids, expected_oids
  end

  def test_prettify_commit_messages
    message = <<-MESSAGE
Testing this whole prettify business    

with newlines and stuff  
# take out this line haha
# and this one

not this one    
MESSAGE

    clean_message = <<-MESSAGE
Testing this whole prettify business

with newlines and stuff

not this one
MESSAGE

    assert_equal clean_message, Rugged::prettify_message(message, true)
  end
end

