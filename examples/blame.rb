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

    opts.on "--git-dir=DIR" do |dir|
      options.repodir = dir
    end

    opts.on "-L", String do |lines|
      if lines.match(/(\d+),(\d+)/)
        options.start_line = $1
        options.end_line = $2
      else
        raise "-L format error"
      end
    end

    opts.on "-M" do |skip|
      options.m = true
    end

    opts.on "-C" do
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

blame = Rugged::Blame.new(repo, options.path)
blob = repo.rev_parse "HEAD:#{options.path}"


break_on_nil_hunk = false
blob.content.lines.each_with_index do |line, i|
  hunk = blame.for_line(i+1)

  if hunk
    sig = "#{hunk[:final_signature][:name]} <#{hunk[:final_signature][:email]}>"
    printf("%s (%-30s %3d) %.*s", hunk[:final_commit_id][0..7], sig, i+1, line.size, line)
  end
end