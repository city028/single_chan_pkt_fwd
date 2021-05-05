#include <stdint.h>    // Required for unint8 etc
#include <cstdio>      // Required for printf etc
#include<json-c/json.h> // required for json file manipulation
#include "os.h"


/**
 * __Function__: OS_CreateNVMEntry
 *
 * __Description__: Create Item in NVM (in this case a file named: ConfigName)
 *
 * __Input__: ConfigName = File name on a raspberry (this enables multiple files for different porposed if required)
 *
 * __Output__: (integer)Error Code, 1 = File creation error, 0 = ok, no error
 *
 * __Status__: Completed
 *
 * __Remarks__: This function creates a JSON Item in NVM Memory (in the Raspberry case a file but not the contents of the file)
 */
int OS_CreateNVMEntry(char *ConfigName)
{
 /// __Incode Comments:__
 FILE *fp;

 fp = fopen(ConfigName, "w+");
 if (!fp)
 {
   // error
   printf("OS_CreateNVMEntry: Error creating file!");
   return 1; /// Error Code: 1 = Error creating Item (File)
 }
 else
 {
   // Close file and return no error
   fclose(fp);
   return 0; /// Error Code: 0 = No error
 }
}


/**
 * __Function__: OS_CheckNVMExists
 *
 * __Description__: Check if NVM Storage exists for particulair config item
 *
 * __Input__: ConfigName (in case of raspberry, filename)
 *
 * __Output__: (integer)Error Code, 1 = Item (file) does not exist
 *
 * __Status__: Completed
 *
 * __Remarks__: The function now uses a file, but when used on other systems this migh need to
 * be changed to other means of storage
 */
int OS_CheckNVMExists( char *ConfigName)
{
 /// __Incode Comments:__
 FILE *fp;

 // Open file
 if((fp = fopen(ConfigName,"r")) == NULL )
 {
   // Return an error
   printf("OS_CheckNVMExists: Error, Config Item does not exist in NVM!\n");
   return 1; /// Error code 1 = Item (File) does not exist
 }
 else
 {
   // Close file and return no error
   fclose(fp);
   return 0; /// Error code: 0 = Item (File) Exists
 }
}


/**
* __Function__: OS_WriteJSONtoNVM
*
* __Description__: Write a JSON object to NVM
*
* __Input__: Config name = file name on raspberry, Config Item = Json object
*
* __Output__: Error Code, 1 = Error writing JSON to NVM
*
* __Status__: Completed
*
* __Remarks__: Need to install the json-c library on the rasp with:
*
* sudo apt-get install libjson0-dev
*
* sudo apt install libjson-c-dev
*
* Change this procedure in the future to work with None-Volatile Memory (NVM) if required
*/
int OS_WriteJSONtoNVM(char *ConfigName, struct json_object *JSON_Config)
{
 /// __Incode Comments:__
 FILE *fp;
 // Open file
 fp = fopen(ConfigName, "w+");
 if (!fp)
 {
   // error
   printf("OS_WriteJSONtoNVM: Error writing JSON to NVM!\n");
   return 1; /// Error Code: 1 = Error writing JSON to NVM
 }
 else
 {
   // Write this to file
   fprintf( fp, json_object_to_json_string(JSON_Config));
   // Close the file
   fclose(fp);
   return 0; /// Error code: 0 : No error
 }
}



/**
 * __Function__: OS_GetJSONFromNVM
 *
 * __Description__: Get a Json object from a file for which the filename is provided
 *
 * __Input__: Config Item (In Raspberry Pi case the name of the file)
 *
 * __Output__: NULL = Error ; json_object
 *
 * __Status__: Completed
 *
 * __Remarks__: Need to install the json-c library on the rasp with:
 *
 * sudo apt-get install libjson0-dev
 *
 * sudo apt install libjson-c-dev
 *
 * Change this procedure in the future to work with None-Volatile Memory (NVM) if required
 */
struct json_object * OS_GetJSONFromNVM(char *ConfigName)
{
 /// __Incode Comments:__
 FILE *fp;
 char buffer[CONFIG_FILE_SIZE];                   /// JSON file buffer is 1025 bytes, might need to be changed
 struct json_object *ConfigItem;
 struct json_object *devnonce;

 // Open file
 if((fp = fopen(ConfigName,"r")) == NULL )
 {
   // Return an error
   printf("OS_GetJSONFromNVM: Error file: %s cannot be read!\n", ConfigName);
   return NULL; /// Error code: NULL = Error
 }
 else
 {
   // No error so read data
   fread(buffer, CONFIG_FILE_SIZE, 1, fp); /// Check on Buffer size now ar 1024
   fclose(fp);
   // Parse Json
   ConfigItem = json_tokener_parse(buffer);
   if( ConfigItem == NULL)
   {
     // error
     printf("OS_GetJSONFromNVM: Parse error, NVM exists but does not contain JSON!\n");
     return NULL; /// Error code: NULL = Error
   }
   else
   {
     // Debug: Get the devnonce value from the CFG JSON object
     json_object_object_get_ex(ConfigItem, "devnonce", &devnonce);
     //printf("OS_GetJSONFromNVM: Devnonce: %d\n", json_object_get_int(devnonce));
   }
   // No errors return ConfigItem
   return ConfigItem; /// Error Code: > Null = Config Item
 }
}


/**
 * __Function__: OS_PrintBin
 *
 * __Description__: Print the binary value of an integer
 *
 * __Input__: Integer to be printed in Binary
 *
 * __Output__: void
 *
 * __Status__: Complete
 *
 * __Remarks__: None
 */
void OS_PrintBin(int x)
{
   /// __Incode Comments:__
   int i;
   printf("0b");
   for(i=0; i<8; i++)
   {
      if((x & 0x80) !=0)
      {
         printf("1");
      }
      else
      {
         printf("0");
      }
      if (i==3)
      {
         printf(" ");
      }
      x = x<<1;
   }
}

/**
 * __Function__: OS_PrintFrame
 *
 * __Description__: Print the hex and binary values of a provided frame
 *
 * __Input__: Pointer to a Frame, Frame Length
 *
 * __Output__: void
 *
 * __Status__: Complete
 *
 * __Remarks__: None
 */
void OS_PrintFrame(uint8_t *Frame, int LEN)
{
 /// __Incode Comments:__
 int i;

 printf("MSB First\n");
 for(i=0;i<LEN;++i)
 {
   printf("Byte: %02d | ",i);
   printf("Hex Value: 0x%02x | Binary Value: ", (int)Frame[i]);
   OS_PrintBin((int)Frame[i]);
   printf("\n");
 }
 printf("LSB Last\n");

}
