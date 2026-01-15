spacc: HW AES driver
pka:   HW RSA driver
troot: troot verification driver
OTP:   OTP driver
--------------------------------------------------------------------------------------------
How to gen key
1.
Generate RSA key pair through openssl
	openssl genrsa -out private_key.pem 2048
	openssl rsa -in private_key.pem -pubout -out public_key.pem

2.
Base64 decode private_key.pem and public_key.pem
	openssl base64 -d -in private_key.pem -out private_key.bin
	openssl base64 -d -in public_key.pem -out public_key.bin

3.
extract modulus(n.bin), private exponent(d.bin), public exponent(e.bin) from private_key.bin
and convert private_key.bin to key.c

How to sign image

./sign uboot.img private_key.bin

