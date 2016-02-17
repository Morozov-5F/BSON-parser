/*!
 *  @header bson.h Данный модуль позволяет реализовать чтение файлов BSON.
 *  @author Евгений Морозов, 11.01.2014
 */
#ifndef _BSON_
#define _BSON_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/*!
 *  @abstract Переопределение unsigned char для удобства работы.
 */
typedef unsigned char byte;

#ifdef WINDOWS
typedef long long long;
#endif

#define RESERVE_CHECK (sizeof(byte *) - sizeof(long))

/*!
 *  @abstract   Структура, описывающая весь документ.
 *
 *  @discussion Эта структура используется для передачи данных о документе модулю BSON.
 *
 *  @field data     Данные документа
 *  @field size     Размер документа
 *  @field RESERVED Поле для выравнивания структуры
 *  @seealso BSON_Init Функция BSON_Init
 */
typedef struct BSON_Document_def
{
    byte * data;
    long size;
    byte RESERVED [RESERVE_CHECK];
} BSON_Document;

/*!
 *  @abstract   Структура, описывающая контекст в документе.
 *
 *  @discussion В контекстах не рекомендуется что-либо изменять, любые изменения 
 *  контекстов вне функций модуля могут повлечь за собой сбои в работе программы.
 *
 *  @field document       Документ, к которому относится контекст
 *  @field startPosition  Позиция контекста в документе
 *  @field position       Текущая позиция в документе
 *  @field size           Размер контекста
 *  @field RESERVED       Поле для выравнивания структуры
 */
typedef struct BSON_Context_def
{
    BSON_Document const * document;
    byte * startPosition;
    byte * position;
    long size;
    byte RESERVED [RESERVE_CHECK];
} BSON_Context;

/*!
 * @enum  BSON_ERROR Некоторые константы не используются.
 *
 * @const BSON_OPERATION_SUCCESS    Успешное выполнение функций модуля
 * @const BSON_POS_OUT_OF_RANGE     Выход за пределы контекста или документа
 * @const BSON_MEMORY_NOT_ALLOCATED Невозможно выделить память под документ
 * @const BSON_BAD_CONTEXT          Ошибки в контексте
 *
 * @abstract Перечисление, задающее коды ошибок для функций модуля.
 */
enum BSON_ERROR
{
    BSON_OPERATION_SUCCESS,
    BSON_POS_OUT_OF_RANGE,
    BSON_MEMORY_NOT_ALLOCATED,
    BSON_MEMORY_CORRUPTED,
    BSON_DOCUMENT_NOT_FOUND,
    BSON_BAD_CONTEXT
};

/*!
 *  @abstract Инициализирует работу с документом
 *
 *  @discussion Загружает документ в собственную память, позволяя работать с ним. Должен
 *  вызываться перед началом работы с модулем.
 *
 *  @param inputDocument    Структура BSON_Document, сформированная до вызова функции,
 *  которая содержит информацию о документе
 *  @param resultingContext Контекст, полученный после анализа документа. Этот контекст
 *  представляет собой самый верхний уровень документа
 *
 *  @return BSON_OPERATION_SUCCESS при успешном выполнении, BSON_MEMORY_NOT_ALLOCATED при
 *  ошибке выделения памяти под данные документа и BSON_MEMORY_CORRUPTED при нарушении 
 *  целостности файла
 *
 *  @seealso BSON_ERROR
 *
 */
 
int BSON_Init(const BSON_Document * inputDocument, BSON_Context * resultingContext);
/*!
 *  Открывает документ и массив для чтения. После выполнения этой функции можно
 *  извлекать любые другие типы из документа
 *
 *  @param name          Имя блока, который необходимо открыть.
 *  Если name == NULL, то документ открывается по текущей позиции
 *  @param parentContext Родительский контекст, в котором нужно открыть документ
 *  @param childContext  Новый контекст, в который будут записаны данные нового документа
 *
 *  @return BSON_OPERATION_SUCCESS при успешном открытии и BSON_POS_OUT_OF RANGE,
 *  если документ не найден в родительском контексте
 */
int BSON_Open(char * name, const BSON_Context * parentContext, 
			  BSON_Context * childContext);
/*!
 *  @abstract Пропускает текущий блок или ищет блок с заданным именем.
 *
 *  @discussion Перемещает указатель в контексте на найденный блок.
 *  Ищет строго на текущем уровне вложенности, без открытия других уровней.
 *  Если блок был найден, выставляет указатель на начало блока (на заголовочный байт)
 *
 *  @param name    Имя блока. Для пропуска одного блока использовать NULL
 *  @param context Контекст, в котором производится поиск
 *
 *  @return Возвращает код результата, определенный в BSON_ERROR:
 *  BSON_OPERATION_SUCCESS при успехе
 *  и BSON_POS_OUT_OF RANGE, если ничего не найдено
 */
 
int BSON_Fetch(char * name, BSON_Context * context);
/*!
 *  @abstract Проверяет контекст на предмет ошибок
 *
 *  @param context Контекст, который нужно проверить
 *
 *  @return BSON_OPERATION_SUCCESS в случае правильного контекста и BSON_BAD_CONTEXT 
 *  при наличии ошибок в контексте.
 *
 *  @discussion Проверки включают в себя проверку на NULL, проверку значения size и
 *  проверку указателей внутри контекста на NULL. Также есть дополнительные проверки на
 *  разумность данных в контексте.
 *
 *  @seealso BSON_Context
 */
 
