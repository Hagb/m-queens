typedef struct __attribute__ ((packed)) {
    uint cols; // bitfield with all the used columns
    uint diagl;// bitfield with all the used diagonals down left
    uint diagr;// bitfield with all the used diagonals down right
    //uint dummy;// dummy to align at 128bit
} start_condition;

#define MAXN 29

typedef char int_fast8_t;
typedef uchar uint_fast8_t;
typedef short int_fast16_t;
typedef ushort uint_fast16_t;
typedef int int_fast32_t;
typedef uint uint_fast32_t;
#define UINT_FAST32_MAX UINT_MAX

typedef ulong uint_fast64_t;

#define MAXD (N - PLACED)

#if (MAXD > 13)
#error "Depth to high, risk of overflow in result counter"
#endif

// for convenience
#define L (get_local_id(0))
#define G (get_global_id(0))

kernel void solve_subboard(__global const start_condition* in_starts, __global uint* out_cnt) {
    size_t id = get_global_id(0);

    // counter for the number of solutions
    // sufficient until n=29

    // 32 bits are sufficient if the rest depth is <= 13
    uint_fast32_t num = 0;
    __local uint_fast32_t cols[WG_SIZE][DEPTH];
    __local uint_fast32_t posibs[WG_SIZE][DEPTH]; // Our backtracking 'stack'
    __private uint_fast32_t diagl[DEPTH], diagr[DEPTH];
    int_fast16_t d = 0; // d is our depth in the backtrack stack
    // The UINT_FAST32_MAX here is used to fill all 'coloumn' bits after n ...
    cols[L][d] = in_starts[id].cols | (UINT_FAST32_MAX << N);
    // This places the first two queens
    diagl[d] = in_starts[id].diagl;
    diagr[d] = in_starts[id].diagr;
    #define LOOKAHEAD 3

    //  The variable posib contains the bitmask of possibilities we still have
    //  to try in a given row ...
    uint_fast32_t posib = (cols[L][d] | diagl[d] | diagr[d]);

    while (d >= 0) {
      // moving the two shifts out of the inner loop slightly improves
      // performance
      uint_fast32_t diagl_shifted = diagl[d] << 1;
      uint_fast32_t diagr_shifted = diagr[d] >> 1;
      uint_fast32_t l_cols = cols[L][d];


      while (posib != UINT_FAST32_MAX) {
        // The standard trick for getting the rightmost bit in the mask
        uint_fast32_t bit = ~posib & (posib + 1);
        uint_fast32_t new_diagl = (bit << 1) | diagl_shifted;
        uint_fast32_t new_diagr = (bit >> 1) | diagr_shifted;
        uint_fast32_t new_posib = (l_cols | bit | new_diagl | new_diagr);
        posib ^= bit; // Eliminate the tried possibility.
        bit |= l_cols;

        if (new_posib != UINT_FAST32_MAX) {
// slow on the GPU, probably control flow bound
#if 0
            uint_fast32_t lookahead1 = (bit | (new_diagl << (LOOKAHEAD - 2)) | (new_diagr >> (LOOKAHEAD - 2)));
            uint_fast32_t lookahead2 = (bit | (new_diagl << (LOOKAHEAD - 1)) | (new_diagr >> (LOOKAHEAD - 1)));
            uint_fast32_t l_rest = 32 - popcount(bit);
            uint_fast8_t allowed1 = l_rest >= 2;
            uint_fast8_t allowed2 = l_rest > 2;


            if(allowed1 && (lookahead1 == UINT_FAST32_MAX)) {
                continue;
            }

            if(allowed2 && (lookahead2 == UINT_FAST32_MAX)) {
                continue;
            }
#endif

            // The next two lines save stack depth + backtrack operations
            // when we passed the last possibility in a row.
            // Go lower in the stack, avoid branching by writing above the current
            // position
            posibs[L][d + 1] = posib;
            d += posib != UINT_FAST32_MAX; // avoid branching with this trick
            posib = new_posib;

            // make values current
            l_cols = bit;
            cols[L][d] = bit;
            diagl[d] = new_diagl;
            diagr[d] = new_diagr;
            diagl_shifted = new_diagl << 1;
            diagr_shifted = new_diagr >> 1;
        } else {
            // when all columns are used, we found a solution
            num += bit == UINT_FAST32_MAX;
        }
      }
      posib = posibs[L][d]; // backtrack ...
      d--;
    }

    out_cnt[id] += num;
}