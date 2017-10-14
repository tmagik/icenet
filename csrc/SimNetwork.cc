#include <vpi_user.h>
#include <svdpi.h>

#include "device.h"
#include "switch.h"

class NetworkSwitch *netsw = NULL;
class NetworkDevice *netdev = NULL;

extern "C" void network_init(
        const char *devname)
{
    netsw = new NetworkSwitch(devname);
    netdev = new NetworkDevice(random_macaddr());

    netsw->add_device(netdev);
}

extern "C" void network_tick(
        unsigned char out_valid,
        unsigned char *out_ready,
        long long     out_bits,

        unsigned char *in_valid,
        unsigned char in_ready,
        long long     *in_bits,

        long long     *macaddr)
{
    if (!netdev || !netsw) {
        *out_ready = 0;
        *in_valid = 0;
        *in_bits = 0;
        return;
    }

    netdev->tick(out_valid, out_bits, in_ready);
    netdev->switch_to_host();

    netsw->distribute();
    netsw->switch_to_worker();

    *out_ready = netdev->out_ready();
    *in_valid = netdev->in_valid();
    *in_bits = netdev->in_bits();
    *macaddr = netdev->macaddr();
}
