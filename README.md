# rsign

This project utilizes the relevant codes of code signning part of [ReProvision](https://github.com/Matchstic/ReProvision).

In my work, I need a IPA resign tool on Linux. I searched on internet but found nothing. It seems that ldid, isign, jtool can only work on iOS11 and below.

So I made this tool, it just utilizes ReProvision's code, rewrite it to C++, make it portable and standalone.

### Usage
```bash
usage: ./rsign --ipa=string --key=string --cert=string --profile=string --output=string [options] ... 
options:
      --ipa        Specify app content .ipa file path. (string)
  -k, --key        Path to your private key in PEM format. (string)
  -c, --cert       Path to your certificate in DER format. (string)
  -p, --profile    Path to your provisioning profile. This should be associated with your certificate. (string)
  -o, --output     Path to write the re-signed application. (string)
```

### Notes
I use MSBuild-based Linux projects of Visual Studio 2017.

Third-party libraries which not included in [ReProvision](https://github.com/Matchstic/ReProvision): [miniz](https://github.com/richgel999/miniz), [tinydir](https://github.com/cxong/tinydir), [cmdline](https://github.com/tanakh/cmdline)
### License

Licensed under the AGPLv3 License.
