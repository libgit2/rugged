module Rugged
  class RawObject
    attr_reader :type, :len, :data

    def type=(t)
      if t.is_a? String
        @type = Rugged::string_to_type(t)
      elsif t.is_a? Integer
        @type = t
      else
        raise "Invalid object type"
      end
    end

    def initialize(t, d, l = nil)
      self.type=(t)
      @data = d
      @len = l || d.length 
    end

    def hash
      Repository::hash(self)
    end
  end
end
