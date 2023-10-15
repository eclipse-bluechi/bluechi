// SPDX-License-Identifier: LGPL-2.1-or-later

package main

import (
	"fmt"
	"os"

	"github.com/godbus/dbus/v5"
	"github.com/spf13/cobra"
)

// service, object and register interface for basic client connection
const (
	BcDusInterface = "org.eclipse.bluechi"
	BcObjectPath   = "/org/eclipse/bluechi/internal"
	MethodRegister = "org.eclipse.bluechi.internal.Manager.Register"
)

// object path and signals of internal agents interface
// see https://github.com/containers/bluechi/blob/main/data/org.eclipse.bluechi.internal.Agent.xml#L81-L116
const (
	InternalAgentObject       = "/org/eclipse/bluechi/internal/agent"
	InternalAgentSignalPrefix = "org.eclipse.bluechi.internal.Agent."
)

func usageAndExit() {
	message := `
    ./bluechi-tester
        --url tcp:host=10.90.0.4,port=842
        --nodename NODE_AGENT_NAME
        --signal JobDone
        --numbersignals 500`

	fmt.Printf("\nUsage Example:\n" + message + "\n")
	fmt.Printf("\nKeep in mind that NODE_AGENT_NAME must be in the AllowedNodeNames in the controller configuration\n")
	os.Exit(1)
}

func main() {
	var urlValue string
	var nodeName string
	var signalName string
	var numberSignals int

	var rootCmd = &cobra.Command{Use: "bluechi-tester"}
	rootCmd.PersistentFlags().StringVarP(&urlValue, "url", "u", "", "Specify a URL")
	rootCmd.PersistentFlags().StringVarP(&nodeName, "nodename", "n", "", "Specify a node name")
	rootCmd.PersistentFlags().StringVarP(&signalName, "signal", "s", "", "Specify a signal name")
	rootCmd.PersistentFlags().IntVarP(&numberSignals, "numbersignals", "N", 1, "Number of signals to be sent")

	rootCmd.RunE = func(cmd *cobra.Command, args []string) error {

		if urlValue == "" {
			fmt.Println("Error: --url is required but not provided.")
			usageAndExit()
			os.Exit(1)
		}

		if nodeName == "" {
			fmt.Println("Error: --nodename is required but not provided.")
			usageAndExit()
			os.Exit(1)
		}

		if signalName == "" {
			fmt.Println("Error: --signal is required but not provided.")
			usageAndExit()
			os.Exit(1)
		}

		if signalName != "" && signalName != "JobDone" {
			fmt.Printf("Invalid signal name: %s. Only 'JobDone' is accepted.\n", signalName)
			os.Exit(1)
		}

		if numberSignals == 0 {
			numberSignals = 1
		}
		fmt.Println("Connecting to controller:", urlValue)
		conn, err := dbus.Connect(urlValue)
		if err != nil {
			fmt.Fprintln(os.Stderr, "Failed to connect to system bus:", err)
			os.Exit(1)
		}
		defer conn.Close()

		busObject := conn.Object(BcDusInterface, BcObjectPath)
		err = busObject.Call(MethodRegister, 0, nodeName).Store()
		if err != nil {
			fmt.Fprintln(os.Stderr, "Failed to register:", err)
			os.Exit(1)

		}

		if signalName == "JobDone" {
			fmt.Println("Sending " + fmt.Sprint(numberSignals) + " " + signalName + " signal to " + urlValue)
			// emit a JobDone signal, results in an error log on the controller side since JobID 1 or (N) is unknown
			for i := 0; i < numberSignals; i++ {
				err = conn.Emit(InternalAgentObject, InternalAgentSignalPrefix+signalName, 1, "fake-state")
				if err != nil {
					fmt.Fprintln(os.Stderr, "Failed to emit signal")
				}
			}
		}
		return nil
	}
	if err := rootCmd.Execute(); err != nil {
		usageAndExit()
	}
}
