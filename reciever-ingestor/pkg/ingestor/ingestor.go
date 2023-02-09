package ingestor

import (
	"crypto/tls"
	"crypto/x509"
	"fmt"
	"io"
	"io/ioutil"
	"net/http"
	"os"
	"regexp"
	"strconv"
	"time"

	tail "github.com/hpcloud/tail"
)

type IngestorCredentials struct {
	ClientCertificateFile    *string
	ClientCertificateKeyFile *string
	TrustedCaFile            *string
}

type Ingestor struct {
	IngestFile  string
	Endpoint    string
	Credentials *IngestorCredentials
	HttpClient  *http.Client
}

// Initializes an insecure ingestor client using the file to ingest and
// endpoint to send the ingested data to.
func NewInsecureIngestor(ingestFile string, endpoint string) (*Ingestor, error) {
	// Verify that the file exists.
	if _, err := os.Stat(ingestFile); os.IsNotExist(err) {
		return nil, fmt.Errorf("ingest file '%s' does not exist", ingestFile)
	}

	// Create the http client.
	client := http.Client{
		Timeout: 5 * time.Second,
	}

	return &Ingestor{
		IngestFile:  ingestFile,
		Endpoint:    endpoint,
		Credentials: nil,
		HttpClient:  &client,
	}, nil
}

// Initializes an secure ingestor client using the file to ingest and
// endpoint to send the ingested data to.
func NewSecureIngestor(ingestFile string, endpoint string, credentials IngestorCredentials) (*Ingestor, error) {
	// Construct base ingestor.
	ingestorClient, err := NewInsecureIngestor(ingestFile, endpoint)
	if err != nil {
		return nil, err
	}

	// Load in client certs.
	cert, err := tls.LoadX509KeyPair(
		*credentials.ClientCertificateFile,
		*credentials.ClientCertificateKeyFile,
	)
	if err != nil {
		return nil, fmt.Errorf(
			"failed to create an x509 keypair from client cert file %s and client key file %s",
			*credentials.ClientCertificateFile,
			*credentials.ClientCertificateKeyFile,
		)
	}

	// Load in trusted CA.
	caCrtContent, err := ioutil.ReadFile(*credentials.TrustedCaFile)
	if err != nil {
		return nil, fmt.Errorf("failed to read the contents of CA cert %s", *credentials.TrustedCaFile)
	}

	// Create a CA certificate pool, in order for the certificate to be validated.
	caCrtPool := x509.NewCertPool()
	caCrtPool.AppendCertsFromPEM(caCrtContent)

	// Apply TLS to the client.
	ingestorClient.HttpClient.Transport = &http.Transport{
		TLSClientConfig: &tls.Config{
			Certificates: []tls.Certificate{cert},
			RootCAs:      caCrtPool,
		},
	}

	return ingestorClient, nil
}

type MessagePacket struct {
	// Barometer data.
	Pressure    float32
	Temperature float32
	Altitude    float32

	// Power consumption data.
	Current_mA  float32
	LoadVoltage float32
	Power_mW    float32

	// Error state.
	Error error
}

// Starts the ingestor client.
func (ingestorClient *Ingestor) Start() error {
	messages := make(chan MessagePacket)
	errCounter, errMaxCounter := 0, 10

	// Start poller.
	go startFilePoller(ingestorClient.IngestFile, messages)

	for msg := range messages {
		// Check on errors until fatal.
		if msg.Error != nil {
			fmt.Printf(
				"[%d/%d]failed to poll '%s': %v\n",
				errCounter,
				errMaxCounter,
				ingestorClient.IngestFile,
				msg.Error,
			)
			errCounter += 1
			if errCounter > errMaxCounter {
				return fmt.Errorf("exhausted file poller error counter")
			}
			continue
		}

		// TODO: send messages to endpoint.
		fmt.Println("Ingested message:")
		fmt.Printf(" - Pressure: %.2f\n", msg.Pressure)
		fmt.Printf(" - Temperature: %.2f\n", msg.Temperature)
		fmt.Printf(" - Altitude: %.2f\n", msg.Altitude)
		fmt.Printf(" - Current_mA: %.2f\n", msg.Current_mA)
		fmt.Printf(" - LoadVoltage: %.2f\n", msg.LoadVoltage)
		fmt.Printf(" - Power_mW: %.2f\n", msg.Power_mW)
	}

	return nil
}

