#ifndef _os_hpp_
#define _os_hpp_


// Define the functions and pocedures
void OS_PrintFrame(uint8_t *Frame, int LEN);
void OS_PrintBin(int x);
struct json_object * OS_GetJSONFromNVM(char *ConfigName);
int OS_CheckNVMExists( char *ConfigName);
int OS_CreateNVMEntry( char *ConfigName);
int OS_WriteJSONtoNVM( char *ConfigName, struct json_object *JSON_Config);


static const int CONFIG_FILE_SIZE = 1024; /// JSON file buffer is 1024 bytes, might need to be changed


#endif // _os_hpp_
