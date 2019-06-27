/**
 * Copyright © 2019 Föreningen Sambruk
 *
 * This file is part of EgilSCIM.
 *
 * EgilSCIM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EgilSCIM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with EgilSCIM.  If not, see <http://www.gnu.org/licenses/>.
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

func genericSCIMHandler(w http.ResponseWriter, r *http.Request, endpoint string, requestLogger io.Writer) {
	fmt.Fprintln(requestLogger, "---")
	fmt.Fprintf(requestLogger, "%s %s\n", endpoint, r.Method)
	io.Copy(requestLogger, r.Body)
	fmt.Fprintln(requestLogger, "---")
	if r.Method == "POST" && r.Header.Get("Content-Type") != "application/scim+json" {
		log.Println("Invalid media-type!")
		http.Error(w, "Bad media type", 415)
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

func listenAndServe(cert, key string, ch chan error) {
	ch <- http.ListenAndServeTLS(":8000", cert, key, nil)
}

// TestStep represents one test step (a scenario and expected requests)
type TestStep struct {
	Scenario []string
	Requests string
}

// TestSpec represents a whole test (consisting of possibly multiple steps)
type TestSpec struct {
	Description string
	Config      string
	Steps       []TestStep
}

func runTest(testName, testPath string, testLogger *TestLogger, serverErrorChannel chan error) {
	specRaw, err := ioutil.ReadFile(path.Join(testPath, "spec.json"))
	if err != nil {
		log.Fatal(err)
	}

	var testSpec TestSpec
	err = json.Unmarshal(specRaw, &testSpec)

	if err != nil {
		log.Fatal(err)
	}

	// Reset LDAP contents
	cmd := exec.Command(path.Join(testRoot, "scripts", "reset_ldap"))
	err = cmd.Run()

	if err != nil {
		log.Fatal(err)
	}

	fmt.Printf("Running test '%s' - %s\n", testName, testSpec.Description)

	cacheFile, err := ioutil.TempFile("", "egiltest*.cache")

	if err != nil {
		log.Fatal(err)
	}

	defer os.Remove(cacheFile.Name()) // clean up
	cacheFile.Close()

	for _, step := range testSpec.Steps {
		// Apply scenario
		for _, scen := range step.Scenario {
			cmd := exec.Command(path.Join(testRoot, "scripts", "apply_scenario"), scen)
			cmd.Dir = testRoot
			err = cmd.Run()

			if err != nil {
				log.Fatal(err)
			}
		}

		testLogger.Reset()

		// Run EGIL client
		cmd = exec.Command(egilBinaryPath,
			path.Join(testRoot, testSpec.Config),
			"--cache-file="+cacheFile.Name())

		//cmd.Stderr = os.Stderr
		//cmd.Stdout = os.Stdout
		err = cmd.Run()

		if err != nil {
			log.Fatalf("Failed to run EGIL client: %v", err)
		}

		// Verify that the requests match
		requestsString := ""
		if step.Requests != "" {
			var requestsRaw []byte
			requestsRaw, err = ioutil.ReadFile(path.Join(testPath, step.Requests))

			if err != nil {
				log.Fatal(err)
			}
			requestsString = string(requestsRaw)
		}

		if strings.Trim(requestsString, " \n\t") != strings.Trim(testLogger.String(), " \n\t") {
			log.Println("Received requests don't match expected requests. Expected:")
			log.Printf("%s\n", requestsString)
			log.Println("Received:")
			log.Printf("%s\n", testLogger.String())
			break
		}
	}
}

func findSubDirectories(p string) []string {
	files, err := ioutil.ReadDir(p)
	if err != nil {
		log.Fatal(err)
	}

	var result []string
	for _, f := range files {
		if f.IsDir() {
			result = append(result, f.Name())
		}
	}
	return result
}

func runTestSuite(testLogger *TestLogger, serverErrorChannel chan error) {
	testSuitePath := path.Join(testRoot, "tests")

	// Restart LDAP server
	cmd := exec.Command(path.Join(testRoot, "scripts", "start_test_ldap"))
	err := cmd.Run()

	if err != nil {
		log.Fatal(err)
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
		runTest(dir, path.Join(testSuitePath, dir), testLogger, serverErrorChannel)
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
		"SchoolUnits", "SchoolUnitGroups", "Employments", "Activities"}

	var testLogger TestLogger
	for _, endpoint := range endpoints {
		var logger io.Writer
		if *testrootp == "" {
			logger, _ = os.Create(endpoint + ".log")
		} else {
			logger = &testLogger
		}
		http.HandleFunc("/"+endpoint,
			func(w http.ResponseWriter, r *http.Request) { genericSCIMHandler(w, r, endpoint, logger) })
	}

	var ch chan error
	go listenAndServe(*certp, *keyp, ch)

	if *testrootp == "" {
		err := <-ch
		log.Fatal(err)
	} else {
		runTestSuite(&testLogger, ch)
	}
}
