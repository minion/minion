#!/usr/bin/env ruby

require 'xmlrpc/client'

outfile = ARGV[0]
out = IO.read(outfile)

path = []
pathEdges = []

rbranches = []

id = 0

size = 2.0
step = size / out[/\<.+?\>$/].gsub(/\{.+?\}/, "").split(/,/).size

server = XMLRPC::Client.new2("http://127.0.0.1:20738/RPC2")
server.call("ubigraph.clear")
server.call("ubigraph.set_edge_style_attribute", 0, "oriented", "true")
server.call("ubigraph.set_vertex_style_attribute", 0, "shape", "sphere")
out.scan(/Search(Action:bt|Assign:.*$)/) { |text|
    if text.to_s == "Action:bt"
        server.call("ubigraph.set_vertex_attribute", id - 1, "color", "#FF0000")
        osize = path.size
        path.pop
        pathEdges.pop
        while rbranches.last == path.last
            rbranches.pop
            path.pop
            pathEdges.pop
        end
        rbranches << path.last
        size += step * (osize - path.size)
    else
        #puts "ubigraph.new_vertex_w_id " + id.to_s
        #puts "ubigraph.new_edge " + path.last.to_s + " " + id.to_s unless path.empty?
        server.call("ubigraph.new_vertex_w_id", id)
        lab = text.to_s[/[0-9]+.+$/]
        #server.call("ubigraph.set_vertex_attribute", id, "label", lab)
        #server.call("ubigraph.set_vertex_attribute", id, "size", size.to_s)
        if not path.empty?
            eid = server.call("ubigraph.new_edge", path.last, id)
            pathEdges << eid
            #server.call("ubigraph.set_edge_attribute", eid, "strength", Math.exp(size).to_s)
        end
        path << id
        id += 1
        size -= step
    end
    #sleep 0.03
}
path.each { |node|
    server.call("ubigraph.set_vertex_attribute", node, "color", "#00FF00")
}
#server.call("ubigraph.set_vertex_attribute", id - 1, "size", "3.0")
server.call("ubigraph.set_vertex_attribute", 0, "size", "3.0")
pathEdges.each { |edge|
    server.call("ubigraph.set_edge_attribute", edge, "color", "#00FF00")
    server.call("ubigraph.set_edge_attribute", edge, "width", "3.0")
}
