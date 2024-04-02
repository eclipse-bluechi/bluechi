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
	MethodListNodes = "org.eclipse.bluechi.Controller.ListNodes"
)

func main() {
	conn, err := dbus.ConnectSystemBus()
	if err != nil {
		fmt.Fprintln(os.Stderr, "Failed to connect to system bus:", err)
		os.Exit(1)
	}
	defer conn.Close()

	var nodes [][]interface{}
	busObject := conn.Object(BcDusInterface, BcObjectPath)
	err = busObject.Call(MethodListNodes, 0).Store(&nodes)
	if err != nil {
		fmt.Fprintln(os.Stderr, "Failed to list nodes:", err)
		os.Exit(1)
	}

	for _, node := range nodes {
		fmt.Printf("Name: %s, Status: %s\n", node[0], node[2])
	}
}
