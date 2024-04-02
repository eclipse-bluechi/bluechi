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
	BcDusInterface  = "org.eclipse.bluechi"
	BcObjectPath    = "/org/eclipse/bluechi"
	BcNodeInterface = "org.eclipse.bluechi.Node"

	NodePathPrefix = "/org/eclipse/bluechi/node/"
)

func main() {
	conn, err := dbus.ConnectSystemBus()
	if err != nil {
		fmt.Fprintln(os.Stderr, "Failed to connect to system bus:", err)
		os.Exit(1)
	}
	defer conn.Close()

	busObject := conn.Object(BcDusInterface, BcObjectPath)
	err = busObject.AddMatchSignal("org.freedesktop.DBus.Properties", "PropertiesChanged").Err
	if err != nil {
		fmt.Println("Failed to add signal to node: ", err)
		os.Exit(1)
	}

	c := make(chan *dbus.Signal, 10)
	conn.Signal(c)
	for v := range c {
		path := string(v.Path)
		ifaceName := v.Body[0]
		if strings.HasPrefix(path, NodePathPrefix) && ifaceName == BcNodeInterface {
			changedValues, ok := v.Body[1].(map[string]dbus.Variant)
			if !ok {
				fmt.Println("Received invalid property changed signal")
				continue
			}
			if val, ok := changedValues["Status"]; ok {
				nodeName := strings.Replace(path, NodePathPrefix, "", 1)
				fmt.Printf("Node %s: %s\n", nodeName, val.String())
			}
		}
	}
}
