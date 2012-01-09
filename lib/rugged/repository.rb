module Rugged
  class Repository

    def inspect
      "#<Rugged::Repository:#{object_id} {path: #{path.inspect}}>"
    end

    def walk(from, sorting=Rugged::SORT_DATE, &block)
      walker = Rugged::Walker.new(self)
      walker.sorting(sorting)
      walker.push(from)
      walker.each(&block)
    end

    def head
      ref = Reference.lookup(self, "HEAD")
      ref.resolve
    end

    def lookup(oid)
      Rugged::Object.lookup(self, oid)
    end

    def refs(pattern = nil)
      r = []
      ref_names.each do |ref_name|
        if pattern
          p = Regexp.new(pattern)
          r << Rugged::Reference.lookup(self, ref_name) if p.match(ref_name)
        else
          r << Rugged::Reference.lookup(self, ref_name)
        end
      end
      r
    end

    def ref_names
      Rugged::Reference.each(self)
    end

    def tags(pattern="")
      Rugged::Tag.each(self, pattern)
    end
  end

end
