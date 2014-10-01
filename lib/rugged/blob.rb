module Rugged
  class Blob
    def hashsig
      @hashsig ||= HashSignature.new(self)
    end

    def similarity(other)
      other_sig = case other
      when HashSignature
        other
      when String
        HashSignature.new(other)
      when Blob
        other.hashsig
      else
        raise TypeError, "Expected a Rugged::Blob, String or Rugged::Blob::HashSignature"
      end

      HashSignature.compare(self.hashsig, other_sig)
    end
  end
end
