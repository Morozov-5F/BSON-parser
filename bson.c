#include "bson.h"

#ifdef DATA_ALIGNMENT

#define READ_INT_32(src, dest) (dest) = *(int *)((src));
#define READ_INT_64(src, dest) (dest) = *(long *)((src));

#else

#define READ_INT_32(src, dest) memcpy(&(dest), src, sizeof(int))
#define READ_INT_64(src, dest) memcpy(&(dest), src, sizeof(long))
    
#endif

#define CHECK_CONTEXT(context) {if(BSON_Check_Context((BSON_Context *)context) ==\
BSON_BAD_CONTEXT) return BSON_BAD_CONTEXT;}
#define GET_NAME_LENGTH(pos, ctx) BSON_Get_Name_Length((pos), (ctx))

static int BSON_Get_Name_Length(byte * pos, const BSON_Context * ctx)
{
    int len = (int)((byte *)memchr(pos + 1, 0x0, ctx->startPosition + ctx->size - pos) 
    - pos);
    return len;
}

int BSON_Init(const BSON_Document * inputDocument, BSON_Context * resultingContext)
{
    if(inputDocument == NULL)
        return BSON_MEMORY_NOT_ALLOCATED;

    if(resultingContext == NULL)
        return BSON_BAD_CONTEXT;
    
    resultingContext->document = (BSON_Document *)inputDocument;
    
    int docLen;
    READ_INT_32(resultingContext->document->data, docLen);
    
    if ((docLen != resultingContext->document->size) || 
        (resultingContext->document->data[resultingContext->document->size - 1] != 0x0))
        return BSON_MEMORY_CORRUPTED;
    
    resultingContext->startPosition = resultingContext->position = 
    	resultingContext->document->data + 4;
    resultingContext->size = resultingContext->document->size;
    
    return BSON_OPERATION_SUCCESS;
}

int BSON_Open(char * name, const BSON_Context * parentContext, BSON_Context * childContext)
{
    /* Сначала проверяем под указателем, в случае неудачи, ищем дальше с помощью Fetch(); */
    CHECK_CONTEXT(parentContext);
    if(childContext == NULL)
        return BSON_BAD_CONTEXT;
    
    int len = name ? (int)strlen(name) + 1 : 0;
    byte * currentPos = parentContext->position;
    
    /* Копируем данные в новый контекст */
    childContext->document = parentContext->document;
    childContext->position = parentContext->position;
    childContext->size = parentContext->size;
    childContext->startPosition = parentContext->startPosition;
    /* Если имя на текущей позиции совпадает, то идем дальше */
    if(memcmp(currentPos + 1, name, len))
    {
        int fetchResult = BSON_Fetch(name, childContext);
        if(fetchResult == BSON_POS_OUT_OF_RANGE)
            return BSON_POS_OUT_OF_RANGE;
        
        currentPos = childContext->position;
    }
    /* Проверки на правильность документа */
    len = len ? len : GET_NAME_LENGTH(currentPos, parentContext);
    if(len > childContext->document->size)
        return BSON_MEMORY_CORRUPTED;
    
    int size;
    READ_INT_32(currentPos + len + 1, size);
    currentPos += len + 1 + 4;
    
    if(*(currentPos + size - 5) != 0x0)
        return  BSON_MEMORY_CORRUPTED;
    
    childContext->size = size;
    childContext->startPosition = childContext->position = currentPos;
    
    return BSON_OPERATION_SUCCESS;
}

