#pragma once
#include <semaphore>
#include <atomic>

class spinlock
{
    std::atomic_flag m_flag = ATOMIC_FLAG_INIT;
public:
    void lock()
    {
        while (!m_flag.test_and_set(std::memory_order_acquire))
            ;
    }
    void unlock()
    {
        m_flag.clear(std::memory_order_release);
    }
};

class rwlock
{
    template<unsigned int readers_bits, unsigned int wait_to_read_bits, unsigned int writer_bits>
    union status_bits
    {
        struct 
        {
            unsigned int readers: readers_bits;
            unsigned int wait_to_read: wait_to_read_bits;
            unsigned int writers: writer_bits;
        };
        uint32_t value;
        status_bits() : value(0) {}
        status_bits(const int& v) : value(v) {}
        bool inc_reader() { if (readers < ((1 << readers_bits) - 1)) {readers++; return true;} else return false;}
        bool inc_wait_to_read() { if (wait_to_read < ((1 << wait_to_read_bits) - 1)) {wait_to_read++; return true;} else return false;}
        bool inc_writer() { if (writers < ((1 << writer_bits) - 1)) {writers++; return true;} else return false;}
        static constexpr uint32_t reader_one() { return 1;}
        static constexpr uint32_t wait_to_read_one() { return 1 << readers_bits;}
        static constexpr uint32_t writer_one() { return 1 << (readers_bits + wait_to_read_bits);}
    };
    using status = status_bits<16, 8, 8>;
    std::atomic<uint32_t> m_status;
    std::counting_semaphore<256> m_readSem{0}, m_writeSem{0};

public:
    void lock_read()
    {
        status old_status, new_status;
        do 
        {
            old_status = m_status.load(std::memory_order_relaxed);
            new_status = m_status.load(std::memory_order_relaxed);
            if (old_status.writers > 0)
            {
                if (!new_status.inc_wait_to_read()) continue;
            }
            else
            {
                if (!new_status.inc_reader()) continue;
            }
        }
        while (!m_status.compare_exchange_weak(old_status.value, new_status.value, std::memory_order_acquire, std::memory_order_relaxed));
        
        if (new_status.writers > 0)
            m_readSem.acquire();
    }

    void unlock_read()
    {
        status old_status = m_status.fetch_sub(status::reader_one(), std::memory_order_release);
        if (old_status.readers == 1 && old_status.writers > 0)
            m_writeSem.release();
    }

    void lock_write()
    {
        status old_status, new_status;
        do 
        {
            old_status = m_status.load(std::memory_order_relaxed);
            new_status = m_status.load(std::memory_order_relaxed);
            if (!new_status.inc_writer()) continue;
        }
        while (!m_status.compare_exchange_weak(old_status.value, new_status.value, std::memory_order_acquire, std::memory_order_relaxed));
        
        if (new_status.readers > 0 || new_status.writers > 0)
            m_writeSem.acquire();
    }

    void unlock_write()
    {
        status old_status, new_status;
        do 
        {
            old_status = m_status.load(std::memory_order_relaxed);
            new_status = m_status.load(std::memory_order_relaxed);
            new_status.writers--;
            if (old_status.wait_to_read > 0)
            {
                new_status.wait_to_read = 0;
                new_status.readers = old_status.wait_to_read;
            }
        }
        while (!m_status.compare_exchange_weak(old_status.value, new_status.value, std::memory_order_release, std::memory_order_relaxed));

        if (old_status.wait_to_read > 0)
        {
            m_readSem.release(old_status.wait_to_read);
        }
        else if (old_status.writers > 1)
        {
            m_writeSem.release();
        }
    }
};