package cmd

import (
	cobra "github.com/spf13/cobra"
)

func Execute() error {
	rootCmd := &cobra.Command{
		Use:           "lora-rx-ingestor",
		Short:         "Ingests received messages from a file, posting results into a given endpoint",
		SilenceErrors: true,
	}
	rootCmd.AddCommand(NewIngestorCommand())
	return rootCmd.Execute()
}
