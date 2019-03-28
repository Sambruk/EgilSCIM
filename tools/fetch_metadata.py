#!/usr/bin/env python3

import argparse
import json
import urllib.request
import sys
from jwcrypto import jwk, jws;

default_metadata_url = 'https://fedscim-poc.skolfederation.se/md/skolfederation-fedscim-0_1.json'

def error_print(*args, **kwargs):
    """Print to stderr"""
    print(*args, file=sys.stderr, **kwargs)


def get_and_verify(url, keys, output: str) -> bool:
    """Downloads and verifies metadata"""
    try:
        with urllib.request.urlopen(url) as webfile:
            sig = webfile.read()
    except urllib.error.URLError:
        error_print("Failed to download metadata from", url)
        return False

    try:
        with open(keys, 'r') as keysfile:
            keyset_str = keysfile.read()
    except OSError:
        error_print("Failed to read key set from file", keys)
        return False

    keys_dict = json.loads(keyset_str)

    jwstoken = jws.JWS()
    jwstoken.deserialize(sig)
    
    jws.JWSHeaderRegistry["exp"] = jws.JWSHeaderParameter('Expiration', True, True)

    for k in keys_dict['keys']:
        try:
            key  = jwk.JWK(**k);
            jwstoken.verify(key)
            with open(output, 'wb') as outfile:
                outfile.write(jwstoken.payload)
            return True
        except jws.InvalidJWSSignature:
            continue

    error_print("Failed to verify the key")
    return False


if __name__ == '__main__':

    parser = argparse.ArgumentParser(description='Download and verify SCIM authentication metadata')

    parser.add_argument('--url',
                        dest='url',
                        help='address for metadata to download',
                        required=False,
                        default=default_metadata_url)
    parser.add_argument('--keys',
                        dest='keys',
                        help='path to key set (local file)',
                        required=True)
    parser.add_argument('--output',
                        dest='output',
                        help='where to write the verified metadata',
                        required=True)

    args = parser.parse_args()

    sys.exit(0 if get_and_verify(args.url, args.keys, args.output) else 1);
