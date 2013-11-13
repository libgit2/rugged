#!/usr/bin/env ruby

# Rugged "diff" example - shows how to use the diff API
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

# This example demonstrates the use of the Rugged diff APIs to
# create `Rugged::Diff` objects and display them, emulating a number of
# core Git `diff` command line options.
#
# This covers on a portion of the core Git diff options and doesn't
# have particularly good error handling, but it should show most of
# the core Rugged diff APIs, including various types of diffs and
# how to do renaming detection and patch formatting.

COLORS = {
  reset: "\033[m",
  bold:  "\033[1m",
  red:   "\033[31m",
  green: "\033[32m",
  cyan:  "\033[36m",
}

def parse_options(args)
  options = OpenStruct.new
  options.repodir = "."

  opt_parser = OptionParser.new do |opts|
    opts.banner = "Usage blame.rb [options] [<commit range>] <path>"

    opts.on "--git-dir=<path>", "Set the path to the repository." do |dir|
      options.repodir = dir
    end

    opts.on("-p", "-u", "--patch") do
      # TODO
    end

    opts.on("--cached") do
      # TODO
    end

    opts.on("--name-only") do
      # TODO
    end

    opts.on("--name-status") do
      # TODO
    end

    opts.on("--raw") do
      # TODO
    end

    opts.on("--color") do
      # TODO
    end

    opts.on("--no-color") do
      # TODO
    end

    opts.on("-R") do
      # TODO
    end

    opts.on("-a", "--text") do
      # TODO
    end

    opts.on("--ignore-space-at-eol") do

    end

    opts.on("-b", "--ignore-space-change") do

    end

    opts.on("-w", "--ignore-all-space") do

    end

    opts.on("--ignored") do

    end

    opts.on("--untracked") do

    end

    opts.on("-M", "--find-renames") do

    end

    opts.on("-C", "--find-copies") do

    end

    opts.on("--find-copies-harder") do

    end

    opts.on("-B", "--break-rewrites") do

    end

    opts.on("-U", "--unified") do

    end

    opts.on("--inter-hunk-context") do

    end

    opts.on("--src-prefix") do

    end

    opts.on("--dst-prefix") do

    end

    opts.on "-h", "--help", "Show this message" do
      puts opts
      exit
    end
  end

  opt_parser.parse!(args)

  raise "Only one or two tree identifiers can be provided" if args.size > 2

  options.treeish1, options.treeish2 = *args

  options
end

options = parse_options(ARGV)

repo = Rugged::Repository.new(options.repodir)

tree1 = if options.treeish1
  obj = repo.rev_parse(options.treeish1)

  case obj
  when Rugged::Tree
    obj
  when Rugged::Commit
    obj.tree
  else
    # TODO Handle other object types.
  end
end

tree2 = if options.treeish2
  obj = repo.rev_parse(options.treeish1)

  case obj
  when Rugged::Tree
    obj
  when Rugged::Commit
    obj.tree
  else
    # TODO Handle other object types.
  end
end

diff = if tree1 && tree2
  tree1.diff(tree2)
elsif tree1 && options.cached
  repo.index.diff(tree1)
elsif tree1
  tree1.diff_workdir
elsif options.cached
  repo.index.diff(repo, repo.rev_parse("HEAD").tree)
else
  repo.index.diff
end

puts diff.patch
