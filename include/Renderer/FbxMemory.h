#pragma once
#include <fbxsdk.h>
#include <cstdlib>

// Custom memory management functions
static void* CustomMalloc(size_t pSize)
{
    return malloc(pSize);
}

static void* CustomCalloc(size_t pCount, size_t pSize)
{
    return calloc(pCount, pSize);
}

static void* CustomRealloc(void* pData, size_t pSize)
{
    return realloc(pData, pSize);
}

static void CustomFree(void* pData)
{
    free(pData);
}

// Initialize FBX memory management
inline void InitializeFbxAllocator()
{
    fbxsdk::FbxSetMallocHandler(CustomMalloc);
    fbxsdk::FbxSetReallocHandler(CustomRealloc);
    fbxsdk::FbxSetCallocHandler(CustomCalloc);
    fbxsdk::FbxSetFreeHandler(CustomFree);
}