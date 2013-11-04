#!/usr/bin/env ruby
$:.unshift File.expand_path("../../lib", __FILE__)

require 'optparse'
require 'ostruct'
require 'rugged'

def print_commit(commit)
  puts "commit #{commit.oid}"

  if commit.parents.size > 1
    puts "Merge: #{commit.parents.map(&:oid).join(' ')}"
  end

  if author = commit.author
    puts "Author: #{author[:name]} <#{author[:email]}>"
  end

  puts "Date: #{commit.time}"

  puts ""

  commit.message.lines.each do |line|
    puts "    #{line}"
  end

  puts ""
end

def parse_options(args)
  options = OpenStruct.new
  options.repodir = "."
  options.paths = []
  options.sorting = [Rugged::SORT_DATE]
  options.skip = 0
  options.limit = -1
  options.min_parents = 0
  options.max_parents = -1

  opt_parser = OptionParser.new do |opts|
    opts.banner = "Usage log.rb [options]"

    opts.on "--git-dir=DIR" do |dir|
      options.repodir = dir
    end

    opts.on "-n", "--max-count=max", Integer do |limit|
      options.limit = limit
    end

    opts.on "--skip=count", Integer do |skip|
      options.skip = skip
    end

    opts.on "--date-order" do
      options.sorting << Rugged::SORT_DATE
    end

    opts.on "--topo-order" do
      options.sorting << Rugged::SORT_TOPO
    end

    opts.on "--merges" do
      options.min_parents = 2
    end

    opts.on "--no-merges" do
      options.max_parents = 1
    end

    opts.on "--no-min-parents" do
      options.min_parents = 0
    end

    opts.on "--no-max-parents" do
      options.max_parents = -1
    end

    opts.on "--reverse" do
      options.sorting << Rugged::SORT_REVERSE
    end
  end
  opt_parser.parse!(args)

  options.paths = args

  options
end

options = parse_options(ARGV)

repo = Rugged::Repository.new(options.repodir)
walker = Rugged::Walker.new(repo)
options.sorting.each do |order|
  walker.sorting(order)
end

walker.push("HEAD")
printed = count = 0
walker.each do |commit|

  next if commit.parents.size < options.min_parents
  next if options.max_parents > 0 && commit.parents.size > options.max_parents

  if options.paths.size > 0
    next unless commit.diff(paths: options.paths).size > 0
  end

  next if (count += 1) < options.skip
  break if options.limit > 0 && (printed += 1) > options.limit

  print_commit(commit)
end
