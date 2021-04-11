/*******************************************************************************
 * UDP Header file
 *******************************************************************************/

#ifndef _udp_hpp_
#define _udp_hpp_



int UDP_init( void );
int UDP_Engine( void );
int UDP_SendUDP(char *msg, int length);
void UDP_Receive( void );




#endif // _udp_hpp_
