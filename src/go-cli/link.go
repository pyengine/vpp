package main

import (
	"go-cli/cmd"
	"go-cli/fdio"
)

func init() {
	// Add client generated commands to cobra's root cmd.
	cmd.RootCmd.AddCommand(fdio.InterfaceClientCommand)
}
