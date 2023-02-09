package main

import (
	"4bit/lora_rx_ingestor/cmd"
	"log"
)

func main() {
	if err := cmd.Execute(); err != nil {
		log.Fatal(err)
	}
}
