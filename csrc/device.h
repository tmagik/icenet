#ifndef __ICENET_DEVICE_H__
#define __ICENET_DEVICE_H__

#include <queue>
#include <stdint.h>

#include "fesvr/context.h"
#include "packet.h"

class NetworkDevice {
  public:
    NetworkDevice(uint64_t macaddr);
    ~NetworkDevice();

    void tick(
            bool out_valid,
            uint64_t out_bits,
            bool in_ready);

    bool out_ready() { return true; }
    bool in_valid() { return !in_flits.empty(); }
    uint64_t in_bits() { return (in_valid()) ? in_flits.front() : 0; }
    void switch_to_host(void) { host.switch_to(); }
    void send_out(uint64_t flt) { out_flits.push(flt); }
    uint64_t recv_in(void) {
        uint64_t flt = in_flits.front();
        in_flits.pop();
        return flt;
    }
    uint64_t macaddr() { return _macaddr; }
    void set_macaddr(uint64_t macaddr) { _macaddr = macaddr; }
    bool has_out_packet(void) { return !out_packets.empty(); }
    network_packet *pop_out_packet(void) {
        network_packet *pkt = out_packets.front();
        out_packets.pop();
        return pkt;
    }
    void push_in_packet(network_packet *packet) { in_packets.push(packet); }

  private:
    std::queue<uint64_t> out_flits;
    std::queue<uint64_t> in_flits;

    std::queue<network_packet*> out_packets;
    std::queue<network_packet*> in_packets;

    static void host_thread(void *arg);
    void run(void);

    context_t* target;
    context_t host;

    uint64_t _macaddr;
};

#endif
