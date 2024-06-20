# Enable/disable modules and 3rd-party libs to be included in interpreter

# ssl module requires one of the TLS libraries below
MICROPY_PY_SSL = 1
# axTLS has minimal size but implements only a subset of modern TLS
# functionality, so may have problems with some servers.
MICROPY_SSL_AXTLS = 1
# mbedTLS is more up to date and complete implementation, but also
# more bloated.
MICROPY_SSL_MBEDTLS = 0
