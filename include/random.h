#ifndef __RANDOM_H_
#define __RANDOM_H_

#include <cmath>
#include <ctime>
#include <vector>
#include <algorithm>

namespace Random {

    double random();

    bool chance(double percent);

    int randomInt(int maxLength);

    std::vector<int> defaultPermutation(int maxLength);

    std::vector<int> randomPermutation(int maxLength);
}


#endif
