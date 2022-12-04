#ifndef MEMCACHE_H
#define MEMCACHE_H
/*
 * Copyright (C) 2022 Earl Johnson
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * Earl Johnson https://github.com/earl-sudo/lilcxx 2022
 */

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
            auto inAddress = (uint64_t)in;
            for (auto block : blocks_) {
                auto blockAddress = (uint64_t)block;
                if (blockAddress > inAddress) continue; // before block
                if (inAddress > (blockAddress + itemsInBlock_ * sizeof(TN))) continue; // past block
                if ((inAddress - blockAddress) % sizeof(TN) != 0) continue; // wrong alignment.
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
