//
//  EESigning.m
//  original Created by Matt Clarke on 28/12/2017.
//

#include "EESigning.h"
#include "ldid/ldid.hpp"

#include <stdio.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/pkcs12.h>
#include <openssl/err.h>

#include "ca.h"

static auto dummy([](double) {});

EESigning::EESigning(const std::string& certificate, const std::string& privateKey)
	: _certificate(certificate), _privateKey(privateKey)
{
	// Create our PKCS12!
	_PKCS12 = _createPKCS12CertificateWithKey(privateKey, certificate);
	if (_PKCS12.size() == 0) {
		// Holy moly Batman.
	}
}

std::string EESigning::_createPKCS12CertificateWithKey(const std::string& key, const std::string& certificate)
{
	// Load CA Chain
	BIO *bio = BIO_new(BIO_s_mem());
	BIO_puts(bio, apple_ios_pem);
	X509 *chain = PEM_read_bio_X509(bio, NULL, NULL, NULL);
	if (!chain) {
		fprintf(stderr, "Failed to load CA chain.\n");
		return "";
	}

	// Load root CA
	BIO *rootCABio = BIO_new(BIO_s_mem());
	BIO_puts(rootCABio, root_pem);
	X509 *rootCA = PEM_read_bio_X509(rootCABio, NULL, NULL, NULL);
	if (!rootCA) {
		fprintf(stderr, "Failed to load root CA.\n");
		return "";
	}

	// Code utilised from: http://fm4dd.com/openssl/pkcs12test.htm
	X509           *cert, *cacert;
	STACK_OF(X509) *cacertstack;
	PKCS12         *pkcs12bundle;
	EVP_PKEY       *cert_privkey;
	BIO            *bio_privkey = NULL, *bio_certificate = NULL, *bio_pkcs12 = NULL;
	int            bytes = 0;
	char           *data = NULL;
	long           len = 0;
	int            error = 0;

	/* ------------------------------------------------------------ *
	 * 1.) These function calls are essential to make PEM_read and  *
	 *     other openssl functions work.                            *
	 * ------------------------------------------------------------ */
	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();

	/*--------------------------------------------------------------*
	 * 2.) we load the certificates private key                     *
	 *    ( for this, it has no password )                     *
	 *--------------------------------------------------------------*/

	bio_privkey = BIO_new(BIO_s_mem());
	BIO_puts(bio_privkey, key.c_str());

	if (!(cert_privkey = PEM_read_bio_PrivateKey(bio_privkey, NULL, NULL, NULL))) {
		fprintf(stderr, "Error loading certificate private key content.\n");
		return "";
	}

	/*--------------------------------------------------------------*
	 * 3.) we load the corresponding certificate                    *
	 *--------------------------------------------------------------*/

	const unsigned char *input = (unsigned char*)certificate.data();
	cert = d2i_X509(NULL, &input, (int)certificate.size());
	if (!cert) {
		fprintf(stderr, "Error loading cert into memory.\n");
		return "";
	}

	/*--------------------------------------------------------------*
	 * 4.) we load the CA certificate who signed it                 *
	 *--------------------------------------------------------------*/

	cacert = chain;

	/*--------------------------------------------------------------*
	 * 5.) we load the CA certificate on the stack                  *
	 *--------------------------------------------------------------*/

	if ((cacertstack = sk_X509_new_null()) == NULL) {
		fprintf(stderr, "Error creating STACK_OF(X509) structure.\n");
		return "";
	}

	sk_X509_push(cacertstack, rootCA);
	sk_X509_push(cacertstack, cacert);

	/*--------------------------------------------------------------*
	 * 6.) we create the PKCS12 structure and fill it with our data *
	 *--------------------------------------------------------------*/

	if ((pkcs12bundle = PKCS12_new()) == NULL) {
		fprintf(stderr, "Error creating PKCS12 structure.\n");
		return "";
	}

	/* values of zero use the openssl default values */
	pkcs12bundle = PKCS12_create(
		(char*)"",       // We give a password of "" here as ldid expects that
		(char*)"ShadowSign", //(char*)"ReProvision",  // friendly certname
		cert_privkey,// the certificate private key
		cert,        // the main certificate
		cacertstack, // stack of CA cert chain
		0,           // int nid_key (default 3DES)
		0,           // int nid_cert (40bitRC2)
		0,           // int iter (default 2048)
		0,           // int mac_iter (default 1)
		0            // int keytype (default no flag)
	);
	if (pkcs12bundle == NULL) {
		fprintf(stderr, "Error generating a valid PKCS12 certificate.\n");
		return "";
	}

	/*--------------------------------------------------------------*
	 * 7.) we write the PKCS12 structure out to NSData              *
	 *--------------------------------------------------------------*/

	bio_pkcs12 = BIO_new(BIO_s_mem());
	bytes = i2d_PKCS12_bio(bio_pkcs12, pkcs12bundle);

	if (bytes <= 0) {
		fprintf(stderr, "Error writing PKCS12 certificate.\n");
		return "";
	}

	len = BIO_get_mem_data(bio_pkcs12, &data);
	std::string result(reinterpret_cast<char const*>(data), len);
	
	/*--------------------------------------------------------------*
	 * 8.) we are done, let's clean up                              *
	 *--------------------------------------------------------------*/

	X509_free(cert);
	X509_free(cacert);
	sk_X509_free(cacertstack);
	PKCS12_free(pkcs12bundle);

	BIO_free_all(bio_pkcs12);
	BIO_free_all(bio_certificate);
	BIO_free_all(bio_privkey);

	return result;
}

bool EESigning::signBundleAtPath(const std::string& path, const std::string& entitlements)
{
	// We request that ldid signs the bundle given, with our PKCS12 file so that it is validly codesigned.
	if (_PKCS12.size() == 0) {
		fprintf(stderr, "No valid PKCS12 certificate is available to use for signing.\n");		
		return false;
	}

	std::string requirementsString = "";

	// We can now sign!
	ldid::DiskFolder folder(path);
	ldid::Bundle outputBundle = Sign("", folder, _PKCS12, requirementsString, ldid::fun([&](const std::string &, const std::string &ent) -> std::string {
		return entitlements;
	}), ldid::fun([&](const std::string &) {}), ldid::fun(dummy));

	// TODO: Handle errors!
	return true;
}