// Attemps to parse the buffer. If all matches, then a message packet is returned
// else nothing.
// TODO: improve this.
func parseBuffer(buffer *string) *MessagePacket {
	// Construct expected regexes.
	pressureRe := regexp.MustCompile(`pressure: (-?\d+\.\d+)`)
	temperatureRe := regexp.MustCompile(`temperature: (-?\d+\.\d+)`)
	altitudeRe := regexp.MustCompile(`altitude: (-?\d+\.\d+)`)
	currentRe := regexp.MustCompile(`current_mA: (-?\d+\.\d+)`)
	voltageRe := regexp.MustCompile(`loadVoltage: (-?\d+\.\d+)`)
	powerRe := regexp.MustCompile(`power_mW: (-?\d+\.\d+)`)

	// Extract each entry.
	pressureMatch := pressureRe.FindStringSubmatch(*buffer)
	temperatureMatch := temperatureRe.FindStringSubmatch(*buffer)
	altitudeMatch := altitudeRe.FindStringSubmatch(*buffer)
	currentMatch := currentRe.FindStringSubmatch(*buffer)
	voltageMatch := voltageRe.FindStringSubmatch(*buffer)
	powerMatch := powerRe.FindStringSubmatch(*buffer)

	// Verify all entries match
	if len(pressureMatch) == 0 {
		return nil
	}
	if len(temperatureMatch) == 0 {
		return nil
	}
	if len(altitudeMatch) == 0 {
		return nil
	}
	if len(currentMatch) == 0 {
		return nil
	}
	if len(voltageMatch) == 0 {
		return nil
	}
	if len(powerMatch) == 0 {
		return nil
	}

	// Assign matches and return parsed message.
	pressure, _ := strconv.ParseFloat(pressureMatch[len(pressureMatch)-1], 32)
	temperature, _ := strconv.ParseFloat(temperatureMatch[len(temperatureMatch)-1], 32)
	altitude, _ := strconv.ParseFloat(altitudeMatch[len(altitudeMatch)-1], 32)
	current, _ := strconv.ParseFloat(currentMatch[len(currentMatch)-1], 32)
	voltage, _ := strconv.ParseFloat(voltageMatch[len(voltageMatch)-1], 32)
	power, _ := strconv.ParseFloat(powerMatch[len(powerMatch)-1], 32)

	return &MessagePacket{
		Pressure:    float32(pressure),
		Temperature: float32(temperature),
		Altitude:    float32(altitude),
		Current_mA:  float32(current),
		LoadVoltage: float32(voltage),
		Power_mW:    float32(power),
	}
}

// Polls given file yielding parsed messages.
func startFilePoller(file string, messages chan MessagePacket) {
	// Init tailing the file.
	t, err := tail.TailFile(file, tail.Config{
		MustExist: true,
		Follow:    true,
		Location: &tail.SeekInfo{
			Whence: io.SeekEnd,
		},
	})
	if err != nil {
		msg := MessagePacket{
			Error: fmt.Errorf("failed to tail file: %v", err),
		}
		messages <- msg
		return
	}

	// Start parsing!
	buffer := ""
	maxBufferSize := 4096

	for line := range t.Lines {
		buffer += line.Text
		if msg := parseBuffer(&buffer); msg != nil {
			messages <- *msg

			// Reset buffer.
			buffer = ""
		}

		// Ensure buffer doesn't go over the max limit.
		if len(buffer) > maxBufferSize {
			pkt := MessagePacket{}
			pkt.Error = fmt.Errorf("buffer overflow without any matches. resetting buffer")
			messages <- pkt
			buffer = ""
		}
	}
}