int BSON_Fetch(char * name, BSON_Context * context)
{
    CHECK_CONTEXT(context);
    
    byte * currentPos = context->position;
    /* Байт-заголовок элемента BSON */
    byte headerByte;
    /* Массив известных постоянных смещений для полей */
    byte typicalOffsets [] = {255, 8, 4, 0, 0, 4, 0, 0, 1, 1, 8, 0, 0, 0, 0, 0, 4, 8, 8};
    int len = (name == NULL) ? 0 : (int)(strlen(name) + 1), toSkip = 0, offset = 0;
    /* Сначала прпускаем весь блок, а потом смотрим имя следующего.
       Подразумевается, что блок под указателем нам не интересен. */
    do
    {
        headerByte = *currentPos;
        /* Ищем конец строки имени в текущем блоке */
        toSkip = GET_NAME_LENGTH(currentPos, context);
        /* Пропускаем это имя вместе с заголовочным байтом */
        currentPos += toSkip + 1;
        /* Заголовочный байт, по факту, определяет размер блока */
        offset = typicalOffsets[headerByte];
        /* Если размер блока нам заранее неизвестен, то нужно прочитать его */
        if(offset == 0 || headerByte == 0x02 || headerByte == 0x05)
        {
            int toAdd;
            READ_INT_32(currentPos, toAdd);
            offset += toAdd;
        }
        currentPos += offset;
        /* Если вышли за границу, то завершаем функцию с ошибкой */
        if(currentPos >= context->startPosition + context->size)
            return BSON_POS_OUT_OF_RANGE;
    } while (memcmp(currentPos + sizeof(byte), name, len));
    /* После цикла присваиваем позиции контекста найденный нами блок. */
    context->position = currentPos;
    return BSON_OPERATION_SUCCESS;
}

int BSON_Extract_Int32(char * name, BSON_Context * context, int * result)
{
    CHECK_CONTEXT(context);
    
    int len = name ? (int)strlen(name) + 1 : 0;
    byte * prevPos = context->position;
    if(*(context->position) != 0x10 || memcmp(context->position + 1, name, len))
    {
        do
        {
            int fetchResult = BSON_Fetch(name, context);
            if(fetchResult == BSON_POS_OUT_OF_RANGE)
            {
                context->position = prevPos;
                return BSON_POS_OUT_OF_RANGE;
            }
            
        } while (*(context->position) != 0x10);
    }

    len = len ? len : GET_NAME_LENGTH(context->position, context);
    READ_INT_32(context->position + 1 + len, *result);
    context->position = context->position + 1 + len + 4;
    
    return BSON_OPERATION_SUCCESS;
}

int BSON_Extract_Int64(char * name, BSON_Context * context, long * result)
{
    CHECK_CONTEXT(context);
    
    int len = name ? (int)strlen(name) + 1 : 0;
    byte * prevPos = context->position;
    if(*(context->position) != 0x12 || memcmp(context->position + 1, name, len))
    {
        do
        {
            int fetchResult = BSON_Fetch(name, context);
            if(fetchResult == BSON_POS_OUT_OF_RANGE)
            {
                context->position = prevPos;
                return BSON_POS_OUT_OF_RANGE;
            }
            
        } while (*(context->position) != 0x12);
    }
    
    len = len ? len : GET_NAME_LENGTH(context->position, context);
    READ_INT_64(context->position + 1 + len, *result);
    context->position = context->position + 1 + len + 4;
    
    return BSON_OPERATION_SUCCESS;
}

int BSON_Extract_Double(char * name, BSON_Context * context, double * result)
{
    CHECK_CONTEXT(context);
    
    int len = name ? (int)strlen(name) + 1 : 0;
    
    byte * prevPos = context->position;
    
    if(*(context->position) != 0x01 || memcmp(context->position + 1, name, len))
    {
        do
        {
            int fetchResult = BSON_Fetch(name, context);
            if(fetchResult == BSON_POS_OUT_OF_RANGE)
            {
                context->position = prevPos;
                return BSON_POS_OUT_OF_RANGE;
            }
            
        } while (*(context->position) != 0x01);
    }
    
    len = len ? len : GET_NAME_LENGTH(context->position, context);
    *result = *((double *)(context->position + 1 + len));
    context->position = context->position + 1 + len + 4;
    
    return BSON_OPERATION_SUCCESS;
}

int BSON_Extract_String(char * name, BSON_Context * context, char ** result)
{
    CHECK_CONTEXT(context);
    
    int len = name ? (int)strlen(name) + 1 : 0;
    byte * prevPos = context->position;
    if(*(context->position) != 0x02 || memcmp(context->position + 1, name, len))
    {
        do
        {
            int fetchResult = BSON_Fetch(name, context);
            if(fetchResult == BSON_POS_OUT_OF_RANGE)
            {
                context->position = prevPos;
                return BSON_POS_OUT_OF_RANGE;
            }
            
        } while (*(context->position) != 0x02);
    }
    
    len = len ? len : GET_NAME_LENGTH(context->position, context);
    context->position = context->position + 1 + len;
    int strSize;
    READ_INT_32(context->position, strSize);
    
    *result = (char *)realloc(*result, sizeof(char) * strSize);
    if(result == NULL)
        return BSON_MEMORY_NOT_ALLOCATED;
    
    memcpy(*result, context->position + 4, strSize);
    context->position += 4 + strSize;
    
    return BSON_OPERATION_SUCCESS;
}

