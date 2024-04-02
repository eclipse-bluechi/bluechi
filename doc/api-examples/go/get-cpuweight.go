// Copyright Contributors to the Eclipse BlueChi project
//
// SPDX-License-Identifier: MIT-0

package main

import (
	"fmt"
	"os"

	"github.com/godbus/dbus/v5"
)

const (
	BcDusInterface        = "org.eclipse.bluechi"
	BcObjectPath          = "/org/eclipse/bluechi"
	MethodGetNode         = "org.eclipse.bluechi.Controller.GetNode"
	MethodGetUnitProperty = "org.eclipse.bluechi.Node.GetUnitProperty"
)

func main() {
	if len(os.Args) != 3 {
		fmt.Printf("Usage: %s node_name unit_name", os.Args[0])
		os.Exit(1)
	}

	nodeName := os.Args[1]
	unitName := os.Args[2]

	conn, err := dbus.ConnectSystemBus()
	if err != nil {
		fmt.Fprintln(os.Stderr, "Failed to connect to system bus:", err)
		os.Exit(1)
	}
	defer conn.Close()

	nodePath := ""
	busObject := conn.Object(BcDusInterface, BcObjectPath)
	err = busObject.Call(MethodGetNode, 0, nodeName).Store(&nodePath)
	if err != nil {
		fmt.Fprintln(os.Stderr, "Failed to get node path:", err)
		os.Exit(1)
	}

	var cpuWeight uint64
	nodeObject := conn.Object(BcDusInterface, dbus.ObjectPath(nodePath))
	err = nodeObject.Call(MethodGetUnitProperty, 0, unitName, "org.freedesktop.systemd1.Service", "CPUWeight").Store(&cpuWeight)
	if err != nil {
		fmt.Fprintln(os.Stderr, "Failed to get cpu weight:", err)
		os.Exit(1)
	}

	fmt.Println(cpuWeight)
}
