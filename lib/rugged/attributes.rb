module Rugged
  class Repository
    def attributes(path, options = {})
      Attributes.new(self, path, options)
    end

    class Attributes
      include Enumerable

      def self.parse_opts(options)
        flags = 0

        case options[:priority]
        when [:file, :index]
          flags = 0
        when [:index, :file]
          flags = 1
        when [:index]
          flags = 2
        else
          raise TypeError, "Invalid load priority"
        end

        flags |= 4 if options[:skip_system]
        flags
      end

      def initialize(repository, path, options = {})
        @repository = repository
        @path = path
        @load_flags = Attributes.parse_opts(options)
      end

      def [](attribute)
        @repository.fetch_attributes(@path, attribute, @load_flags)
      end

      def to_h
        @hash ||= @repository.fetch_attributes(@path, nil, @load_flags)
      end

      def each(&block)
        to_h.each(&block)
      end
    end
  end
end
