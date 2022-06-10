#include "calculatepoolsize.h"
#include "nodepool.h"


size_t calculatePoolSize() {
    return (sizeof(RootNode) + sizeof(NodePool) + sizeof(std::mutex) + sizeof(NodePool::Pool)) * 1.3f;
    // for now we will add a 130% boost for deterministic accuracy because the
    // current value seems to be off by being 30% less than the actual memory allocated
}