int BSON_Check_Context(const BSON_Context * context);
/*!
 *  @abstract Завершающий метод модуля. 
 * 
 *  @discussion Освобождает всю память, занимаемую документом. 
 *  Обязательно должен вызываться после работы с документом.
 *
 *  @param document Документ, который нужно очистить
 *  
 *  @return BSON_OPERATION_SUCCESS, если удалось освободить память. 
 * BSON_DOCUMENT_NOT_FOUND если документ равен NULL
 */
 
int BSON_Finalize(BSON_Document * document);
/*!
 *  @abstract Извлекает из контекста значение типа Double 
 *
 *  @discussion Извлекает только на текущем уровне, не поднимаясь на уровни выше или 
 *  спускаясь ниже
 *
 *  @param name    Имя извлекаемого поля
 *  @param context Контекст, в котором поле должно быть извлечено
 *  @param result  Извлеченное число (выходной параметр)
 *
 *  @return BSON_OPERATION_SUCCESS при успешном извлечении, BSON_POS_OUT_OF_RANGE, 
 *  если поле не найдено
 */
 
int BSON_Extract_Double   (char * name, BSON_Context * context, double * result);
/*!
 *  @abstract Извлекает строку в кодировке UTF-8 из контекста
 *
 *  @discussion Стоит обратить внимание, что указателю *result рекомендуется значение NULL,
 *  но ничего не препятствует обратной ситуации. В таком случае, все данные, которые были в
 *  строке, будут потеряны.
 *
 *  @param name    Имя поля
 *  @param context Контекст, из которого производится извлечение
 *  @param result  Результирующая строка (выходной параметр)
 *
 *  @return BSON_OPERATION_SUCCESS при успехе, BSON_BAD_CONTEXT при неправильном контексте,
 *  BSON_MEMORY_NOT_ALLOCATED, если не удалось выделить память под результат и
 *  BSON_POS_OUT_OF_RANGE, когда не найдено соответствующее поле.
 *
 */
 
int BSON_Extract_String   (char * name, BSON_Context * context, char  ** result);
/*!
 *  @abstract Извлекает массив двоичных данных из контекста
 *
 *  @discussion Стоит обратить внимание, что указателю *result рекомендуется значение NULL,
 *  но ничего не препятствует обратной ситуации. В таком случае, все данные, которые были в
 *  строке, будут потеряны.
 *
 *  @param name    Имя поля
 *  @param context Контекст, из которого производится извлечение
 *  @param result  Результирующий массив байт (выходной параметр)
 *
 *  @return BSON_OPERATION_SUCCESS при успехе, BSON_BAD_CONTEXT при неправильном
 *  контексте, BSON_MEMORY_NOT_ALLOCATED, если не удалось выделить память под 
 *  результат и BSON_POS_OUT_OF_RANGE, когда не найдено соответствующее поле.
 *
 *  @seealso byte
 */
int BSON_Extract_Binary   (char * name, BSON_Context * context, byte  ** result);

/*!
 *  @abstract Извлекает логическое(булевское) значение из контекста
 *
 *  @discussion Значение переменной result после выполнения функции: 
 *  0x0 при 'false' и 0x1 при 'true'
 *
 *  @param name    Имя поля
 *  @param context Контекст, из котрого происходит извлечение
 *  @param result  Извлеченное значение (выходной параметр)
 *
 *  @return BSON_OPERATION_SUCCESS при успешном извлечении, BSON_BAD_CONTEXT при ошибках
 *  в контексте и BSON_POS_OUT_OF_RANGE, если искомое поле не найдено
 *
 *  @seealso byte
 */
int BSON_Extract_Boolean  (char * name, BSON_Context * context, byte   * result);

/*!
 *  @abstract Извлекает дату в формате UTC(кол-во миллисекунд с 1 января 1970 года)
 *  @discussion В данной реализации извлекает в тип time_t, определенный в заголовочном
 *  файле <time.h>
 *
 *  @param name    Имя поля
 *  @param context Контекст, из которого происходит извлечение
 *  @param result  Извлеченная дата и время (выходной параметр)
 *
 *  @return BSON_OPERATION_SUCCESS при успешном извлечении, BSON_BAD_CONTEXT при ошибках
 *  в контексте и BSON_POS_OUT_OF_RANGE, если искомое поле не найдено
 */
int BSON_Extract_DateTime (char * name, BSON_Context * context, time_t   * result);

/*!
 *  @abstract Извлекает из контекста число типа int32
 *
 *  @param name    Имя поля
 *  @param context Контекст, из которого происходит извлечение
 *  @param result  Извлеченное число (выходной параметр)
 *
 *  @return BSON_OPERATION_SUCCESS при успешном извлечении, BSON_BAD_CONTEXT при ошибках
 *  в контексте и BSON_POS_OUT_OF_RANGE, если искомое поле не найдено
 */
int BSON_Extract_Int32    (char * name, BSON_Context * context, int    * result);

/*!
 *  @abstract Извлекает из контекста число типа int64
 *
 *  @param name    Имя поля
 *  @param context Контекст, из которого происходит извлечение
 *  @param result  Извлеченное число (выходной параметр)
 *
 *  @return BSON_OPERATION_SUCCESS при успешном извлечении, BSON_BAD_CONTEXT при ошибках
 *  в контексте и BSON_POS_OUT_OF_RANGE, если искомое поле не найдено
 */
int BSON_Extract_Int64    (char * name, BSON_Context * context, long   * result);

#endif