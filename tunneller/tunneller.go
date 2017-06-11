package main

import (
	"flag"
	"fmt"
	"golang.org/x/crypto/ssh"
	"io"
	"io/ioutil"
	"log"
	"net"
	"os"
	"time"
)

type Endpoint struct {
	Host string
	Port int
}

func (endpoint *Endpoint) String() string {
	return fmt.Sprintf("%s:%d", endpoint.Host, endpoint.Port)
}

// From https://sosedoff.com/2015/05/25/ssh-port-forwarding-with-go.html
// Handle local client connections and tunnel data to the remote server
// Will use io.Copy - http://golang.org/pkg/io/#Copy
func handleConnection(connection net.Conn, remote net.Conn) {
	defer connection.Close()
	chDone := make(chan bool)

	go func() {
		_, err := io.Copy(connection, remote)
		if err != nil {
			log.Println("Error copying remote->local: %s", err)
		}
		chDone <- true
	}()

	go func() {
		_, err := io.Copy(remote, connection)
		if err != nil {
			log.Println("Error copying local->remote: %s", err)
		}
		chDone <- true
	}()

	<-chDone
}

func publicKeyFile(file string) ssh.AuthMethod {
	buffer, err := ioutil.ReadFile(file)
	if err != nil {
		log.Fatalf("Error reading key: %s", file)
		return nil
	}

	key, err := ssh.ParsePrivateKey(buffer)
	if err != nil {
		log.Fatalf("Error reading key: %s", file)
		return nil
	}
	return ssh.PublicKeys(key)
}

func main() {
	var keyFile string
	var logFile string
	var localEndpoint = Endpoint{
		Host: "localhost",
		Port: 22,
	}
	var serverEndpoint = Endpoint{
		Host: "",
		Port: 22,
	}
	var remoteEndpoint = Endpoint{
		Host: "localhost",
		Port: 7000,
	}

	flag.StringVar(&keyFile, "key", "", "private key file")
	flag.StringVar(&logFile, "log", "tunneller.log", "log file")
	flag.StringVar(&serverEndpoint.Host, "server", "", "server to expose the local service")
	flag.IntVar(&remoteEndpoint.Port, "remote-port", 7000, "port that will forward to local port")
	flag.IntVar(&localEndpoint.Port, "local-port", 22, "port to accept incoming connections")
	flag.Parse()

	if keyFile == "" || remoteEndpoint.Host == "" {
		flag.PrintDefaults()
		os.Exit(2)
	}

	f, err := os.OpenFile(logFile, os.O_RDWR | os.O_CREATE | os.O_APPEND, 0666)
	if err != nil {
		log.Fatalf("Error opening file: %v", err)
	}
	defer f.Close()

	log.SetOutput(f)

	sshConfig := &ssh.ClientConfig{
		User: "ubuntu",
		Auth: []ssh.AuthMethod{
			publicKeyFile(keyFile),
		},
		HostKeyCallback: ssh.InsecureIgnoreHostKey(),
	}

	for {
		serverConnection, err := ssh.Dial("tcp", serverEndpoint.String(), sshConfig)
		if err != nil {
			log.Printf("Unable to connect to remote server: %s", err)
		} else {
			log.Printf("Connected")

			listener, err := serverConnection.Listen("tcp", remoteEndpoint.String())
			if err != nil {
				log.Printf("Unable to listen on %s: %s", remoteEndpoint, err)
			} else {
				defer listener.Close()

				for {
					local, err := net.Dial("tcp", localEndpoint.String())
					if err != nil {
						log.Printf("Unable to connect to local server: %s", err)
					} else {
						clientConnection, err := listener.Accept()
						if err != nil {
							log.Printf("Error %s", err)
							break
						}

						handleConnection(clientConnection, local)
					}
				}
			}
		}

		time.Sleep(1 * time.Second)
	}
}
