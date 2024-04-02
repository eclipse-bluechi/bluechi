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
	BcDusInterface  = "org.eclipse.bluechi"
	BcObjectPath    = "/org/eclipse/bluechi"
	MethodGetNode   = "org.eclipse.bluechi.Controller.GetNode"
	MethodListUnits = "org.eclipse.bluechi.Node.ListUnits"
)

func main() {
	if len(os.Args) != 2 {
		fmt.Println("No node name supplied")
		os.Exit(1)
	}

	nodeName := os.Args[1]

	conn, err := dbus.ConnectSystemBus()
	if err != nil {
		fmt.Fprintln(os.Stderr, "Failed to connect to system bus:", err)
		os.Exit(1)
	}
	defer conn.Close()

	nodePath := ""
	mgrObject := conn.Object(BcDusInterface, BcObjectPath)
	err = mgrObject.Call(MethodGetNode, 0, nodeName).Store(&nodePath)
	if err != nil {
		fmt.Fprintln(os.Stderr, "Failed to get node path:", err)
		os.Exit(1)
	}

	var units [][]interface{}
	nodeObject := conn.Object(BcDusInterface, dbus.ObjectPath(nodePath))
	err = nodeObject.Call(MethodListUnits, 0).Store(&units)
	if err != nil {
		fmt.Fprintln(os.Stderr, "Failed to list units:", err)
		os.Exit(1)
	}

	for _, unit := range units {
		fmt.Printf("%s - %s\n", unit[0], unit[1])
	}
}
