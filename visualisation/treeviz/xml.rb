#!/usr/bin/env ruby

outfile = ARGV[0]
out = IO.read(outfile)

path = []

rbranches = []

id = 0

time = Time.at(0)
step = 24 * 60 * 60

out.scan(/Search(Action:bt|Assign:.*$)/) { |text|
    if text.to_s == "Action:bt"
        path.pop
        time -= step
        puts "<File name=\"backtrack\" id=\"-#{id}\" created=\"" + time.strftime("%Y-%m-%d %H:%M:%S") + "\"/>"
        puts "</Folder>"
        while rbranches.last == path.last
            rbranches.pop
            path.pop
            time -= step
            puts "</Folder>"
        end
        rbranches << path.last
    else
        time += step
        lab = text.to_s[/[0-9]+.+$/]
        puts "<Folder name=\"#{lab}\" id=\"#{id}\" created=\"" + time.strftime("%Y-%m-%d %H:%M:%S") + "\">"
        path << id
        id += 1
    end
}
puts "<File name=\"solution!\" id=\"-#{id}\" created=\"" + time.strftime("%Y-%m-%d %H:%M:%S") + "\"/>"
path.length.times { puts "</Folder>" }
