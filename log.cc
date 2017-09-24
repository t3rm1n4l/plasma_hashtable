#include "log.h"
#include <assert.h>
#include <sys/mman.h>

Log::Log() :head(0), tail(0), phyHead(0) {
    logBuf = static_cast<char *>(mmap(0, LOG_MAXSIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0));
}

LogSpace Log::ReserveSpace(int size) {
    auto r = LogSpace{tail, logBuf+tail+logBlockHeaderSize};
    uint32_t *blockLen = reinterpret_cast<uint32_t*>(logBuf+tail);
    *blockLen = static_cast<uint32_t>(size);
    tail += size + logBlockHeaderSize;
    return r;
}

void Log::FinalizeWrite(LogSpace &s) {
}

bytes Log::Read(LogOffset off, Buffer &b) {
    uint32_t *blockLen = reinterpret_cast<uint32_t*>(logBuf + off);
    int n = static_cast<int>(*blockLen);
    auto buf = b.Alloc(n);
    memcpy(buf.data, logBuf + off + logBlockHeaderSize, n);
    return buf;
}

bytes Log::Read(LogOffset off, int n, Buffer &b, int &blkSz) {
    uint32_t *blockLen = reinterpret_cast<uint32_t*>(logBuf + off);
    blkSz = *blockLen;
    auto buf = b.Alloc(n);
    memcpy(buf.data, logBuf + off + logBlockHeaderSize, n);
    return buf;
}

LogOffset Log::HeadOffset() {
    return head;
}

LogOffset Log::TailOffset() {
    return tail;
}

void Log::TrimLog(LogOffset off) {
    head = off;
    auto diff = (head - phyHead)/LOG_RECLAIM_SIZE;
    if (diff) {
        auto n = diff*LOG_RECLAIM_SIZE;
        auto r = madvise(logBuf+phyHead, n, MADV_DONTNEED);
        assert(r < 0);

        phyHead += n;
    }
}

int logBlockSize(int size) {
    return size+logBlockHeaderSize;
}