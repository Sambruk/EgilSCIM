/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2019 Föreningen Sambruk
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.

 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// Package main is the starting point of the EGIL Test server
package main

import (
	"bytes"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"os/exec"
	"path"
	"strconv"
	"strings"
	"time"
)

var testRoot string
var egilBinaryPath string

func readerToString(r io.Reader) string {
	buf := new(bytes.Buffer)
	buf.ReadFrom(r)
	return buf.String()
}

var genericHandlerFailWith int

func genericSCIMHandler(w http.ResponseWriter, r *http.Request, endpoint string, requestLogger io.Writer) {
	fmt.Fprintln(requestLogger, "---")
	fmt.Fprintf(requestLogger, "%s %s\n", endpoint, r.Method)
	io.Copy(requestLogger, r.Body)
	fmt.Fprintln(requestLogger, "---")

	if genericHandlerFailWith != 0 {
		http.Error(w, "Failing test step", genericHandlerFailWith)
		return
	}
	if (r.Method == "POST" || r.Method == "PUT") && r.Header.Get("Content-Type") != "application/scim+json" {
		log.Println("Invalid media-type!")
		http.Error(w, "Bad media type", 415)
	} else if r.Method == "DELETE" {
		w.WriteHeader(204)
	} else if r.Method == "GET" {
		http.Error(w, "Not implemented", http.StatusNotImplemented)
	} else {
		fmt.Fprintln(w, "Hello, World")
	}
}

// TestLogger is a Writer used to save all requests from the client
type TestLogger struct {
	buffer strings.Builder
}

// Reset throws away previously saved requests
func (tl *TestLogger) Reset() {
	tl.buffer.Reset()
}

func (tl *TestLogger) Write(p []byte) (int, error) {
	return tl.buffer.Write(p)
}

func (tl *TestLogger) String() string {
	return tl.buffer.String()
}

func listenAndServe(handler http.Handler, cert, key string, ch chan error) {
	srv := &http.Server{
		Addr:    ":8000",
		Handler: handler,
	}
	ch <- srv.ListenAndServeTLS(cert, key)
}

// TestStep represents one test step (a scenario and expected requests)
type TestStep struct {
	Scenario     []string
	Requests     string
	FailWith     int
	ExpectErrors bool
}

// TestSpec represents a whole test (consisting of possibly multiple steps)
type TestSpec struct {
	Description string
	Config      string
	Steps       []TestStep
}

func runTest(testName, testPath string,
	testLogger *TestLogger,
	serverErrorChannel chan error,
	cert string,
	key string) {

	specRaw, err := ioutil.ReadFile(path.Join(testPath, "spec.json"))
	if err != nil {
		log.Fatalf("Failed to read test spec for %s: %v", testPath, err)
	}

	var testSpec TestSpec
	err = json.Unmarshal(specRaw, &testSpec)

	if err != nil {
		log.Fatalf("Failed to parse test spec json: %v", err)
	}

	// Reset LDAP contents
	cmd := exec.Command(path.Join(testRoot, "scripts", "reset_ldap"))
	err = cmd.Run()

	if err != nil {
		log.Fatalf("Failed to reset LDAP: %v", err)
	}

	fmt.Printf("Running test '%s' - %s\n", testName, testSpec.Description)

	cacheFile, err := ioutil.TempFile("", "egiltest.cache")

	if err != nil {
		log.Fatalf("Failed to create temporary cache file: %v", err)
	}

	// We don't actually want the file, just a unique name
	cacheFile.Close()
	os.Remove(cacheFile.Name())

	// Remove again after the test
	defer os.Remove(cacheFile.Name())

	for idx, step := range testSpec.Steps {

		// Check if our listener has reported an error
		select {
		case err := <-serverErrorChannel:
			log.Fatalf("Error while listening on socket: %v", err)
		default:
		}

		// Apply scenario
		for _, scen := range step.Scenario {
			cmd := exec.Command(path.Join(testRoot, "scripts", "apply_scenario"), scen)
			err = cmd.Run()

			if err != nil {
				log.Fatalf("Failed to apply scenario: %v", err)
			}
		}

		genericHandlerFailWith = step.FailWith

		testLogger.Reset()

		// Run EGIL client
		cmd = exec.Command(egilBinaryPath,
			path.Join(testRoot, testSpec.Config),
			"--cache-file="+cacheFile.Name(),
			"--cert="+cert,
			"--key="+key,
			"--scim-auth-WEAK=true")

		var stderr strings.Builder
		cmd.Stderr = &stderr
		err = cmd.Run()

		if err != nil {
			log.Fatalf("Failed to run EGIL client: %v", err)
		}

		// Verify that the requests match
		requestsString := ""
		if step.Requests != "" {
			var requestsRaw []byte
			requestsPath := path.Join(testPath, step.Requests)
			requestsRaw, err = ioutil.ReadFile(requestsPath)

			if err != nil {
				log.Fatalf("Failed to read requests file %s: %v", requestsPath, err)
			}
			requestsString = string(requestsRaw)
		}

		if strings.Trim(requestsString, " \n\t") != strings.Trim(testLogger.String(), " \n\t") {
			log.Printf("Received requests don't match expected requests (step %d).\n", idx)
			expectedFilename := testName + "_" + strconv.Itoa(idx) + "_expected.txt"
			receivedFilename := testName + "_" + strconv.Itoa(idx) + "_received.txt"
			ioutil.WriteFile(expectedFilename, []byte(requestsString), 0644)
			ioutil.WriteFile(receivedFilename, []byte(testLogger.String()), 0644)
			break
		}

		if step.FailWith == 0 && !step.ExpectErrors {
			if stderr.String() != "" {
				log.Printf("Clients printed something on stderr:\n%s", stderr.String())
				break
			}
		} else {
			if stderr.String() == "" {
				log.Printf("Expected errors on stderr, but client was silent!")
				break
			}
		}
	}
}