int BSON_Extract_Binary(char * name, BSON_Context * context, byte ** result)
{
    CHECK_CONTEXT(context);
    
    int len = name ? (int)strlen(name) + 1 : 0;
    byte * prevPos = context->position;
    if(*(context->position) != 0x05 || memcmp(context->position + 1, name, len))
    {
        do
        {
            int fetchResult = BSON_Fetch(name, context);
            if(fetchResult == BSON_POS_OUT_OF_RANGE)
            {
                context->position = prevPos;
                return BSON_POS_OUT_OF_RANGE;
            }

            
        } while (*(context->position) != 0x05);
    }
    
    len = len ? len : GET_NAME_LENGTH(context->position, context);
    context->position = context->position + 1 + len;
    int binSize;
    READ_INT_32(context->position, binSize);
    
    *result = (byte *)realloc(*result, sizeof(byte) * binSize);
    if(result == NULL)
        return BSON_MEMORY_NOT_ALLOCATED;
    
    memcpy(*result, context->position + 4, binSize);
    context->position += 4 + binSize;
    
    return BSON_OPERATION_SUCCESS;
}

int BSON_Extract_Boolean(char * name, BSON_Context * context, byte * result)
{
    CHECK_CONTEXT(context);
    
    int len = name ? (int)strlen(name) + 1 : 0;
    byte * prevPos = context->position;
    if(*(context->position) != 0x08 || memcmp(context->position + 1, name, len))
    {
        do
        {
            int fetchResult = BSON_Fetch(name, context);
            if(fetchResult == BSON_POS_OUT_OF_RANGE)
            {
                context->position = prevPos;
                return BSON_POS_OUT_OF_RANGE;
            }

            
        } while (*(context->position) != 0x08);
    }
    
    len = len ? len : GET_NAME_LENGTH(context->position, context);
    *result = *(context->position + 1 + len);
    context->position = context->position + 1 + len + 1;
    
    return BSON_OPERATION_SUCCESS;
}

int BSON_Extract_DateTime (char * name, BSON_Context * context, time_t * result)
{
    CHECK_CONTEXT(context);
    
    int len = name ? (int)strlen(name) + 1 : 0;
    byte * prevPos = context->position;
    if(*(context->position) != 0x09 || memcmp(context->position + 1, name, len))
    {
        do
        {
            int fetchResult = BSON_Fetch(name, context);
            if(fetchResult == BSON_POS_OUT_OF_RANGE)
            {
                context->position = prevPos;
                return BSON_POS_OUT_OF_RANGE;
            }
        } while (*(context->position) != 0x09);
    }
    
    len = len ? len : GET_NAME_LENGTH(context->position, context);
    READ_INT_64(context->position + 1 + len, *result);
    context->position = context->position + 1 + len + 4;
    
    return BSON_OPERATION_SUCCESS;

}

int BSON_Check_Context(const BSON_Context * context)
{
    /* Базовые проверки */
    if(context == NULL || context->startPosition == NULL || 
       context->position == NULL || context->document == NULL)
        return BSON_BAD_CONTEXT;
   
    /* Рассширенные проверки */
    if(context->position < context->document->data || 
       context->position > context->document->data + context->document->size || 
       context->startPosition < context->document->data || 
       context->startPosition > context->document->data + context->document->size)
        return BSON_BAD_CONTEXT;
    
    return BSON_OPERATION_SUCCESS;
}

int BSON_Finalize(BSON_Document * document)
{
    if (document == NULL)
        return BSON_DOCUMENT_NOT_FOUND;
        
    free(document->data);
    document->size = 0;
    
    return BSON_OPERATION_SUCCESS;
}