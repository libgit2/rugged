#!/usr/bin/env ruby

# Rugged "blame" example - shows how to use the blame API
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

# This example demonstrates how to invoke the Rugged blame API to roughly
# simulate the output of `git blame` and a few of its command line arguments.

def parse_options(args)
  options = OpenStruct.new
  options.repodir = "."

  opt_parser = OptionParser.new do |opts|
    opts.banner = "Usage blame.rb [options] [<commit range>] <path>"

    opts.on "--git-dir=<path>", "Set the path to the repository." do |dir|
      options.repodir = dir
    end

    opts.on "-L", "=<n,m>", "Process only line range n-m, counting from 1" do |lines|
      if lines.match(/(\d+),(\d+)/)
        options.start_line = $1.to_i
        options.end_line = $2.to_i
      else
        raise "-L format error"
      end
    end

    opts.on "-M", "Find line moves within and across files" do
      options.m = true
    end

    opts.on "-C", "Find line copies within and across files" do
      options.c = true
    end
  end
  opt_parser.parse!(args)

  case args.length
  when 0
    raise "Please specify a path"
  when 1
    options.path = args[0]
  when 2
    options.path, options.commitspec = *args
  else
    options.path, options.commitspec = args[0], "#{args[1]}..#{args[2]}"
  end

  options
end

options = parse_options(ARGV)

repo = Rugged::Repository.new(options.repodir)

if options.commitspec
  # TODO: Rugged can't parse revspecs yet.
end

blame = Rugged::Blame.new(repo, options.path, {
  min_line: options.start_line,
  max_line: options.end_line,
  track_copies_same_file: options.m,
  track_copies_same_commit_moves: options.c,
})
blob = repo.rev_parse "HEAD:#{options.path}"

if options.start_line
  line_index = options.start_line
  lines = blob.content.lines[(options.start_line - 1)..(options.end_line - 1)]
else
  line_index = 1
  lines = blob.content.lines
end

lines.each do |line|
  hunk = blame.for_line(line_index)

  sig = "#{hunk[:final_signature][:name]} <#{hunk[:final_signature][:email]}>"
  printf("%s (%-30s %3d) %.*s", hunk[:final_commit_id][0..7], sig, line_index, line.size, line)

  line_index += 1
end
