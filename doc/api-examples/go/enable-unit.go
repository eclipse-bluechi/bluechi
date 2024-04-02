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
	MethodEnableUnitFiles = "org.eclipse.bluechi.Node.EnableUnitFiles"
)

func main() {
	if len(os.Args) < 3 {
		fmt.Printf("Usage: %s node_name unit_name...", os.Args[0])
		os.Exit(1)
	}

	nodeName := os.Args[1]
	units := []string{}
	for _, unit := range os.Args[2:] {
		units = append(units, unit)
	}

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

	type ChangesResponse struct {
		OperationType string
		SymlinkFile   string
		SymlinkDest   string
	}

	var carriesInstallInfoResponse bool
	var changes []ChangesResponse
	nodeObject := conn.Object(BcDusInterface, dbus.ObjectPath(nodePath))
	err = nodeObject.Call(MethodEnableUnitFiles, 0, units, false, false).Store(&carriesInstallInfoResponse, &changes)
	if err != nil {
		fmt.Fprintln(os.Stderr, "Failed to enable unit files:", err)
		os.Exit(1)
	}

	if carriesInstallInfoResponse {
		fmt.Println("The unit files included enablement information")
	} else {
		fmt.Println("The unit files did not include any enablement information")
	}

	for _, change := range changes {
		if change.OperationType == "symlink" {
			fmt.Printf("Created symlink %s -> %s\n", change.SymlinkFile, change.SymlinkDest)
		} else if change.OperationType == "unlink" {
			fmt.Printf("Removed '%s'", change.SymlinkFile)
		}
	}
}