func findSubDirectories(p string) []string {
	files, err := ioutil.ReadDir(p)
	if err != nil {
		log.Fatalf("Failed to read directory: %v", err)
	}

	var result []string
	for _, f := range files {
		if f.IsDir() {
			result = append(result, f.Name())
		}
	}
	return result
}

func runTestSuite(testLogger *TestLogger, serverErrorChannel chan error, cert, key string) {
	testSuitePath := path.Join(testRoot, "tests")

	// Restart LDAP server
	cmd := exec.Command(path.Join(testRoot, "scripts", "start_test_ldap"))
	err := cmd.Run()

	if err != nil {
		log.Fatalf("Failed to start LDAP server: %v", err)
	}

	// Wait for LDAP to be properly initialized and ready to go
	i := 0
	for ; i < 10; i++ {
		cmd = exec.Command(path.Join(testRoot, "scripts", "ldap_isup"))
		err = cmd.Run()

		if err == nil {
			break
		}
		time.Sleep(1 * time.Second)
	}

	if i == 10 {
		log.Fatal("Failed to connect to LDAP")
	}

	// Give it some extra time...
	time.Sleep(3 * time.Second)

	testDirectories := findSubDirectories(testSuitePath)

	for _, dir := range testDirectories {
		runTest(dir, path.Join(testSuitePath, dir), testLogger, serverErrorChannel, cert, key)
	}
}

func main() {
	var certp = flag.String("cert", "", "path to server certificate")
	var keyp = flag.String("key", "", "path to server private key")
	var testrootp = flag.String("testroot", "", "path to root of test directory")
	var binaryp = flag.String("binary", "", "path to EGIL client binary")

	flag.Parse()

	testRoot = *testrootp
	egilBinaryPath = *binaryp

	endpoints := []string{"Users", "StudentGroups", "Organisations",
		"SchoolUnits", "SchoolUnitGroups", "Employments", "Activities", "Subjects", "Courses"}

	var testLogger TestLogger
	mux := http.NewServeMux()
	for _, endpoint := range endpoints {
		var logger io.Writer
		if *testrootp == "" {
			logger, _ = os.Create(endpoint + ".log")
		} else {
			logger = &testLogger
		}
		ep := endpoint
		mux.HandleFunc("/"+endpoint,
			func(w http.ResponseWriter, r *http.Request) { genericSCIMHandler(w, r, ep, logger) })
		mux.HandleFunc("/"+endpoint+"/",
			func(w http.ResponseWriter, r *http.Request) { genericSCIMHandler(w, r, ep, logger) })
	}

	ch := make(chan error)
	go listenAndServe(mux, *certp, *keyp, ch)

	if *testrootp == "" {
		err := <-ch
		log.Fatal(err)
	} else {
		runTestSuite(&testLogger, ch, *certp, *keyp)
	}
}
