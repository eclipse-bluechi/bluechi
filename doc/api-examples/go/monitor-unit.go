// Copyright Contributors to the Eclipse BlueChi project
//
// SPDX-License-Identifier: MIT-0

package main

import (
	"fmt"
	"os"
	"strings"

	"github.com/godbus/dbus/v5"
)

const (
	BcDusInterface      = "org.eclipse.bluechi"
	BcObjectPath        = "/org/eclipse/bluechi"
	MethodCreateMonitor = "org.eclipse.bluechi.Controller.CreateMonitor"
	MethodSubscribe     = "org.eclipse.bluechi.Monitor.Subscribe"

	SignalUnitNew               = "UnitNew"
	SignalUnitRemoved           = "UnitRemoved"
	SignalUnitPropertiesChanged = "UnitPropertiesChanged"
	SignalUnitStateChanged      = "UnitStateChanged"
)

func main() {
	if len(os.Args) != 2 {
		fmt.Printf("Usage: %s node_name", os.Args[0])
		os.Exit(1)
	}

	nodeName := os.Args[1]
	// use wildcard to match all units
	unitName := "*"

	conn, err := dbus.ConnectSystemBus()
	if err != nil {
		fmt.Fprintln(os.Stderr, "Failed to connect to system bus:", err)
		os.Exit(1)
	}
	defer conn.Close()

	monitorPath := ""
	busObject := conn.Object(BcDusInterface, BcObjectPath)
	err = busObject.Call(MethodCreateMonitor, 0).Store(&monitorPath)
	if err != nil {
		fmt.Fprintln(os.Stderr, "Failed to get node path:", err)
		os.Exit(1)
	}

	monitorObject := conn.Object(BcDusInterface, dbus.ObjectPath(monitorPath))

	err = monitorObject.AddMatchSignal("org.eclipse.bluechi.Monitor", SignalUnitNew).Err
	if err != nil {
		fmt.Println("Failed to add signal to UnitNew: ", err)
		os.Exit(1)
	}
	err = monitorObject.AddMatchSignal("org.eclipse.bluechi.Monitor", SignalUnitRemoved).Err
	if err != nil {
		fmt.Println("Failed to add signal to UnitRemoved: ", err)
		os.Exit(1)
	}
	err = monitorObject.AddMatchSignal("org.eclipse.bluechi.Monitor", SignalUnitPropertiesChanged).Err
	if err != nil {
		fmt.Println("Failed to add signal to UnitPropertiesChanged: ", err)
		os.Exit(1)
	}
	err = monitorObject.AddMatchSignal("org.eclipse.bluechi.Monitor", SignalUnitStateChanged).Err
	if err != nil {
		fmt.Println("Failed to add signal to UnitStateChanged: ", err)
		os.Exit(1)
	}

	var subscriptionID uint64
	err = monitorObject.Call(MethodSubscribe, 0, nodeName, unitName).Store(&subscriptionID)
	if err != nil {
		fmt.Println("Failed to subscribe to all units: ", err)
		os.Exit(1)
	}

	c := make(chan *dbus.Signal, 10)
	conn.Signal(c)
	for v := range c {
		if strings.HasSuffix(v.Name, SignalUnitNew) {
			unitName := v.Body[1]
			reason := v.Body[2]
			fmt.Printf("New Unit %s on node %s, reason: %s\n", unitName, nodeName, reason)
		} else if strings.HasSuffix(v.Name, SignalUnitRemoved) {
			unitName := v.Body[1]
			reason := v.Body[2]
			fmt.Printf("Removed Unit %s on node %s, reason: %s\n", unitName, nodeName, reason)
		} else if strings.HasSuffix(v.Name, SignalUnitPropertiesChanged) {
			unitName := v.Body[1]
			iface := v.Body[2]
			fmt.Printf("Unit %s on node %s changed for iface %s\n", unitName, nodeName, iface)
		} else if strings.HasSuffix(v.Name, SignalUnitStateChanged) {
			unitName := v.Body[1]
			activeState := v.Body[2]
			subState := v.Body[3]
			reason := v.Body[4]
			fmt.Printf("Unit %s on node %s changed to state (%s, %s), reason: %s\n", unitName, nodeName, activeState, subState, reason)
		} else {
			fmt.Println("Unexpected signal name: ", v.Name)
		}
	}
}
