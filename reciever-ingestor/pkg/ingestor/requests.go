package ingestor

import (
	"4bit/lora_rx_ingestor/config"
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"log"
	"net/http"
)

type StatePostRequest struct {
	BarometerState *BarometerState
	Power          *PowerState
}

type PowerState struct {
	Current_mA  float32
	LoadVoltage float32
	Power_mW    float32
}

// Barometer data.
type BarometerState struct {
	Pressure    float32
	Temperature float32
	Altitude    float32
}

func PingApi(client *http.Client) error {
	endpoint := fmt.Sprintf("https://%s/ping", config.API_SERVER_ENDPOINT)
	req, err := http.NewRequest(http.MethodGet, endpoint, bytes.NewBufferString(""))
	if err != nil {
		return fmt.Errorf("failed to construct request")
	}

	log.Println("Invoking a GET request to endpoint:", endpoint)
	resp, err := client.Do(req)
	if err != nil {
		return fmt.Errorf("failed to invoke GET request with server")
	}
	defer resp.Body.Close()

	body, _ := io.ReadAll(resp.Body)
	log.Printf("Response status from the server %d with content: %s", resp.StatusCode, body)
	return nil
}

func PostNode(client *http.Client) error {
	endpoint := fmt.Sprintf("https://%s/node", config.API_SERVER_ENDPOINT)
	req, err := http.NewRequest(http.MethodPost, endpoint, bytes.NewBufferString(""))
	if err != nil {
		return fmt.Errorf("failed to construct request")
	}

	log.Println("Invoking a POST request to endpoint:", endpoint)
	resp, err := client.Do(req)
	if err != nil {
		return fmt.Errorf("failed to invoke POST request with server")
	}
	defer resp.Body.Close()

	body, _ := io.ReadAll(resp.Body)
	log.Printf("Response status from the server %d with content: %s", resp.StatusCode, body)
	return nil
}

func PostNodeState(client *http.Client, state MessagePacket) error {
	// Create request body.
	reqBody := StatePostRequest{
		BarometerState: &BarometerState{
			Pressure:    state.Pressure,
			Temperature: state.Temperature,
			Altitude:    state.Altitude,
		},
		Power: &PowerState{
			Current_mA:  state.Current_mA,
			LoadVoltage: state.LoadVoltage,
			Power_mW:    state.Power_mW,
		},
	}
	bodyBuffer, _ := json.Marshal(reqBody)

	// Ship it.
	endpoint := fmt.Sprintf("https://%s/node/state", config.API_SERVER_ENDPOINT)
	req, err := http.NewRequest(http.MethodPost, endpoint, bytes.NewBuffer(bodyBuffer))
	if err != nil {
		return fmt.Errorf("failed to construct request")
	}

	log.Println("Invoking a POST request to endpoint:", endpoint)
	resp, err := client.Do(req)
	if err != nil {
		return fmt.Errorf("failed to invoke POST request with server")
	}
	defer resp.Body.Close()
	log.Printf("Response status from the server %d", resp.StatusCode)
	return nil
}
