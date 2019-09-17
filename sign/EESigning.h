//
//  EESigning.h
//  original Created by Matt Clarke on 28/12/2017.
//

#pragma once

#include <string>

class EESigning {
public:
	EESigning(const std::string& certificate, const std::string& privateKey);
	bool signBundleAtPath(const std::string& path, const std::string& entitlements);
protected:
	std::string _createPKCS12CertificateWithKey(const std::string& key, const std::string& certificate);
protected:
	std::string _certificate; // treat as binary data
	std::string _privateKey;
	std::string _PKCS12;
};
