#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "clock.h"
#include "uart0.h"
#include "tm4c123gh6pm.h"
#include "lab5call.h"
//additional defines + struct
#define MAX_CHARS 80
#define MAX_FIELDS 5

#define null '\0'


void getsUart0(USER_DATA *data){
    uint8_t count = 0;
    char c;
    while(true){
        c = getcUart0();
        if((c == 8||c == 127)){
            if (count > 0)//decrement the count so the last char is overwritten
                count--;
            continue;
        }
        if(c == 13 || c == 10){
            data->buffer[count] = '\0';//add null terminator to the end and return function
            return;
        }
        if(c >= 32){
            data->buffer[count] = c;//store the character in buffer
            count++;
            if(count == MAX_CHARS){
                data->buffer[count] = '\0';
                return;
            }
        }
    }
}
void parseFields(USER_DATA* data){
    uint8_t count = 0;
    data->fieldCount = 0;
    uint8_t i = 0;
    for(i = 0;i < MAX_FIELDS;i++) {
        data->fieldPosition[i] = 0;
        data->fieldType[i] = '\0';
    }
    bool delim = true;
    while(data->buffer[count] != '\0') {
        char parseChar = data->buffer[count];
        if ((parseChar >= 'A' && parseChar <= 'Z')||(parseChar >= 'a' && parseChar <= 'z')) {
            if (delim && data->fieldCount < 5) {
                delim = false;
                data->fieldPosition[data->fieldCount] = count;
                data->fieldType[data->fieldCount] = 'A';
                data->fieldCount++;
            }
        }
        else{
            if((parseChar >= '0' && parseChar <= '9')) {
                if(delim && data->fieldCount < 5) {
                    delim = false;
                    data->fieldPosition[data->fieldCount] = count;
                    data->fieldType[data->fieldCount] = 'N';
                    data->fieldCount++;
                }
            }
            else{
                delim = true;
                data->buffer[count] = null;
            }
        }
        count++;
    }
}
char* getFieldString(USER_DATA* data, uint8_t fieldNumber){
    char result[MAX_CHARS+1];
    uint8_t count = 0;
    uint8_t num = data->fieldPosition[fieldNumber];
    while(data->buffer[num] != '\0') {
        result[count] = data->buffer[num];
        count++;
        num++;
    }
    result[count] = null;
    return result;
}
int32_t getFieldInteger(USER_DATA* data, uint8_t fieldNumber){
    if (data->fieldType[fieldNumber] == 'N') {
        uint8_t count = data->fieldPosition[fieldNumber];
        int32_t result = 0;
        for (count; data->buffer[count] != '\0'; ++count) {
            result = (result*10) + (data->buffer[count] - '0'); //'0' or 48
        }
        return result;
    } else {
        return 0;
    }
}
bool isCommand(USER_DATA* data, const char strC[], uint8_t minArgs){
    if (data->fieldCount >= minArgs){
        if(myComp(data->buffer, strC)){
            return true;
        }
    }
    return false;
}
int myComp(char *s1, char *s2){
  int count = 0;
  while(s1[count]!= '\0'||s2[count]!= '\0'){
       if(s1[count] != s2[count]) {
           return 0;
       }
      count++;
    }
    return 1;
}
