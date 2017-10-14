#include "device.h"

#include <stdio.h>

void NetworkDevice::host_thread(void *arg)
{
    NetworkDevice *netdev = static_cast<NetworkDevice*>(arg);
    netdev->run();

    while (true)
        netdev->target->switch_to();
}

NetworkDevice::NetworkDevice(uint64_t macaddr)
{
    _macaddr = macaddr;
    target = context_t::current();
    host.init(host_thread, this);
}

NetworkDevice::~NetworkDevice()
{
}

void NetworkDevice::run(void)
{
    network_packet *send_packet = NULL;
    network_packet *recv_packet;

    while (true) {
        while (!out_flits.empty()) {
            if (send_packet == NULL) {
                int len = out_flits.front() & 0xffff;
                send_packet = new network_packet;
                network_packet_init(send_packet, len);
            }

            network_packet_add(send_packet, out_flits.front());
            out_flits.pop();

            if (network_packet_complete(send_packet)) {
                out_packets.push(send_packet);
                send_packet = NULL;
            }
        }

        while (!in_packets.empty()) {
            recv_packet = in_packets.front();
            recv_packet->data[0] &= ~0xffffL;
            recv_packet->data[0] |= (recv_packet->len & 0xffff);

            for (int i = 0; i < recv_packet->len; i++)
                in_flits.push(recv_packet->data[i]);

            in_packets.pop();
            delete recv_packet;
        }

        target->switch_to();
    }
}


void NetworkDevice::tick(
            bool out_valid,
            uint64_t out_bits,
            bool in_ready)
{
    if (out_valid && out_ready())
        out_flits.push(out_bits);

    if (in_valid() && in_ready)
        in_flits.pop();
}
