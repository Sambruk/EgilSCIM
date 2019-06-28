#!/usr/bin/env python3

# Copyright (c) 2019 Föreningen Sambruk
#
# You should have received a copy of the MIT license along with this project.
# If not, see <https://opensource.org/licenses/MIT>.
#

import argparse
import json
import sys

if __name__ == '__main__':

    parser = argparse.ArgumentParser(description='List contents in authentication metadata')

    parser.add_argument('path',
                        help='path to metadata file',
                        nargs=1)
    parser.add_argument('--entity',
                        dest='entity',
                        help='Limit output to a specific entity',
                        nargs=1)
    parser.add_argument('--servers',
                        dest='servers',
                        action='store_true',
                        help='whether server names should be displayed')
    parser.add_argument('--clients',
                        dest='clients',
                        action='store_true',
                        help='whether client names should be displayed')

    args = parser.parse_args()

    try:
        with open(args.path[0], 'r') as metadatafile:
            metadata_str = metadatafile.read()
    except OSError:
        print("Failed to read metadata from file", path)
        sys.exit(1)
    
    metadata = json.loads(metadata_str)

    for entity in metadata['entities']:
        if (args.entity and args.entity[0] != entity['entity_id']):
            continue
        
        print(entity['entity_id'])
        if (args.servers and 'servers' in entity):
            for server in entity['servers']:
                print("\t", server['name'])
        if (args.clients and 'clients' in entity):
            for client in entity['clients']:
                print("\t", client['name'])
