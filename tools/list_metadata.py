#!/usr/bin/env python3

# Copyright (c) 2019 Föreningen Sambruk
#
# You should have received a copy of the MIT license along with this project.
# If not, see <https://opensource.org/licenses/MIT>.
#

import argparse
import json
import sys
from url_normalize import url_normalize

# Returns all tags from a server in canonical form (lower case) in a set
def get_canonical_tags(server) -> frozenset:
    if 'tags' in server:
        tags = set()
        for tag in server['tags']:
            tags.add(tag.lower())
        return frozenset(tags)
    else:
        return frozenset()

# Converts a set of tags to the format it should be used when
# printing, or when specifying as argument to e.g. the EGIL client.
def tag_set_pretty(tags) -> str:
    return ','.join(tags)

def url_equals(url1, url2) -> bool:
    return url_normalize(url1) == url_normalize(url2)
    
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
                        help='whether server URIs should be displayed')
    parser.add_argument('--server-tags',
                        dest='server_tags',
                        action='store_true',
                        help='whether server tag combinations should be displayed')

    args = parser.parse_args()

    try:
        with open(args.path[0], 'r') as metadatafile:
            metadata_str = metadatafile.read()
    except OSError:
        print("Failed to read metadata from file", path)
        sys.exit(1)
    
    metadata = json.loads(metadata_str)

    for entity in metadata['entities']:
        if args.entity and not url_equals(args.entity[0], entity['entity_id']):
            continue
        
        print(entity['entity_id'])

        if 'servers' in entity:
            if args.servers:
                for server in entity['servers']:
                    print("\t", server['base_uri'])
            if args.server_tags:
                tag_combinations = set()
                for server in entity['servers']:
                    tag_combinations.add(get_canonical_tags(server))

                if len(tag_combinations) > 0:
                    for tags in tag_combinations:
                        print("\t", '"' + tag_set_pretty(tags) + '"')
