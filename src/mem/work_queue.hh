/*
 * Copyright (c) 2012-2013 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2001-2005 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Subhankar Pal
 */

/**
 * @file
 * WorkQueue declaration
 */

#ifndef __MEM_WORK_QUEUE_HH__
#define __MEM_WORK_QUEUE_HH__

#include <list>
#include <queue>

#include "mem/abstract_mem.hh"
#include "mem/port.hh"
#include "params/WorkQueue.hh"

/**
 * The work queue is a dual-ported structure that allows pushing
 * and popping of "work items" from the queue.
 *
 * @sa  \ref gem5MemorySystem "gem5 Memory System"
 */

namespace gem5
{

namespace memory
{

class WorkQueue : public AbstractMemory
{
  public:

    enum WorkQueuePort_e {
        PopPortType,
        PushPortType
    };

  private:

    /**
     * A deferred packet stores a packet along with its scheduled
     * transmission time
     */
    class DeferredPacket
    {

      public:

        const Tick tick;
        const PacketPtr pkt;
        WorkQueuePort_e portType;

        DeferredPacket(PacketPtr _pkt, Tick _tick, WorkQueuePort_e _portType) : 
		tick(_tick), pkt(_pkt), portType(_portType)
        { }
    };

    class MemoryPort : public ResponsePort
    {

      private:

        WorkQueue& workQueue;
        WorkQueuePort_e type;

      public:

        MemoryPort(const std::string& _name, WorkQueue& _workQueue,
                         WorkQueuePort_e _type);

      protected:

        void access(PacketPtr pkt);

        Tick recvAtomic(PacketPtr pkt);

        void recvFunctional(PacketPtr pkt);

        bool recvTimingReq(PacketPtr pkt);

        void recvRespRetry();

        AddrRangeList getAddrRanges() const;

    };

    std::queue<uint32_t> workQ;

    // MemoryPort port;
    /**
     * popPort is the port that is hooked up to the work consumer.
     * pushPort is the port that is hooked up to the work producer.
     */
    MemoryPort popPort, pushPort;

    /**
     * Number of entries in the work queue.
     */
    int size;

    /**
     * Latency from that a request is accepted until the response is
     * ready to be sent.
     */
    const Tick latency;

    /**
     * Fudge factor added to the latency.
     */
    // const Tick latency_var;

    /**
     * Internal (unbounded) storage to mimic the delay caused by the
     * actual memory access. Note that this is where the packet spends
     * the memory latency.
     */
    std::list<DeferredPacket> packetQueue;

    /**
     * Bandwidth in ticks per byte. The regulation affects the
     * acceptance rate of requests and the queueing takes place after
     * the regulation.
     */
    // const double bandwidth;

    /**
     * Track the state of the memory as either idle or busy, no need
     * for an enum with only two states.
     */
    //bool isBusy;
    bool isBusy, isEmpty, isFull;

    /**
     * Remember if we have to retry an outstanding request that
     * arrived while we were busy.
     */
    bool retryPop, retryPush;

    /**
     * Remember if we failed to send a response and are awaiting a
     * retry. This is only used as a check.
     */
    bool retryResp;

    // Addresses associated with this work queue.
    Addr popPortAddr, pushPortAddr;

    /**
     * Release the memory after being busy and send a retry if a
     * request was rejected in the meanwhile.
     */
    void release();

    EventFunctionWrapper releaseEvent; //, releasePushPortEvent;

    /**
     * Dequeue a packet from our internal packet queue and move it to
     * the port where it will be sent as soon as possible.
     */
    void dequeue();

    EventFunctionWrapper dequeueEvent;

    /**
     * Detemine the latency.
     *
     * @return the latency seen by the current packet
     */
    Tick getLatency() const;

    /**
     * Upstream caches need this packet until true is returned, so
     * hold it for deletion until a subsequent call
     */
    std::unique_ptr<Packet> pendingDelete;

  public:

    WorkQueue(const WorkQueueParams &p);

    DrainState drain() override;

    Port &getPort(const std::string &if_name,
                  PortID idx=InvalidPortID) override;
    void init() override;
    void access(PacketPtr pkt);

    /**
     * Get the address range
     *
     * @return a single contiguous address range
     */
    Addr getPopPortAddr() const;
    Addr getPushPortAddr() const;

  protected:

    Tick recvAtomic(PacketPtr pkt);

    // void recvFunctional(PacketPtr pkt);

    bool recvTimingReq(PacketPtr pkt, WorkQueuePort_e portType);

    void recvRespRetry();

    /**
     * Stats that count number of pushes and pops to this work queue.
     */
    statistics::Scalar pushes, pops;

  public:
    /**
     * Register stats for this object.
     */
    void regStats();
};

} // namespace memory
} // namespace gem5

#endif //__MEM_WORK_QUEUE_HH__
