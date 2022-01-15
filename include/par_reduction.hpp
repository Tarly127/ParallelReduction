#include <thread>
#include <concepts>
#include <vector>
#include <span>
#include <numeric>
#include <iostream>
#include <functional>
#include <mutex>

#ifndef __MY_PARALLEL__
#define __MY_PARALLEL__


#pragma GCC system_header
namespace my_parallel
{
    template<typename T>
    concept is_number = std::floating_point<T> || std::integral<T>;

    // Newer version (based)
    // Generic that accepts only real number types and infers the base value as 0
    template<typename InputIt, typename Fn, typename... Args>
    requires is_number<typename std::iterator_traits<InputIt>::value_type> 
    constexpr typename std::iterator_traits<InputIt>::value_type 
    reduce (InputIt begin, InputIt end, Fn&& f, Args&&... args)
    {
        using value_t = typename std::iterator_traits<InputIt>::value_type;

        const std::size_t N_THREADS = std::thread::hardware_concurrency() * 2; // assuming hyperthreading
        const std::size_t COUNT = static_cast<std::size_t>( std::distance(begin, end) );

        std::thread workers[N_THREADS];
        value_t partial_reduction[N_THREADS];

        std::size_t curr_block_size = 0;
        InputIt end_aux;

        for(auto tindex = 0; tindex < N_THREADS; tindex++)
        {
            curr_block_size = (COUNT / N_THREADS) + ( tindex < (COUNT % N_THREADS) );

            partial_reduction[tindex] = 0;

            end_aux = begin;

            std::advance(end_aux, curr_block_size);
                                
            workers[tindex] = std::thread(
                [&](InputIt p_begin, InputIt p_end, auto tid) -> void
                {
                    for(; p_begin != p_end; ++p_begin)
                    {
                        std::invoke(f, *p_begin, &partial_reduction[tid], args...);
                    }
                },
                begin,
                end_aux,
                tindex
            );

            begin = end_aux;
        }

        for(std::thread& t : workers)
        {
            t.join();
        }

        value_t fr = partial_reduction[0]; // (menos roto)

        bool first = true;

        for(const value_t& pr : partial_reduction)
        {
            if(!first) std::invoke(f, pr, &fr, args...);
            else first = false;
        }

        return fr;
    }

    // Newest version (very based)
    // Generic that accepts any type and will use a passed initial value for the accumulator
    template<typename InputIt, typename Fn, typename... Args>
    constexpr typename std::iterator_traits<InputIt>::value_type 
    reduce (InputIt begin, InputIt end, typename std::iterator_traits<InputIt>::value_type init, Fn&& f, Args&&... args)
    {
        using value_t = typename std::iterator_traits<InputIt>::value_type;

        const std::size_t N_THREADS = std::thread::hardware_concurrency() * 2; // assuming hyperthreading
        const std::size_t COUNT = static_cast<std::size_t>( std::distance(begin, end) );

        std::thread workers[N_THREADS];
        value_t partial_reduction[N_THREADS];

        std::size_t curr_block_size = 0;

        InputIt end_aux;

        for(auto tindex = 0; tindex < N_THREADS; tindex++)
        {
            curr_block_size = (COUNT / N_THREADS) + ( tindex < (COUNT % N_THREADS) );

            partial_reduction[tindex] = init;

            end_aux = begin;

            std::advance(end_aux, curr_block_size);
                                
            workers[tindex] = std::thread(
                [&](InputIt p_begin, InputIt p_end, auto tid) -> void
                {
                    for(; p_begin != p_end; ++p_begin)
                    {
                        std::invoke(f, *p_begin, &partial_reduction[tid], args...);
                    }
                },
                begin,
                end_aux,
                tindex
            );

            begin = end_aux;
        }

        for(std::thread& t : workers)
        {
            t.join();
        }

        value_t fr = init; // kinda roto também

        for(const value_t& pr : partial_reduction)
        {
            std::invoke(f, pr, &fr, args...);
        }

        return fr;
    }

    // Original version (cringe)
    template<typename T, typename Fn, typename... Args>
    requires is_number<T> && std::invocable<Fn, T, T*, Args...>
    static constexpr T 
    reduce_original ( const std::vector<T>& v, const std::size_t num_threads, Fn&& f, Args&&... args )
    {
        // Various declarations
        const std::size_t v_size = v.size();
        std::size_t bs_accum = 0, curr_block_size;

        std::vector<T> partial_reductions(num_threads, 0); 
        std::vector<std::thread> workers(num_threads); 


        /**
         * @brief Each thread will make a partial reduction over a small chunk of the data 
         * First, we need to start each thread's work
         */
        for(auto tid = 0; tid < num_threads; tid++)
        {
            curr_block_size = tid < (v_size % num_threads) ? (v_size / num_threads) + 1 : v_size / num_threads;

            workers[tid] = std::thread(
                [&](std::span<const T> pv, auto tid) -> void {

                    for (const T& elem : pv)
                    {
                        std::invoke(f, elem, &partial_reductions[tid], args...);
                    }
                },
                std::span ( v.begin() + bs_accum, v.begin() + bs_accum + curr_block_size ), 
                tid
            );        
            bs_accum += curr_block_size;
        }

        // Thread Synchronization
        for(std::thread& t : workers)
        {
            t.join();
        }

        T final_reduction = 0;
        int i = 0;

        // Final partial reduction
        for(T& pr : partial_reductions)
        {
            std::invoke(f, pr, &final_reduction, args...);
        }

        return final_reduction;
    }
}
 
#endif
