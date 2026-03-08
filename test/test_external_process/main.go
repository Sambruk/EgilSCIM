/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2026 Föreningen Sambruk
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

// test_external_process is a test tool for the external process data source
// functionality. It outputs hard-coded test data sets as JSON to stdout.
package main

import (
	"encoding/json"
	"fmt"
	"os"
)

// Each data set is a slice of objects. Values can be strings or arrays of strings.
var dataSets = map[string][]map[string]interface{}{
	"schoolunits1": {
		{
			"id":             "f80e5b0b-af6b-4797-8726-738a06fffc2c",
			"schoolUnitCode": "12345678",
			"name":           "Storskolan",
		},
		{
			"id":             "322dfc6d-fba0-4ce8-8c73-cbe3d46f97e9",
			"schoolUnitCode": "11223344",
			"name":           "Lillskolan",
		},
	},
	"basegroups": {
		{
			"entryUUID": "39074b36-e0ed-4443-a501-5148992014b9",
			"cn":        "grupp1",
			"l":         "Klass",
			"owner":     "ou=skolenhet1,ou=skolgruppen,dc=kommunen,dc=se",
			"member": []string{
				"uid=baje,dc=kommunen,dc=se",
				"uid=lini,dc=kommunen,dc=se",
				"uid=pejo,dc=kommunen,dc=se",
			},
		},
		{
			"entryUUID": "645ebd9d-55b5-4e7a-900a-92b9369c8f6a",
			"cn":        "grupp2",
			"l":         "Undervisning",
			"owner":     "ou=skolenhet1,ou=skolgruppen,dc=kommunen,dc=se",
			"member": []string{
				"uid=anan,dc=kommunen,dc=se",
				"uid=baje,dc=kommunen,dc=se",
				"uid=lini,dc=kommunen,dc=se",
			},
		},
		{
			"entryUUID": "9e2b1131-aac4-4ba3-adc8-a232a2b553d8",
			"cn":        "grupp3",
			"l":         "Undervisning",
			"owner":     "ou=skolenhet2,ou=skolgruppen,dc=kommunen,dc=se",
			"member": []string{
				"uid=baje,dc=kommunen,dc=se",
				"uid=stjo,dc=kommunen,dc=se",
			},
		},
		{
			"entryUUID": "def59679-3808-4210-a707-ebce13467206",
			"cn":        "DNP-GRGRMAT01_6",
			"l":         "Undervisning",
			"owner":     "ou=skolenhet1,ou=skolgruppen,dc=kommunen,dc=se",
			"member": []string{
				"uid=anan,dc=kommunen,dc=se",
				"uid=baje,dc=kommunen,dc=se",
				"uid=lini,dc=kommunen,dc=se",
				"uid=pejo,dc=kommunen,dc=se",
			},
		},
		{
			"entryUUID": "0d9a892f-3698-4b56-91c2-44fdb61dedf1",
			"cn":        "DNP-SVASVA03_GY",
			"l":         "Undervisning",
			"owner":     "ou=skolenhet2,ou=skolgruppen,dc=kommunen,dc=se",
			"member": []string{
				"uid=baje,dc=kommunen,dc=se",
				"uid=stjo,dc=kommunen,dc=se",
			},
		},
	},
}

func main() {
	if len(os.Args) != 2 {
		fmt.Fprintf(os.Stderr, "Usage: %s <dataset>\n", os.Args[0])
		fmt.Fprintf(os.Stderr, "Available data sets:\n")
		for name := range dataSets {
			fmt.Fprintf(os.Stderr, "  %s\n", name)
		}
		os.Exit(1)
	}

	name := os.Args[1]
	data, ok := dataSets[name]
	if !ok {
		fmt.Fprintf(os.Stderr, "Unknown data set: %s\n", name)
		fmt.Fprintf(os.Stderr, "Available data sets:\n")
		for n := range dataSets {
			fmt.Fprintf(os.Stderr, "  %s\n", n)
		}
		os.Exit(1)
	}

	encoder := json.NewEncoder(os.Stdout)
	encoder.SetIndent("", " ")
	if err := encoder.Encode(data); err != nil {
		fmt.Fprintf(os.Stderr, "Error encoding JSON: %v\n", err)
		os.Exit(1)
	}
}
