/*******************************************************************************
 *
 * Making some changes to the below code, orignial credits, see below!!
 *
 *******************************************************************************/

/*******************************************************************************
 *
 * Copyright (c) 2015 Thomas Telkamp
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 *******************************************************************************/
 #include <stdio.h>
 #include "hal.h"
 #include "udp.h"


 using namespace std;















// Main programme with loop the loop
 int main ()
 {

     // Initialise the Hardware Abstraction Layer (HAL)
     HAL_Init();

     // Initalise the UDP Packet forwarder
     UDP_Init();


     // Loop the loop
     while(1) {
         // Execute the HAL engine in the main loop
         HAL_Engine();

         // Execute the UDP engine in the main loop
         UDP_Engine();

         // not to go crasy with the calls
         delay(1);
     }
     // never get to here if all is well
     return (0);

 }
