require 'benchmark'

dir = File.dirname(File.expand_path(__FILE__))
$LOAD_PATH.unshift dir + '/lib'
require 'ribbit'

n = 50000

sha = "ce08fe4884650f067bd5703b6a59a8b3b3c99a09"
raw = Ribbit::Lib.hex_to_raw(sha)

Benchmark.bm do |x|
  x.report("Ribbit Hex2Raw:") do
    1.upto(n) { Ribbit::Lib.hex_to_raw(sha) }
  end
  x.report("Ruby Hex2Raw  :") do
    1.upto(n) { [sha].pack("H*") }
  end
  x.report("Ribbit Raw2Hex:") do
    1.upto(n) { Ribbit::Lib.raw_to_hex(raw) }
  end
  x.report("Ruby Raw2Hex  :") do
    1.upto(n) { raw.unpack("H*")[0] }
  end
end