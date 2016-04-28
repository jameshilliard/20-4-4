// Copyright 2010 by Tivo Inc, All Rights Reserved

#define AR8328_ETH_PHY 2  // ethernet
#define AR8328_MOCA_PHY 6 // moca 

void ar8328_write(struct net_device *dev, uint32 addr, uint32 data);
uint32 ar8328_read(struct net_device *dev, uint32 addr);
void ar8328_config(struct net_device *dev);
void ar8328_mac6_ctl(struct net_device *dev, int enable);
