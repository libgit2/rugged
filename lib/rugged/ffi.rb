require "ffi"

module Rugged
  class ODB < FFI::ManagedStruct
    layout :dummy, :char

    def self.release(ptr)
      LibGit2.odb_free(ptr)
    end

    def exists?(hex)
      oid = OID.from_string(hex)
      LibGit2.odb_exists(self, oid) == 1
    end
  end

  class OID < FFI::Struct
    layout :id, [:uchar, 20]

    def self.from_raw(raw)
      OID.new.tap { |oid| LibGit2.oid_fromraw(oid, raw) }
    end

    def self.from_string(string)
      OID.new.tap { |oid| LibGit2.oid_fromstrn(oid, string, string.length) }
    end

    def to_s
      ptr = ::FFI::MemoryPointer.new(:char, 40)
      LibGit2.oid_fmt(ptr, self)
      ptr.get_string(0, 40)
    end
  end

  class Repository < FFI::ManagedStruct
    layout :dummy, :char

    def self.release(ptr)
      LibGit2.repository_free(ptr)
    end

    def self.open(path)
      ptr = ::FFI::MemoryPointer.new(:pointer)

      result = LibGit2.repository_open(ptr, path)
      LibGit2.exception_check(result)

      self.new(ptr.get_pointer(0))
    end

    def self.create
      # TODO: create code
    end

    def exists?(hex)
      ptr = ::FFI::MemoryPointer.new(:pointer)

      result = LibGit2.repository_odb(ptr, self)
      LibGit2.exception_check(result)

      ODB.new(ptr.get_pointer(0)).exists?(hex)
    end
    alias :include? :exists?

    def read(*args)
      raise "Not implemented"
    end

    def read_header(*args)
      raise "Not implemented"
    end

    def write(*args)
      raise "Not implemented"
    end

    def each_id(*args)
      raise "Not implemented"
    end

    def path
      LibGit2.repository_path(self)
    end

    def workdir
      LibGit2.repository_workdir(self)
    end

    def workdir=(path)
      result = LibGit2.repository_set_workdir(self, path)
      LibGit2.exception_check(result)
    end

    def status(*args)
      raise "Not implemented"
    end

    def index(*args)
      raise "Not implemented"
    end

    def index=(*args)
      raise "Not implemented"
    end

    def config(*args)
      raise "Not implemented"
    end

    def config=(*args)
      raise "Not implemented"
    end

    def bare?
      LibGit2.repository_is_bare(self) == 1
    end

    def empty?
      LibGit2.repository_is_empty(self) == 1
    end

    def head_detached?
      LibGit2.repository_head_detached(self) == 1
    end

    def head_orphan?
      LibGit2.repository_head_orphan(self) == 1
    end

    def merge_base(one, two)
      base = OID.new

      result = LibGit2.merge_base(base, self, OID.from_string(one), OID.from_string(two))
      LibGit2.exception_check(result)

      return base.to_s
    end
  end

  class Reference < FFI::ManagedStruct
    layout :dummy, :char

    include Comparable
    
    def self.lookup(repository, name)
      ptr = ::FFI::MemoryPointer.new(:pointer)

      result = LibGit2.reference_lookup(ptr, repository, name)
      LibGit2.exception_check(result)

      new(ptr.get_pointer(0))
    end

    def self.each(repository)
      return self.enum_for(:each, repository) unless block_given?

      LibGit2.reference_foreach(repository, 3, lambda { |str, _|
        yield str
        0 # The callback expects an int to be returned
      }, nil)
      
      self
    end

    def target
      raise "Not implemented"
    end

    def name
      raise "Not implemented"
    end

    def self.release(ptr)
      LibGit2.reference_free(ptr)
    end
  end

  class Branch < Rugged::Reference
    def self.each(repository)
      return self.enum_for(:each, repository) unless block_given?

      LibGit2.branch_foreach(repository, 3, lambda { |str, type, _|
        yield str
        0 # The callback expects an int to be returned
      }, nil)
      
      self
    end
  end

  class StrArray < FFI::ManagedStruct
    layout :strings, :pointer,
           :count, :ulong
 
    def self.release(ptr)
      LibGit2.strarray_free(self)
    end

    def each
      self[:strings].read_array_of_pointer(self[:count]).each do |ptr|
        yield ptr.get_string(0)
      end
    end
   end

  class Tag
    def self.each(repository, pattern = "")
      string_array = StrArray.new

      result = LibGit2.tag_list_match(string_array, pattern, repository)
      LibGit2.exception_check(result)
 
      string_array.each do |tag_name|
        yield tag_name
      end
 
      self
    end
  end

  class Error < StandardError; end
  class NoMemError < ::NoMemoryError; end
  class OSError < ::IOError; end
  class InvalidError < ::ArgumentError; end
  class ReferenceError < Error; end
  class ZlibError < Error; end
  class RepositoryError < Error; end
  class ConfigError < Error; end
  class RegexError < Error; end
  class OdbError < Error; end
  class IndexError < Error; end
  class ObjectError < Error; end
  class NetworkError < Error; end
  class TagError < Error; end
  class TreeError < Error; end
  class IndexerError < Error; end

  ERROR_MAP = {
    :nomemory     => NoMemError,
    :os           => OSError,
    :invalid      => InvalidError,
    :reference    => ReferenceError,
    :zlib         => ZlibError,
    :repository   => RepositoryError,
    :config       => ConfigError,
    :regex        => RegexError,
    :odb          => OdbError,
    :index        => IndexError,
    :object       => ObjectError,
    :net          => NetworkError,
    :tag          => TagError,
    :tree         => TreeError,
    :indexe       => IndexerError,
  }

  module LibGit2
    extend ::FFI::Library
    ffi_lib "libgit2"

    RC = enum [
      :OK, 0,
      :ERROR, -1,
      :ENOTFOUND, -3,
      :EEXISTS, -4,
      :EAMBIGUOUS, -5,
      :EBUFS, -6,
      :EUSER, -7,
      :EBAREREPO, -8,
      :EORPHANEDHEAD, -9,
      :EUNMERGED, -10,
      :ENONFASTFORWARD, -11,
      :EINVALIDSPEC, -12,
      :EMERGECONFLICT, -13,

      :PASSTHROUGH, -30,
      :ITEROVER, -31,
    ]

    enum :branch_t, [
      :local, 1,
      :remote, 2,
    ]

    enum :error_t, [
      :nomemory, 0,
      :os, 1,
      :invalid, 2,
      :reference, 3,
      :zlib, 4,
      :repository, 5,
      :config, 6,
      :regex, 7,
      :odb, 8,
      :index, 9,
      :object, 10,
      :net, 11,
      :tag, 12,
      :tree, 13,
      :indexer, 14
    ]

    class Error < FFI::Struct
      layout :message, :string,
             :klass, :error_t
    end

    attach_function :giterr_last, [], Error
    attach_function :giterr_clear, [], :void

    attach_function :merge_base, :git_merge_base, [OID, Repository, OID, OID], :int

    attach_function :repository_is_empty, :git_repository_is_empty, [Repository], :int
    attach_function :repository_path, :git_repository_path, [Repository], :string
    attach_function :repository_workdir, :git_repository_workdir, [Repository], :string
    attach_function :repository_set_workdir, :git_repository_set_workdir, [Repository, :string], RC
    attach_function :repository_is_bare, :git_repository_is_bare, [Repository], :int
    attach_function :repository_free, :git_repository_free, [Repository], :void
    attach_function :repository_open, :git_repository_open, [:pointer, :string], RC
    attach_function :repository_head, :git_repository_head, [:pointer, Repository], RC
    attach_function :repository_odb, :git_repository_odb, [:pointer, Repository], RC
    attach_function :repository_head_detached, :git_repository_head_detached, [Repository], :int
    attach_function :repository_head_orphan, :git_repository_head_orphan, [Repository], :int
    attach_function :repository_is_empty, :git_repository_is_empty, [Repository], :int

    attach_function :reference_free, :git_reference_free, [Reference], :void
    attach_function :reference_name, :git_reference_name, [Reference], :string
    attach_function :reference_owner, :git_reference_owner, [Reference], Repository

    callback :branch_foreach_cb, [:string, :branch_t, :void], :int
    attach_function :branch_move, :git_branch_move, [Repository, :string, :string, :int], RC
    attach_function :branch_foreach, :git_branch_foreach, [Repository, :int, :branch_foreach_cb, :pointer], RC

    callback :reference_foreach_cb, [:string, :void], :int
    attach_function :reference_foreach, :git_reference_foreach, [Repository, :uint, :reference_foreach_cb, :pointer], RC
    attach_function :reference_lookup, :git_reference_lookup, [:pointer, Repository, :string], RC
    attach_function :reference_cmp, :git_reference_cmp, [Reference, Reference], :int

    attach_function :strarray_free, :git_strarray_free, [StrArray], :void

    attach_function :tag_list_match, :git_tag_list_match, [StrArray, :string, Repository], :int

    attach_function :oid_fromstrn, :git_oid_fromstrn, [OID, :string, :ulong], :int
    attach_function :oid_fromraw, :git_oid_fromraw, [OID, :pointer], :void
    attach_function :oid_fmt, :git_oid_fmt, [:pointer, OID], :void
    attach_function :oid_pathfmt, :git_oid_pathfmt, [:pointer, OID], :void
    attach_function :oid_iszero, :git_oid_iszero, [OID], :int

    attach_function :odb_free, :git_odb_free, [ODB], :void
    attach_function :odb_exists, :git_odb_exists, [ODB, OID], :int

    def self.exception_check(value)
      return if value == :OK || (error = giterr_last).null?

      # Why does giterr_last return a pointer and not an Error struct?
      error = Error.new(error)

      exception = (ERROR_MAP[error[:klass]] || Rugged::Error).new(error[:message])
      giterr_clear
      raise exception
    end
  end
end