require 'rugged/diff/patch'
require 'rugged/diff/hunk'
require 'rugged/diff/line'

module Rugged
  class Diff
    include Enumerable
    alias each each_patch

    def patches
      Enumerator.new(self, :each_patch)
    end
  end
end
