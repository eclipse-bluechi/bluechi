# SPDX-License-Identifier: CC0-1.0

require "dbus"

sysbus = DBus.system_bus

bc_service = sysbus["org.eclipse.bluechi"]
bc_object = bc_service["/org/eclipse/bluechi"]
bc_interface = bc_object["org.eclipse.bluechi.Manager"]
nodes = bc_interface.ListNodes

puts "BlueChi nodes:"
puts "================"
nodes.each do |node|
  print "Name: ", node[0], "\n"
  print "Path: ", node[1], "\n"
  print "Status: ", node[2], "\n\n"
end
