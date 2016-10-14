module Rugged
  class Index
    include Enumerable

    def diff(*args)
      options = args.pop if args.last.is_a?(Hash)
      other   = args.shift
      _diff other, options
    end

    def to_s
      s = "#<Rugged::Index\n"
      self.each do |entry|
        s << "  [#{entry[:stage]}] '#{entry[:path]}'\n"
      end
      s + '>'
    end
  end
end
