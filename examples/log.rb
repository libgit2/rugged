#!/usr/bin/env ruby

# Rugged "log" example - shows how to walk history and get commit info
#
# Written by the Rugged contributors, based on the libgit2 examples.
#
# To the extent possible under law, the author(s) have dedicated all copyright
# and related and neighboring rights to this software to the public domain
# worldwide. This software is distributed without any warranty.
#
# You should have received a copy of the CC0 Public Domain Dedication along
# with this software. If not, see
# <http://creativecommons.org/publicdomain/zero/1.0/>.
$:.unshift File.expand_path("../../lib", __FILE__)

require 'optparse'
require 'ostruct'
require 'rugged'

# This example demonstrates the Rugged rev walker APIs to roughly
# simulate the output of `git log` and a few of command line arguments.
# `git log` has many many options and this only shows a few of them.
#
# This does not have:
#
# - Robust error handling
# - Colorized or paginated output formatting
# - Most of the `git log` options
#
# This does have:
#
# - Examples of translating command line arguments to equivalent Rugged
#   revwalker configuration calls
# - Simplified options to apply pathspec limits and to show basic diffs

# Helper to print a commit object.
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

# Parse some log command line options.
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
