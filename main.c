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
 #include <wiringPi.h>    // used in this module for the function delay()
 #include "hal.h"         // Hardware abstraction layer (lora)
 #include "udp.h"         // UDP Layer definitions
 #include "gateway.h"     // Application Layer = Gateway definitions

 // Main programme with loop the loop
 int main ()
 {
     // Initialise the Hardware Abstraction Layer (HAL)
     HAL_Init();

     // Initalise the UDP Packet forwarder
     UDP_Init();

     // Initialise the application, in this case the Gateway
     GW_Init();

     // Loop the loop, should do exit when there is an error
     while(1) {
         // Execute the HAL engine in the main loop
         HAL_Engine();

         // Execute the UDP engine in the main loop
         UDP_Engine();

         // Execute the Gateway engine in the main Loop
         GW_Engine();

         // not to go crasy with the calls
         delay(1);
     }
     // never get to here if all is well
     return (0);

 }
