#ifndef COMPNET_LAB4_SRC_SWITCH_H
#define COMPNET_LAB4_SRC_SWITCH_H

#include "types.h"
#include <cstring>
#include <stdio.h>
class SwitchBase {
 public:
  SwitchBase() = default;
  ~SwitchBase() = default;

  virtual void InitSwitch(int numPorts) = 0;
  virtual int ProcessFrame(int inPort, char* framePtr) = 0;
};

extern SwitchBase* CreateSwitchObject();

struct mac2port
{
    mac_addr_t addr;
    int port;
    unsigned int counter;
    mac2port *next;
    mac2port *prev;
};

mac2port * FindInTable(mac2port *table,void *addr)
{
    mac2port *temp = table->next;
    //printf("    In FindInTable:\n");
    while(temp != table)
    {
        /*for(int i=0;i < 6;i++)
            printf("%x:",*(uint8_t*)(table->addr+i));
        printf("\n");
        for (int i=0; i < 6; i++)
            printf("%x:",*(uint8_t*)(addr+i));
        printf("\n");*/
        if(memcmp(temp->addr,addr,6) == 0)
        {
            return temp;
        }
        temp = temp->next;
    }
    return NULL;
}

void TableInsert(mac2port *table,mac2port *src)
{
    src->next = table->next;
    src->prev = table;
    src->next->prev = src;
    table->next = src;
    /*
    for(int i=0;i < 6;i++)
        printf("%x:",*(uint8_t*)(src->addr+i));
    */
}

#endif  // ! COMPNET_LAB4_SRC_SWITCH_H
