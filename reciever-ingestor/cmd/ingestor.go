package cmd

import (
	"4bit/lora_rx_ingestor/pkg/ingestor"
	"fmt"

	cobra "github.com/spf13/cobra"
)

func handleIngestorCommand(cmd *cobra.Command, args []string) error {
	// Construct the ingestor client.
	clientCertFile := cmd.PersistentFlags().Lookup("cert").Value.String()
	clientKeyFile := cmd.PersistentFlags().Lookup("key").Value.String()
	trustedCasDir := cmd.PersistentFlags().Lookup("trusted-cas").Value.String()
	file := cmd.PersistentFlags().Lookup("file").Value.String()
	endpoint := cmd.PersistentFlags().Lookup("endpoint").Value.String()

	// Create ingestor client.
	fmt.Printf("Ingesting from '%s' file onto endpoint %s\n", file, endpoint)
	var ingestorClient ingestor.Ingestor
	if clientCertFile == "" || clientKeyFile == "" || trustedCasDir == "" {
		fmt.Println("Starting insecure ingestor client")
		client, err := ingestor.NewInsecureIngestor(file, endpoint)
		if err != nil {
			return fmt.Errorf("failed to create insecure client: %v", err)
		}
		ingestorClient = *client
	} else {
		fmt.Println("Starting secure ingestor client")
		client, err := ingestor.NewSecureIngestor(
			file,
			endpoint,
			ingestor.IngestorCredentials{
				ClientCertificateFile:    &clientCertFile,
				ClientCertificateKeyFile: &clientKeyFile,
				TrustedCasDir:            &trustedCasDir,
			},
		)
		if err != nil {
			return fmt.Errorf("failed to create a secure client: %v", err)
		}
		ingestorClient = *client
	}

	// Verify connection with api server.
	if err := ingestor.PingApi(ingestorClient.HttpClient); err != nil {
		return fmt.Errorf("failed to ping api server: %v", err)
	}

	// Ensure the node has been published to the server.
	if err := ingestor.PostNode(ingestorClient.HttpClient); err != nil {
		return fmt.Errorf("failed to create new node entry in api server: %v", err)
	}

	// Start ingesting!
	if err := ingestorClient.Start(); err != nil {
		return fmt.Errorf("client failed: %v", err)
	}
	return nil
}

func NewIngestorCommand() *cobra.Command {
	cmd := &cobra.Command{
		Use:   "client",
		Short: "Starts the ingestor client with the given options.",
		RunE:  handleIngestorCommand,
	}

	cmd.PersistentFlags().String("cert", "", "Path to client certificate.")
	cmd.PersistentFlags().String("key", "", "Path to client key.")
	cmd.PersistentFlags().String("trusted-cas", "", "Path to a directory of trusted CA.")
	cmd.PersistentFlags().String("endpoint", "localhost:3000", "Endpoint to send ingested messages to")
	cmd.PersistentFlags().String("file", "", "File to ingest from")
	cmd.MarkPersistentFlagRequired("file")

	return cmd
}
