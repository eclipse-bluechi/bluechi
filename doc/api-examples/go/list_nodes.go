// SPDX-License-Identifier: CC0-1.0

package main

import (
	"fmt"
	"os"
	"github.com/godbus/dbus/v5"
)

// Constants to manage bluechi dbus interface
const (
        BcDusInterface = "org.eclipse.bluechi"
        BcObjectPath = "/org/eclipse/bluechi"
	MethodListNodes = "org.eclipse.bluechi.Manager.ListNodes"
)

func main() {
	var nodes [][]interface{}

	conn, err := dbus.ConnectSystemBus()
	if err != nil {
		fmt.Fprintln(os.Stderr, "Failed to connect to system bus:", err)
		os.Exit(1)
	}
	defer conn.Close()

	busObject := conn.Object(BcDusInterface, BcObjectPath)
	err = busObject.Call(MethodListNodes, 0).Store(&nodes)
	if err != nil {
		fmt.Fprintln(os.Stderr, "Failed to list nodes:", err)
		os.Exit(1)
	}

	for _, node := range nodes {
		fmt.Println("Name:", node[0])
		fmt.Println("Status:", node[2])
		fmt.Println("Object Path:", node[1])
		fmt.Println()
	}
}
