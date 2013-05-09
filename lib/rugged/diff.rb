require 'rugged/diff/patch'
require 'rugged/diff/hunk'
require 'rugged/diff/line'
require 'rugged/diff/delta'

module Rugged
  class Diff
    include Enumerable
    alias each each_patch

    def patches
      each_patch.to_a
    end

    def deltas
      each_delta.to_a
    end
  end
end
