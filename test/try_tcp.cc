#include "xy_api.h"

int main(int argc, char *argv[]) {
  struct rte_mempool *buf_pool = xy_setup(argc, argv);

  uint16_t port = 0;
  uint16_t rx_rings = 1;
  uint16_t tx_rings = 1;
  uint16_t nb_rxd = 128;
  uint16_t nb_txd = 512;
  xy_dev_port_init(buf_pool, &xy_this_mac, port, rx_rings, tx_rings,
                   nb_rxd, nb_txd);
  return 0;
}