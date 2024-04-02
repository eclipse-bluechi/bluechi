// Copyright Contributors to the Eclipse BlueChi project
//
// SPDX-License-Identifier: MIT-0

package main

import (
	"fmt"
	"os"
	"strconv"

	"github.com/godbus/dbus/v5"
)

const (
	BcDusInterface          = "org.eclipse.bluechi"
	BcObjectPath            = "/org/eclipse/bluechi"
	MethodGetNode           = "org.eclipse.bluechi.Controller.GetNode"
	MethodSetUnitProperties = "org.eclipse.bluechi.Node.SetUnitProperties"
)

func main() {
	if len(os.Args) != 4 {
		fmt.Printf("Usage: %s node_name unit_name cpu_weight\n", os.Args[0])
		os.Exit(1)
	}

	nodeName := os.Args[1]
	unitName := os.Args[2]
	cpuWeight, err := strconv.ParseUint(os.Args[3], 10, 64)
	if err != nil {
		fmt.Printf("Failed to parse cpu weight to uint from value '%s'\n", os.Args[3])
		os.Exit(1)
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

	values := []struct {
		UnitName string
		Value    interface{}
	}{
		{
			UnitName: "CPUWeight",
			Value:    dbus.MakeVariant(cpuWeight),
		},
	}
	runtime := true

	nodeObject := conn.Object(BcDusInterface, dbus.ObjectPath(nodePath))
	err = nodeObject.Call(MethodSetUnitProperties, 0, unitName, runtime, values).Store()
	if err != nil {
		fmt.Fprintln(os.Stderr, "Failed to set cpu weight:", err)
		os.Exit(1)
	}
}
