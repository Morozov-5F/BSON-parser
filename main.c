#include <stdlib.h>
#include <stdio.h>

#include "bson.h"

int main(int argc, const char * argv[])
{
    
    BSON_Context ctx; BSON_Document doc;
    FILE * file = fopen("event.bson", "r");
    
    if(!file)
        return EXIT_FAILURE;
    fseek(file, 0, SEEK_END);
    doc.size = (int)ftell(file);
    rewind(file);
    
    doc.data = (byte *)malloc(sizeof(byte) * doc.size);
    fread(doc.data, sizeof(byte), doc.size, file);
    fclose(file);
    
    if(BSON_Init(&doc, &ctx) != BSON_OPERATION_SUCCESS)
        return EXIT_FAILURE;
    
    BSON_Context eventContext;
    if(BSON_Open(NULL, &ctx, &eventContext)!= BSON_OPERATION_SUCCESS)
        return EXIT_FAILURE;
    
    int i, t, source, severity;
    char * message = NULL;
    BSON_Extract_Int32("type", &eventContext, &t);
    BSON_Extract_Int32("source", &eventContext, &source);
    BSON_Extract_Int32("severity", &eventContext, &severity);
    BSON_Extract_String(NULL, &eventContext, &message);
    
    printf("type: %i; source: %i; severity: %i;\
           \nmessage: %s\n", t, source, severity, message);
    
    BSON_Context paramContext;
    if(BSON_Open("param", &eventContext, &paramContext))
        return EXIT_FAILURE;
        
    for(i = 0; i < 3; ++i)
    {
        BSON_Context inCtx;
        if(BSON_Open(NULL, &paramContext, &inCtx))
            return EXIT_FAILURE;
        int num, type;
        BSON_Extract_Int32(NULL, &inCtx, &num);
        BSON_Extract_Int32(NULL, &inCtx, &type);
        char * value = NULL;
        if(type == 3)
            BSON_Extract_String(NULL, &inCtx, &value);
        printf("num: %i; type: %i; value: %s\n", num, type, value);
        BSON_Fetch(NULL, &paramContext);
    }
    
    BSON_Finalize(&doc);
    
    return EXIT_SUCCESS;
}
