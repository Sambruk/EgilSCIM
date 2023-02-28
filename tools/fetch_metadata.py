#!/usr/bin/env python3

# Copyright (c) 2019 Föreningen Sambruk
#
# You should have received a copy of the MIT license along with this project.
# If not, see <https://opensource.org/licenses/MIT>.
#

import argparse
import json
import urllib.request
import sys
from jose import jws
from datetime import datetime, timedelta
import os.path
import shutil
import time

default_metadata_url = 'https://md.swefed.se/kontosynk/kontosynk-prod-1.jws'

def error_print(*args, **kwargs):
    """Print to stderr"""
    print(*args, file=sys.stderr, **kwargs)


def get_and_verify(url, keys, output: str) -> bool:
    """Downloads and verifies metadata"""
    try:
        with urllib.request.urlopen(url) as webfile:
            sig = webfile.read()
    except urllib.error.URLError as e:
        raise RuntimeError("Failed to get URL " + url + " (" + str(e) + ")")

    jwsdict = json.loads(sig)
    
    with open(keys, 'r') as keysfile:
        keyset_str = keysfile.read()

    payload = jwsdict['payload']
    for s in jwsdict['signatures']:
        try:
            protected = s['protected']
            signature = s['signature']
            compact = protected + "." + payload + "." + signature

            exp_header = jws.get_unverified_headers(compact)['exp']
            exp = datetime.utcfromtimestamp(int(exp_header))
            if exp < datetime.utcnow():
                error_print("Signature expired at: " + str(exp))
                continue
            
            verified_payload = jws.verify(compact, keyset_str, algorithms=['ES256'])
            with open(output, 'wb') as outfile:
                outfile.write(verified_payload)
                return True
        except jws.JWSError:
            continue

    return False

def still_valid(cached: str) -> bool:
    """Checks if a metadata file is still valid according to its cache_ttl"""
    try:
        with open(cached, 'r') as f:
            cached_dict = json.load(f)

        cache_ttl = cached_dict.get('cache_ttl', 3600)
        mtime = os.path.getmtime(cached)
    
        return time.time() < mtime + cache_ttl
    except Exception as e:
        print("Failed to check if cached metadata was valid")
        error_print(e)
        return False

if __name__ == '__main__':

    parser = argparse.ArgumentParser(description='Download and verify federated TLS authentication metadata')

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
    parser.add_argument('--cached',
                        dest='cached',
                        help=("optional path to an earlier downloaded version, "
                              "if its cache TTL hasn't expired this version "
                              "will be re-used instead of fetching a new"),
                        required=False)

    args = parser.parse_args()

    try:
        if args.cached != None and os.path.isfile(args.cached) and still_valid(args.cached):
            try:
                shutil.copyfile(args.cached, args.output)
            except shutil.SameFileError:
                pass
            except:
                raise RuntimeError("Failed to copy cached version to new version")
            
        elif not get_and_verify(args.url, args.keys, args.output):
            raise RuntimeError("Failed to verify the signature")
    except Exception as e:
        error_print(e)
        sys.exit(1)
