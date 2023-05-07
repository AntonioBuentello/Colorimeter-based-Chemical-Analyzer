
#ifndef LAB5CALL_H
#define LAB5CALL_H

//additional defines + struct
#define MAX_CHARS 80
#define MAX_FIELDS 5
typedef struct _USER_DATA{
    char buffer[MAX_CHARS + 1]; //null terminator added
    uint8_t fieldCount;
    uint8_t fieldPosition[MAX_FIELDS];
    char fieldType[MAX_FIELDS];
} USER_DATA;


void getsUart0(USER_DATA *data);
void parseFields(USER_DATA* data);
char* getFieldString(USER_DATA* data, uint8_t fieldNumber);
int32_t getFieldInteger(USER_DATA* data, uint8_t fieldNumber);
bool isCommand(USER_DATA* data, const char strC[], uint8_t minArgs);
int myComp(char *s1, char *s2);

#endif
