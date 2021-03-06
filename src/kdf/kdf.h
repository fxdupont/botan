/*
* KDF/MGF
* (C) 1999-2007 Jack Lloyd
*
* Distributed under the terms of the Botan license
*/

#ifndef BOTAN_KDF_BASE_H__
#define BOTAN_KDF_BASE_H__

#include <botan/algo_base.h>
#include <botan/secmem.h>
#include <botan/types.h>

namespace Botan {

/**
* Key Derivation Function
*/
class BOTAN_DLL KDF : public Algorithm
   {
   public:
      /**
      * Derive a key
      * @param key_len the desired output length in bytes
      * @param secret the secret input
      * @param salt a diversifier
      */
      secure_vector<byte> derive_key(size_t key_len,
                                    const secure_vector<byte>& secret,
                                    const std::string& salt = "") const
         {
         return derive_key(key_len, &secret[0], secret.size(),
                           reinterpret_cast<const byte*>(salt.data()),
                           salt.length());
         }

      /**
      * Derive a key
      * @param key_len the desired output length in bytes
      * @param secret the secret input
      * @param salt a diversifier
      */
      template<typename Alloc, typename Alloc2>
      secure_vector<byte> derive_key(size_t key_len,
                                     const std::vector<byte, Alloc>& secret,
                                     const std::vector<byte, Alloc2>& salt) const
         {
         return derive_key(key_len,
                           &secret[0], secret.size(),
                           &salt[0], salt.size());
         }

      /**
      * Derive a key
      * @param key_len the desired output length in bytes
      * @param secret the secret input
      * @param salt a diversifier
      * @param salt_len size of salt in bytes
      */
      secure_vector<byte> derive_key(size_t key_len,
                                    const secure_vector<byte>& secret,
                                    const byte salt[],
                                    size_t salt_len) const
         {
         return derive_key(key_len,
                           &secret[0], secret.size(),
                           salt, salt_len);
         }

      /**
      * Derive a key
      * @param key_len the desired output length in bytes
      * @param secret the secret input
      * @param secret_len size of secret in bytes
      * @param salt a diversifier
      */
      secure_vector<byte> derive_key(size_t key_len,
                                    const byte secret[],
                                    size_t secret_len,
                                    const std::string& salt = "") const
         {
         return derive_key(key_len, secret, secret_len,
                           reinterpret_cast<const byte*>(salt.data()),
                           salt.length());
         }

      /**
      * Derive a key
      * @param key_len the desired output length in bytes
      * @param secret the secret input
      * @param secret_len size of secret in bytes
      * @param salt a diversifier
      * @param salt_len size of salt in bytes
      */
      secure_vector<byte> derive_key(size_t key_len,
                                    const byte secret[],
                                    size_t secret_len,
                                    const byte salt[],
                                    size_t salt_len) const
         {
         return derive(key_len, secret, secret_len, salt, salt_len);
         }

      void clear() {}

      virtual KDF* clone() const = 0;
   private:
      virtual secure_vector<byte>
         derive(size_t key_len,
                const byte secret[], size_t secret_len,
                const byte salt[], size_t salt_len) const = 0;
   };

/**
* Mask Generation Function
*/
class BOTAN_DLL MGF
   {
   public:
      virtual void mask(const byte in[], size_t in_len,
                              byte out[], size_t out_len) const = 0;

      virtual ~MGF() {}
   };

}

#endif
