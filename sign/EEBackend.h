//
//  EEBackend.h
//  original Created by Matt Clarke on 02/01/2018.
//

#pragma once

#include <string>

bool unpackIpaAtPath(const std::string& ipaPath, std::string* outputDirectory);

bool repackIpaAtPath(const std::string& extractedPath, const std::string& outputPath);

std::string applicationTemporaryDirectory();