#!/usr/bin/env bash

# Extracts public key from a x509 certificate and outputs
# its pin. Depends on OpenSSL.

if [ $1 == "-h" ]; then
    echo "Usage: $0 <certificate>"
    exit 0
fi

public_key=$(mktemp /tmp/public.key.XXXXXX)
openssl x509 -noout -in $1 -pubkey | \
    openssl asn1parse -noout -inform pem -out ${public_key}
openssl dgst -sha256 -binary ${public_key} | openssl enc -base64
rm ${public_key}
