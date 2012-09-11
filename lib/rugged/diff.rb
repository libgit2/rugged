require 'rugged/diff/delta'
require 'rugged/diff/hunk'
require 'rugged/diff/line'

module Rugged
  class Diff
    include Enumerable
    alias each each_delta

    def deltas
      Enumerator.new(self, :each_delta)
    end
  end
end
