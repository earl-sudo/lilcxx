#ifndef MEMCACHE_H
#define MEMCACHE_H

#include <iostream>
#include <cstdlib>
#include <cstdint>
#include <list>

namespace EjUtil {

    template<typename TN>
    struct MemCache {
    private:
        std::list<TN*>      blocks_; // Memory blocks which contain memory for multiple TN.
        std::list<TN*>      freeList_; // List of memory blocks to give when needed.
        void addToFreeList(TN* block) {
            for (size_t i = 0; i < itemsInBlock_; i++) {
                freeList_.push_back(&block[i]);
            }
        }
        void addBlock() { // Add block and split into freeList.
            auto block = (TN*)calloc(itemsInBlock_, sizeof(TN));
            addToFreeList(block);
            blocks_.push_back(block);
        }
    public:
        bool                turnOff_ = false; // Just use normal calloc()/free().
        size_t              itemsInBlock_ = 16; // How many blocks in each allocation.
        bool                createBlocksAsNeeded_ = true; // Create block of objects when freeListEmpty.
        // Stats
        size_t              numAllocated_ = 0;
        size_t              numFree_ = 0;
        size_t              maxNumCreated_ = 0;
        size_t              numNoneCacheCreated_ = 0;

        void init() { if (turnOff_) return; addBlock();  }
        MemCache() {
            for (auto item : freeList_) {
                if (!fromBlock(item)) {
                    free(item);
                }
            }
            for (auto block : blocks_) {
                free(block);
            }
        }
        bool fromBlock(const TN* in) const { // Check that address is one of our memory objects, i.e. from freeList.
            auto inAddr = (uint64_t)in;
            for (auto block : blocks_) {
                auto blockAddr = (uint64_t)block;
                if (blockAddr > inAddr) continue; // before block
                if (inAddr > (blockAddr + itemsInBlock_*sizeof(TN))) continue; // past block
                if ((inAddr-blockAddr)%sizeof(TN)!=0) continue; // wrong alignment.
                return true;
            }
            return false;
        }
        TN* get() {
            numAllocated_++;
            if (turnOff_) {
                numNoneCacheCreated_++;
                return (TN*)calloc(1, sizeof(TN));
            }
            if (!freeList_.empty()) {
                auto ret = freeList_.back();
                freeList_.pop_back();
                return ret;
            }
            if (!createBlocksAsNeeded_) {
                numNoneCacheCreated_++;
                return (TN *) calloc(1, sizeof(TN));
            }
            addBlock();
            numNoneCacheCreated_ += itemsInBlock_;
            auto ret = freeList_.back();
            freeList_.pop_back();
            return ret;
        }
        void put(TN* returned) {
            numFree_++;
            auto numAlive = (numAllocated_ - numFree_);
            if (numAlive > maxNumCreated_) maxNumCreated_ = numAlive;
            if (turnOff_) {
                free(returned);
                return;
            }
            freeList_.push_back(returned);
        }
    };
} // namespace EjUtil

#endif //MEMCACHE_H
