#ifndef CPUSOLVER_H
#define CPUSOLVER_H

#include "isolver.h"
#include "parallel_hashmap/phmap.h"
#include <vector>
#include <stddef.h>

static constexpr size_t AVX2_alignment = 32;

template <class T>
class aligned_vec {
    T* begin;
    T* first_empty;
  public:
    aligned_vec(size_t size, size_t init_size = 0)
        : begin{nullptr}
    {
        // handle zero size allocations
        if (size > 0) {
            begin = static_cast<T*>(aligned_alloc(AVX2_alignment, size*sizeof(T)));
            assert(begin);
        }
        first_empty = begin + init_size;
    }

    aligned_vec (aligned_vec&& other) {
        this->begin = other.begin;
        this->first_empty = other.first_empty;
        other.begin = nullptr;
        other.first_empty = nullptr;
    }

    aligned_vec(aligned_vec const&) = delete;
    aligned_vec& operator=(aligned_vec const&) = delete;

    ~aligned_vec()
    {
        free(begin);
        begin = nullptr;
        first_empty = nullptr;
    }

    size_t size() const
    {
        return first_empty - begin;
    }

    void clear()
    {
        first_empty = begin;
    }

    T* data()
    {
        return begin;
    }

    const T* data() const
    {
        return begin;
    }

    T& operator[] (size_t index)
    {
        assert(begin);
        assert(index < size());
        return begin[index];
    }

    const T& operator[] (size_t index) const
    {
        assert(begin);
        assert(index < size());
        return begin[index];
    }

    void push_back(T& element)
    {
        assert(begin);
        *first_empty = element;
        first_empty++;
    }
};

class bit_probabilities {
public:
    bit_probabilities() = default;
    bit_probabilities(uint8_t bits) {
        num_bits = bits;
        counts.resize(num_bits, 0);
    }

    void update(uint_fast32_t bits) {
        total_count++;

        for(uint8_t bit_pos = 0; bit_pos < num_bits; bit_pos++) {
            counts[bit_pos] += (bits & (UINT32_C(1) << bit_pos)) != 0;
        }
    }

    std::vector<float> get_probablities() {
        std::vector<float> res;
        for(const auto& count: counts) {
            res.push_back(count / static_cast<float>(total_count));
        }

        return res;
    }

    size_t total_count = 0;
    std::vector<size_t> counts;
    uint8_t num_bits = 0;
};

class ClAccell;
class cpuSolver : public ISolver
{
public:
    cpuSolver();
    bool init(uint8_t boardsize, uint8_t placed);
    uint64_t solve_subboard(const std::vector<start_condition_t>& starts);
    size_t init_lookup(uint8_t depth, uint32_t skip_mask);
    using lut_t = std::vector<aligned_vec<diags_packed_t>>;
    static constexpr size_t max_candidates = 1024;


private:
    uint_fast8_t boardsize = 0;
    uint_fast8_t placed = 0;


    uint_fast64_t stat_lookups = 0;
    uint_fast64_t stat_cmps = 0;
    uint_fast64_t stat_high_prob = 0;
    uint_fast64_t stat_prob_saved = 0;

    bit_probabilities stat_solver_diagl;
    bit_probabilities stat_solver_diagr;


    // maps column patterns to index in lookup_solutions;
    phmap::flat_hash_map<uint32_t, uint32_t> lookup_hash;

    // store solutions, this is constant after initializing the lookup table
    // elements in lookup_solutions_low_prob don't have all bits of lookup_prob_mask set
    std::vector<diags_packed_t> flat_lookup_low_prob;
    // elements here have all bits of lookup_prob_mask set
    std::vector<diags_packed_t> flat_lookup_high_prob;

    // sizes for flat lookup tables
    std::vector<uint32_t> low_prob_sizes;
    std::vector<uint32_t> high_prob_sizes;

    // strides for flat lookup tables
    size_t high_prob_stride = 0;
    size_t low_prob_stride = 0;

    uint32_t prob_mask;

    ClAccell* accel = nullptr;

    uint64_t get_solution_cnt(uint32_t cols, diags_packed_t search_elem, lut_t &lookup_candidates_high_prob, lut_t &lookup_candidates_low_prob);
    uint64_t count_solutions(size_t sol_size, const diags_packed_t* __restrict__ sol_data, const aligned_vec<diags_packed_t>& candidates);
    uint8_t lookup_depth(uint8_t boardsize, uint8_t placed);
    __uint128_t factorial(uint8_t n);
    bool is_perfect_lut(uint8_t lut_depth, uint8_t free_bits, uint64_t entries);
};

#endif // CPUSOLVER_H
