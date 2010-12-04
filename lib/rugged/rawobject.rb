module Rugged
  class RawObject
    attr_reader :type, :len, :data

    def initialize(d, l, t)
      @data = d
      @len = l
      type = t

      if t.is_a? String
        @type = Rugged::string_to_type(t)
      elsif t.is_a? Integer
        @type = t
      else
        raise "Invalid object type"
      end
    end
  end
end
