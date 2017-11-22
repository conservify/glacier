package main

import (
	"flag"
	"fmt"
	"golang.org/x/crypto/ssh"
	"io"
	"io/ioutil"
	"log"
	"log/syslog"
	"net"
	"os"
	"sync"
	"time"
)

type Endpoint struct {
	Host string
	Port int
}

func (endpoint *Endpoint) String() string {
	return fmt.Sprintf("%s:%d", endpoint.Host, endpoint.Port)
}

type IdleTimeoutConn struct {
	Conn net.Conn
}

func (self IdleTimeoutConn) Read(buf []byte) (int, error) {
	self.Conn.SetDeadline(time.Now().Add(30 * time.Second))
	return self.Conn.Read(buf)
}

func (self IdleTimeoutConn) Write(buf []byte) (int, error) {
	self.Conn.SetDeadline(time.Now().Add(30 * time.Second))
	return self.Conn.Write(buf)
}

// From https://sosedoff.com/2015/05/25/ssh-port-forwarding-with-go.html
// Handle local client connections and tunnel data to the remote server
// Will use io.Copy - http://golang.org/pkg/io/#Copy
func handleConnection(connection net.Conn, remote net.Conn) {
	chDone := make(chan bool)

	go func() {
		_, err := io.Copy(IdleTimeoutConn{Conn: connection}, IdleTimeoutConn{Conn: remote})
		if err != nil {
			log.Printf("Error copying remote->local: %s", err)
		}
		chDone <- true
	}()

	go func() {
		_, err := io.Copy(IdleTimeoutConn{Conn: remote}, IdleTimeoutConn{Conn: connection})
		if err != nil {
			log.Printf("Error copying local->remote: %s", err)
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

type Options struct {
	User              string
	KeyFile           string
	LogFile           string
	LocalEndpoint     Endpoint
	ServerEndpoint    Endpoint
	RemoteEndpoint    Endpoint
	Reverse           bool
	Syslog            string
	ReconnectInterval int
}

func forwardLocalPortToRemotePort(o *Options, sshConfig *ssh.ClientConfig) {
	log.Printf("Listening on %v...", o.LocalEndpoint.String())

	listener, err := net.Listen("tcp", o.LocalEndpoint.String())
	if err != nil {
		log.Printf("Unable to listen on %v: %s", o.LocalEndpoint, err)
	} else {
		for {
			log.Printf("New connection, opening %s...", o.LocalEndpoint.String())

			clientConnection, err := listener.Accept()
			if err != nil {
				if err != io.EOF {
					log.Printf("Error %s", err)
				}
				break
			} else {
				log.Printf("Connecting to %v...", o.ServerEndpoint.String())

				serverConnection, err := ssh.Dial("tcp", o.ServerEndpoint.String(), sshConfig)
				if err != nil {
					log.Printf("Unable to connect to remote server: %s", err)
				} else {
					local, err := serverConnection.Dial("tcp", o.RemoteEndpoint.String())
					if err != nil {
						log.Printf("Unable to connect to local server: %s", err)
					} else {
						log.Printf("Tunnelling!")

						s := time.Now()

						handleConnection(clientConnection, local)

						local.Close()

						log.Printf("Done after %v", time.Since(s))
					}

					serverConnection.Close()
				}

				clientConnection.Close()
			}

			time.Sleep(1 * time.Second)
		}

		listener.Close()
	}
}

func serviceRemoteToLocalConnections(listener net.Listener, o *Options, sshConfig *ssh.ClientConfig, busy *sync.Mutex) {
	for {
		clientConnection, err := listener.Accept()
		if err != nil {
			if err != io.EOF {
				log.Printf("Error %s", err)
			}
			return
		} else {
			busy.Lock()

			log.Printf("New connection, opening %s...", o.LocalEndpoint.String())

			local, err := net.Dial("tcp", o.LocalEndpoint.String())
			if err != nil {
				log.Printf("Unable to connect to local server: %s", err)
			} else {
				log.Printf("Tunnelling!")

				s := time.Now()

				handleConnection(clientConnection, local)

				local.Close()

				log.Printf("Done after %v", time.Since(s))
			}

			clientConnection.Close()
		}

		busy.Unlock()

		time.Sleep(1 * time.Second)
	}
}

func forwardRemotePortToLocalPort(o *Options, sshConfig *ssh.ClientConfig) {
	log.Printf("Connecting to %v...", o.ServerEndpoint.String())

	serverConnection, err := ssh.Dial("tcp", o.ServerEndpoint.String(), sshConfig)
	if err != nil {
		log.Printf("Unable to connect to remote server: %s", err)
	} else {
		log.Printf("Done, listening on %v...", o.RemoteEndpoint.String())

		listener, err := serverConnection.Listen("tcp", o.RemoteEndpoint.String())
		if err != nil {
			log.Printf("Unable to listen on %v: %s", o.RemoteEndpoint, err)
		} else {
			busy := sync.Mutex{}

			go serviceRemoteToLocalConnections(listener, o, sshConfig, &busy)

			time.Sleep(time.Duration(o.ReconnectInterval) * time.Second)

			busy.Lock()

			listener.Close()
		}

		serverConnection.Close()
	}
}

func main() {
	o := Options{
		LocalEndpoint: Endpoint{
			Host: "127.0.0.1",
			Port: 22,
		},
		ServerEndpoint: Endpoint{
			Host: "",
			Port: 22,
		},
		RemoteEndpoint: Endpoint{
			Host: "127.0.0.1",
			Port: 7000,
		},
		Reverse: false,
	}

	flag.StringVar(&o.User, "user", "ubuntu", "user name")
	flag.StringVar(&o.KeyFile, "key", "", "private key file")
	flag.StringVar(&o.LogFile, "log", "", "log file")
	flag.StringVar(&o.ServerEndpoint.Host, "server", "", "server to expose the local service")
	flag.StringVar(&o.Syslog, "syslog", "", "enable syslog and name the ap")
	flag.IntVar(&o.RemoteEndpoint.Port, "remote-port", 7000, "port that will forward to local port")
	flag.IntVar(&o.LocalEndpoint.Port, "local-port", 22, "port to accept incoming connections")
	flag.IntVar(&o.ReconnectInterval, "reconnect-interval", 300, "how often to force a reconnection")
	flag.BoolVar(&o.Reverse, "reverse", false, "reverse the direction, listen locally")

	flag.Parse()

	if o.KeyFile == "" || o.RemoteEndpoint.Host == "" {
		flag.PrintDefaults()
		os.Exit(2)
	}

	if o.Syslog == "" {
		f, err := os.OpenFile(o.LogFile, os.O_RDWR|os.O_CREATE|os.O_APPEND, 0666)
		if err != nil {
			log.Fatalf("Error opening file: %v", err)
		}
		defer f.Close()
		if o.LogFile != "" {

		log.SetOutput(f)
		}
	} else {
		syslog, err := syslog.New(syslog.LOG_NOTICE, o.Syslog)
		if err == nil {
			log.SetOutput(syslog)
		}
	}

	sshConfig := &ssh.ClientConfig{
		User: o.User,
		Auth: []ssh.AuthMethod{
			publicKeyFile(o.KeyFile),
		},
		HostKeyCallback: ssh.InsecureIgnoreHostKey(),
	}

	for {
		if o.Reverse {
			forwardLocalPortToRemotePort(&o, sshConfig)
		} else {
			forwardRemotePortToLocalPort(&o, sshConfig)
		}

		time.Sleep(1 * time.Second)
	}
}
