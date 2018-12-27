#include "cpusolver.h"
#include "cpusolver.h"
#include <iostream>
#include <cstdint>

cpuSolver::cpuSolver()
{

}

constexpr uint_fast8_t MINN = 2;
constexpr uint_fast8_t MAXN = 29;

bool cpuSolver::init(uint8_t boardsize, uint8_t placed)
{
    if(boardsize > MAXN || boardsize < MINN) {
        std::cout << "Invalid boardsize for cpusolver" << std::endl;
        return false;
    }

    if(placed >= boardsize) {
        std::cout << "Invalid number of placed queens for cpusolver" << std::endl;
        return false;
    }
    this->boardsize = boardsize;
    this->placed = placed;

    return true;
}


uint64_t cpuSolver::solve_subboard(const std::vector<start_condition>& starts) {

  // counter for the number of solutions
  // sufficient until n=29
  uint_fast64_t num = 0;
  size_t start_cnt = starts.size();

#pragma omp parallel for reduction(+ : num) schedule(dynamic)
  for (size_t cnt = 0; cnt < start_cnt; cnt++) {
    uint_fast64_t l_num = 0;
    uint_fast32_t cols[MAXN], posibs[MAXN]; // Our backtracking 'stack'
    uint_fast32_t diagl[MAXN], diagr[MAXN];
    int_fast8_t rest[MAXN]; // number of rows left
    int_fast16_t d = 0; // d is our depth in the backtrack stack
    // The UINT_FAST32_MAX here is used to fill all 'coloumn' bits after n ...
    cols[d] = starts[cnt].cols | (UINT_FAST32_MAX << boardsize);
    // This places the first two queens
    diagl[d] = starts[cnt].diagl;
    diagr[d] = starts[cnt].diagr;
#define LOOKAHEAD 3
    // we're allready two rows into the field here
    rest[d] = boardsize - LOOKAHEAD - this->placed;

    //printf("cpuid: X, cols: %x, diagl: %x, diagr: %x, rest: %d\n", cols[d], diagl[d], diagr[d], rest[d]);

    //  The variable posib contains the bitmask of possibilities we still have
    //  to try in a given row ...
    uint_fast32_t posib = (cols[d] | diagl[d] | diagr[d]);

    while (d >= 0) {
      // moving the two shifts out of the inner loop slightly improves
      // performance
      uint_fast32_t diagl_shifted = diagl[d] << 1;
      uint_fast32_t diagr_shifted = diagr[d] >> 1;
      int_fast8_t l_rest = rest[d];

      while (posib != UINT_FAST32_MAX) {
        // The standard trick for getting the rightmost bit in the mask
        uint_fast32_t bit = ~posib & (posib + 1);
        uint_fast32_t new_diagl = (bit << 1) | diagl_shifted;
        uint_fast32_t new_diagr = (bit >> 1) | diagr_shifted;
        uint_fast32_t new_posib = (cols[d] | bit | new_diagl | new_diagr);
        posib ^= bit; // Eliminate the tried possibility.
        bit |= cols[d];

        if (new_posib != UINT_FAST32_MAX) {
            uint_fast32_t lookahead1 = (bit | (new_diagl << (LOOKAHEAD - 2)) | (new_diagr >> (LOOKAHEAD - 2)));
            uint_fast32_t lookahead2 = (bit | (new_diagl << (LOOKAHEAD - 1)) | (new_diagr >> (LOOKAHEAD - 1)));
            uint_fast8_t allowed1 = l_rest >= 0;
            uint_fast8_t allowed2 = l_rest > 0;

            if(allowed1 && (lookahead1 == UINT_FAST32_MAX)) {
                continue;
            }

            if(allowed2 && (lookahead2 == UINT_FAST32_MAX)) {
                continue;
            }

            // The next two lines save stack depth + backtrack operations
            // when we passed the last possibility in a row.
            // Go lower in the stack, avoid branching by writing above the current
            // position
            posibs[d + 1] = posib;
            d += posib != UINT_FAST32_MAX; // avoid branching with this trick
            posib = new_posib;

            l_rest--;

            // make values current
            cols[d] = bit;
            diagl[d] = new_diagl;
            diagr[d] = new_diagr;
            rest[d] = l_rest;
            diagl_shifted = new_diagl << 1;
            diagr_shifted = new_diagr >> 1;
        } else {
            // when all columns are used, we found a solution
            l_num += bit == UINT_FAST32_MAX;
        }
      }
      posib = posibs[d]; // backtrack ...
      d--;
    }
     num += l_num;
  }
  return num * 2;
}

uint64_t cpuSolver::solve_subboard(const std::vector<Record> &starts)
{
    // counter for the number of solutions
    // sufficient until n=29
    uint_fast64_t num = 0;
    size_t start_cnt = starts.size();

  #pragma omp parallel for reduction(+ : num) schedule(dynamic)
    for (size_t cnt = 0; cnt < start_cnt; cnt++) {
        uint_fast64_t l_num = 0;
        uint_fast32_t cols[MAXN], rows[MAXN], posibs[MAXN]; // Our backtracking 'stack'
        uint_fast64_t diagu[MAXN], diagd[MAXN];
        int_fast16_t d = 1; // d is our depth in the backtrack stack
        uint_fast16_t row_idx = 0;
        // The UINT_FAST32_MAX here is used to fill all 'coloumn' bits after n ...
        const uint8_t N = boardsize;
        cols[d] = ((((starts[cnt].hor>>2)|(UINT64_MAX<<(N-4)))+1)<<(N-5))-1; // TODO(chr): decipher this, copied from q27 source
        rows[d] = starts[cnt].vert >> 2;
        diagu[d] = starts[cnt].diag_up >> 4;
        diagd[d] = (starts[cnt].diag_down >> 4) << (N - 5);

        auto new_cols = cols[d] + 1;

        // found a solution
        if(new_cols == 0) {
            l_num++;
            continue;
        }

        // skip already placed rows
        while((rows[d] & 1) != 0) {
            rows[d] >>= 1;
            row_idx++;
            diagu[d] <<= 1;
            diagd[d] >>= 1;
        }
        rows[d] >>= 1;
        row_idx++;

        uint_fast32_t posib = ~(cols[d] | diagu[d] | diagd[d]);
        posibs[d] = posib;

        while(d > 0) {

            while (posib != 0) {
                uint_fast32_t bit = posib & -posib;

                uint_fast32_t new_col = cols[d] | bit;
                uint_fast64_t new_diagu = (diagu[d] | bit) << 1;
                uint_fast64_t new_diagd = (diagd[d] | bit) >> 1;
                posib ^= bit;
                posibs[d] = posib;

                d++;

                cols[d] = new_col;
                diagu[d] = new_diagu;
                diagd[d] = new_diagd;
                rows[d] = rows[d - 1];

                // skip already placed rows
                while((rows[d] & 1) != 0) {
                    rows[d] >>= 1;
                    row_idx++;
                    diagu[d] <<= 1;
                    diagd[d] >>= 1;
                }

                // found a solution
                if((cols[d] + 1) == 0) {
                    l_num++;
                    continue;
                }

                posib = ~(cols[d] | diagu[d] | diagd[d]);
            }

            d--;
            posib = posibs[d];

        }

        num += l_num;
    }
}
