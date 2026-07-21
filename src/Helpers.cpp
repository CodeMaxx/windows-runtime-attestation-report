#include "Helpers.h"

#include <wincrypt.h>
#include <cstdio>
#include <cstring>

static const HashAlg HashAlgs[] = {
    { CALG_MD5,     "MD5",     16 },
    { CALG_SHA1,    "SHA-1",   20 },
    { CALG_SHA_256, "SHA-256", 32 },
    { CALG_SHA_384, "SHA-384", 48 },
    { CALG_SHA_512, "SHA-512", 64 },
};

const HashAlg*
FindHashAlg(UINT16 algorithm)
{
    for (const HashAlg& alg : HashAlgs) {
        if (alg.id == algorithm) {
            return &alg;
        }
    }
    return nullptr;
}

void
PrintHex(const BYTE* data, size_t length)
{
    for (size_t i = 0; i < length; i += 1) {
        std::printf("%02x", data[i]);
    }
}

void
PrintRule(char fill)
{
    constexpr size_t kRuleWidth = 72;
    char rule[kRuleWidth + 1];
    std::memset(rule, fill, kRuleWidth);
    rule[kRuleWidth] = '\0';
    std::printf("%s\n", rule);
}

void
PrintBanner(const char* title)
{
    std::printf("\n");
    PrintRule('=');
    std::printf("  %s\n", title);
    PrintRule('=');
}

void
PrintFooter(const char* title)
{
    std::printf("\n");
    PrintRule('-');
    std::printf("  %s\n", title);
    PrintRule('-');
}

void
PrintBlob(const char* label, const BYTE* report, UINT32 reportSize,
          UINT16 algorithm, UINT32 offset)
{
    const HashAlg* alg = FindHashAlg(algorithm);
    const char* algName;

    if (alg != nullptr) {
        algName = alg->name;
    } else {
        algName = "unknown";
    }

    std::printf("      %-15s (%s): ", label, algName);

    if (alg != nullptr && offset != 0 &&
        InRange(offset, alg->digestLength, reportSize)) {
        PrintHex(report + offset, alg->digestLength);
        std::printf("\n");
    } else {
        std::printf("(none)\n");
    }
}
