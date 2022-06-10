#include "random.h"
#include "openssl/rand.h"

namespace Random {

    double random() {
        uint32_t num;
        RAND_bytes((unsigned char*)&num, sizeof(num));
        return double(num) / double(UINT32_MAX);
    }

    bool chance(double percent) {
        return random() < double(percent / 100);
    }

    int randomInt(int maxLength) {
        return std::round(random() * maxLength);
    }

    std::vector<int> defaultPermutation(int maxLength) {
        std::vector<int> permutation;
        permutation.reserve(maxLength);
        for(int i=0;i<maxLength;++i) permutation.push_back(i);
        return permutation;
    }

    std::vector<int> randomPermutation(int maxLength) {
        std::vector<int> permutation(defaultPermutation(maxLength));
        std::sort(permutation.begin(), permutation.end(), [](int, int) { return (random() > 0.5); });
        return permutation;
    }

}
