#include "Crypto.h"
#include "Impl/AlgorithmRSA.h"
#include "Impl/AlgorithmSHA1.h"
#include "Impl/AlgorithmSalsa20.h"

IHashFunction* Crypto::CreateSHA1()
{
    return new AlgorithmSHA1();
}

IStreamCipher* Crypto::CreateSalsa20(const uint8_t* keyBytes, const size_t keySize)
{
    return new AlgorithmSalsa20(keyBytes, keySize);
}

IPublicKeyAlgorithm* Crypto::CreateRSA(const IPublicKeyAlgorithm::HashingAlgorithm hashingAlgorithm, const RSAPaddingMode paddingMode)
{
    return new AlgorithmRSA(hashingAlgorithm, paddingMode);
}