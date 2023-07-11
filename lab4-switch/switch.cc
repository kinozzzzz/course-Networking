#include "switch.h"

class Switch : public SwitchBase
{
  	public:
		int portnum;
		mac2port forward[256];

		void InitSwitch(int numPorts) override 
		{
			portnum = numPorts;
			for(int i=0;i < 256;i++)
			{
				forward[i].next = &forward[i];
				forward[i].prev = &forward[i];
			}
		}

  		int ProcessFrame(int inPort, char* framePtr) override
		{
			if(inPort == 1)
			{
				for(int i = 0;i < 256;i++)
				{
					mac2port *temp = forward[i].next;
					while(temp != &forward[i])
					{
						//printf("%x,%d      ",temp->addr,temp->counter);
						(temp->counter)--;
						//printf("%x,%d      ",temp->addr,temp->counter);
						//printf("\n");
						if(temp->counter == 0)
						{
							temp->next->prev = temp->prev;
							temp->prev->next = temp->next;
							mac2port *newtemp = temp;
							temp = temp->next;
							delete newtemp;
							continue;
						}
						temp = temp->next;
					}
				}
				printf("\n");
				return -1;
			}
			/*for(int i = 0;i < 12;i++)
			{
				printf("%x:",*(uint8_t*)(framePtr+i));
				if(i==5) printf("  ");
			}
			printf("\n");*/
			mac2port *table = FindInTable(&forward[*(uint8_t*)(framePtr+11)],framePtr+6);
			if(table == NULL)
			{
				//printf("insert!\n");
				mac2port *temp = new mac2port();
				memcpy(temp->addr,framePtr+6,6);
				temp->counter = 10;
				temp->port = inPort;
				TableInsert(&forward[*(uint8_t*)(framePtr+11)],temp);
			}
			else
			{
				table->counter = 10;
			}

			table = FindInTable(&forward[*(uint8_t*)(framePtr+5)],framePtr);
			if(table == NULL)	return 0;
			else
			{
				if(inPort == table->port) return -1;
				return table->port;
			}
		}
};

SwitchBase* CreateSwitchObject() 
{
  	return new Switch();
}