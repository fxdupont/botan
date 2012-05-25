/*
* Mlock Allocator
* (C) 2012 Jack Lloyd
*
* Distributed under the terms of the Botan license
*/

#include <botan/locking_allocator.h>
#include <botan/internal/assert.h>
#include <algorithm>
#include <cstring>
#include <sys/mman.h>
#include <sys/resource.h>

namespace Botan {

namespace {

size_t mlock_limit()
   {
   struct rlimit limits;
   ::getrlimit(RLIMIT_MEMLOCK, &limits);

   if(limits.rlim_cur < limits.rlim_max)
      {
      limits.rlim_cur = limits.rlim_max;
      ::setrlimit(RLIMIT_MEMLOCK, &limits);
      ::getrlimit(RLIMIT_MEMLOCK, &limits);
      }

   /*
   * Linux defaults to only 64 KiB of mlockable memory per process
   * (too small) but BSDs offer a small fraction of total RAM (more
   * than we need). Bound the total mlock size to 256 KiB which is
   * enough to run the entire test suite without spilling to non-mlock
   * memory, but small enough that we should not cause problems if
   * multiple processes are mlocking on the same machine.
   */
   return std::min<size_t>(limits.rlim_cur, 256*1024);
   }

bool ptr_in_pool(const void* pool_ptr, size_t poolsize,
                 const void* buf_ptr, size_t bufsize)
   {
   const size_t pool = reinterpret_cast<size_t>(pool_ptr);
   const size_t buf = reinterpret_cast<size_t>(buf_ptr);

   if(buf < pool || buf >= pool + poolsize)
      return false;

   BOTAN_ASSERT(buf + bufsize <= pool + poolsize,
                "Invalid pointer/length halfway into mem pool");

   return true;
   }

size_t padding_for_alignment(size_t offset, size_t desired_alignment)
   {
   size_t mod = offset % desired_alignment;
   if(mod == 0)
      return 0; // already right on
   return desired_alignment - mod;
   }

}

void* mlock_allocator::allocate(size_t n, size_t alignment)
   {
   if(!m_pool || n >= m_poolsize)
      return nullptr; // bigger than the whole pool!

   std::lock_guard<std::mutex> lock(m_mutex);

   auto best_fit = m_freelist.end();

   for(auto i = m_freelist.begin(); i != m_freelist.end(); ++i)
      {
      // If we have a perfect fit, use it immediately
      if(i->second == n && (i->first % alignment) == 0)
         {
         const size_t offset = i->first;
         m_freelist.erase(i);
         ::memset(m_pool + offset, 0, n);

         BOTAN_ASSERT((reinterpret_cast<size_t>(m_pool) + offset) % alignment == 0,
                      "Returning correctly aligned pointer");

         return m_pool + offset;
         }

      if((i->second >= (n + padding_for_alignment(i->first, alignment)) &&
          ((best_fit == m_freelist.end()) || (best_fit->second > i->second))))
         {
         best_fit = i;
         }
      }

   if(best_fit != m_freelist.end())
      {
      const size_t offset = best_fit->first;

      const size_t alignment_padding = padding_for_alignment(offset, alignment);

      best_fit->first += n + alignment_padding;
      best_fit->second -= n + alignment_padding;

      // Need to realign, split the block
      if(alignment_padding)
         {
         if(best_fit->second == 0)
            {
            /*
            We used the entire block except for small piece used for
            alignment at the beginning, so just update this entry.
            */
            best_fit->second = alignment_padding;
            }
         else
            m_freelist.insert(best_fit, std::make_pair(offset, alignment_padding));
         }

      ::memset(m_pool + offset + alignment_padding, 0, n);

      BOTAN_ASSERT((reinterpret_cast<size_t>(m_pool) + offset + alignment_padding) % alignment == 0,
                   "Returning correctly aligned pointer");

      return m_pool + offset + alignment_padding;
      }

   return nullptr;
   }

bool mlock_allocator::deallocate(void* p, size_t n)
   {
   if(!m_pool || !ptr_in_pool(m_pool, m_poolsize, p, n))
      return false;

   std::lock_guard<std::mutex> lock(m_mutex);

   const size_t start = static_cast<byte*>(p) - m_pool;

   auto comp = [](std::pair<size_t, size_t> x, std::pair<size_t, size_t> y){ return x.first < y.first; };

   auto i = std::lower_bound(m_freelist.begin(), m_freelist.end(),
                             std::make_pair(start, 0), comp);

   // try to merge with later block
   if(start + n == i->first)
      {
      i->first = start;
      i->second += n;
      n = 0;
      }

   // try to merge with previous block
   if(i != m_freelist.begin())
      {
      auto prev = std::prev(i);

      if(prev->first + prev->second == start)
         {
         if(n)
            {
            prev->second += n;
            n = 0;
            }
         else
            {
            // merge adjoining
            prev->second += i->second;
            m_freelist.erase(i);
            }
         }
      }

   if(n != 0) // no merge possible?
      m_freelist.insert(i, std::make_pair(start, n));

   return true;
   }

mlock_allocator::mlock_allocator() :
   m_poolsize(mlock_limit()),
   m_pool(nullptr)
   {
#if !defined(MAP_NOCORE)
   #define MAP_NOCORE 0
#endif

   if(m_poolsize)
      {
      m_pool = static_cast<byte*>(
         ::mmap(
            nullptr, m_poolsize,
            PROT_READ | PROT_WRITE,
            MAP_ANONYMOUS | MAP_SHARED | MAP_NOCORE,
            -1, 0));

      if(m_pool == static_cast<byte*>(MAP_FAILED))
         throw std::runtime_error("Failed to mmap pool");

      std::memset(m_pool, 0x00, m_poolsize);

      if(::mlock(m_pool, m_poolsize) != 0)
         {
         ::munmap(m_pool, m_poolsize);
         m_pool = nullptr;
         throw std::runtime_error("Failed to lock pool");
         }

      m_freelist.push_back(std::make_pair(0, m_poolsize));
      }
   }

mlock_allocator::~mlock_allocator()
   {
   if(m_pool)
      {
      std::memset(m_pool, 0, m_poolsize);
      ::munlock(m_pool, m_poolsize);
      ::munmap(m_pool, m_poolsize);
      m_pool = nullptr;
      }
   }

mlock_allocator& mlock_allocator::instance()
   {
   static mlock_allocator mlock;
   return mlock;
   }

}