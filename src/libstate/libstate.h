/*
* Library Internal/Global State
* (C) 1999-2008 Jack Lloyd
*
* Distributed under the terms of the Botan license
*/

#ifndef BOTAN_LIB_STATE_H__
#define BOTAN_LIB_STATE_H__

#include <botan/global_state.h>
#include <botan/algo_factory.h>
#include <botan/rng.h>
#include <mutex>
#include <string>
#include <vector>
#include <map>
#include <memory>

#if defined(BOTAN_HAS_LOCKING_ALLOCATOR)
  #include <botan/locking_allocator.h>
#endif

namespace Botan {

/**
* Global Library State
*/
class BOTAN_DLL Library_State
   {
   public:
      Library_State();
      ~Library_State();

      Library_State(const Library_State&) = delete;
      Library_State& operator=(const Library_State&) = delete;

      void initialize();

      /**
      * @return global Algorithm_Factory
      */
      Algorithm_Factory& algorithm_factory() const;

      /**
      * @return global RandomNumberGenerator
      */
      RandomNumberGenerator& global_rng();

      void poll_available_sources(class Entropy_Accumulator& accum);

      /**
      * Get a parameter value as std::string.
      * @param section the section of the desired key
      * @param key the desired keys name
      * @result the value of the parameter
      */
      std::string get(const std::string& section,
                      const std::string& key);

      /**
      * Check whether a certain parameter is set or not.
      * @param section the section of the desired key
      * @param key the desired keys name
      * @result true if the parameters value is set,
      * false otherwise
      */
      bool is_set(const std::string& section,
                  const std::string& key);

      /**
      * Set a configuration parameter.
      * @param section the section of the desired key
      * @param key the desired keys name
      * @param value the new value
      * @param overwrite if set to true, the parameters value
      * will be overwritten even if it is already set, otherwise
      * no existing values will be overwritten.
      */
      void set(const std::string& section,
               const std::string& key,
               const std::string& value,
               bool overwrite = true);

      /**
      * Add a parameter value to the "alias" section.
      * @param key the name of the parameter which shall have a new alias
      * @param value the new alias
      */
      void add_alias(const std::string& key,
                     const std::string& value);

      /**
      * Resolve an alias.
      * @param alias the alias to resolve.
      * @return what the alias stands for
      */
      std::string deref_alias(const std::string& alias);
   private:
      static RandomNumberGenerator* make_global_rng(Algorithm_Factory& af,
                                                    std::mutex& mutex);

      static std::vector<std::unique_ptr<EntropySource>> entropy_sources();

      void load_default_config();

      std::mutex global_rng_lock;
      RandomNumberGenerator* global_rng_ptr;

      std::mutex m_entropy_src_mutex;
      std::vector<std::unique_ptr<EntropySource>> m_sources;

      std::mutex config_lock;
      std::map<std::string, std::string> config;

      Algorithm_Factory* m_algorithm_factory;

#if defined(BOTAN_HAS_LOCKING_ALLOCATOR)
      mlock_allocator* mlock_allocator_ptr;
#endif
   };

}

#endif
