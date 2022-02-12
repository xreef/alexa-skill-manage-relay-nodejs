#include <pgmspace.h>

// Amazon Root CA 1 (AmazonRootCA1.pem)
static const uint8_t AWS_CERT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----
)EOF";

// Device Certificate (XXXXXXXXXX-certificate.pem.crt)
static const uint8_t AWS_CERT_CRT[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----
)KEY";

// Device Private Key (XXXXXXXXXX-private.pem.key)
static const uint8_t AWS_CERT_PRIVATE[] PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----

-----END RSA PRIVATE KEY-----
)KEY";
