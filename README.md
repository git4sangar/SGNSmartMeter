# SGNSmartMeter
Code Barc
# Smart Meter

## Introduction
* XMPP Client and Software Update
## Notes for compiling open sources
  * Configure script syntax
    `env LDFLAGS="-L/home/pi/sgn/dev/lib" LIBS="-lssl -lcrypto" ./configure --prefix=/home/pi/sgn/dev/`
## XMPP Notes
  * XMPP clients verify peers as it establishes the connection using TLS with the XMPP server
  * For verification to get through we need to supply the root certificate for that server
  * XMPP picks the certificate from a user defined path & file through the function 
    `SSL_CTX_load_verify_locations`
  * XMPP client calls this function from `tls_new` function defined in `src/tls_openssl.c`.
  * Change the code as follows
  `SSL_CTX_load_verify_locations(tls->ssl_ctx, "/home/tstone10/sgn/dev/certs/cacert.pem", NULL);`
    Attention: the file name shall be with path
