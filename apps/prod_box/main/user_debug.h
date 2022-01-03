#ifndef __USER_DEBUG_H__
#define __USER_DEBUG_H__

#include <stdio.h>
#include <string.h>

#define USER_DEBUG_PRINT  1
#define ASSERT_INFO_PRINT  1

extern int user_debug_enabled;

// ==== LOGGING ====

#ifdef __GNUC__
#define SHORT_FILE strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__
#else
#define SHORT_FILE strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__
#endif

//#define SHORT_FILE strrchr(__FILE__, '/')+1


#if USER_DEBUG_PRINT
#define custom_log(N,M, ...) \
	   do{if (user_debug_enabled == 0)break;\
		printf("[%s: %s:%4d] " M "\r\n", N, SHORT_FILE, __LINE__, ##__VA_ARGS__);}while(0)

#define custom_hex_log(name, p_data, len) \
    do{\
        uint32_t i; \
        printf("[%s][%s:%d]", SHORT_FILE,__FUNCTION__,__LINE__);\
        printf("%s ",name);  \
        for(i=0;i<len;i++)\
        {\
          printf("%02X ", *(p_data+i));\
        }\
        printf("\n");\
      }while(0)


#else

#define custom_log(N, M, ...)			 
#endif


#if ASSERT_INFO_PRINT                                       
#define debug_print_assert(A,B,C,D,E,F) do {printf("[ESP8266:%s:%s:%4d] **ASSERT** %s""\r\n", D, F, E, (C!=NULL) ? C : "" );}while(0==1)
#else 
#define debug_print_assert(A,B,C,D,E,F)
#endif

#define unlikely( EXPRESSSION )     !!(EXPRESSSION)


#define require_action( X, LABEL, ACTION )                                                              \
	do                                                                                                  \
	{                                                                                                   \
		if( unlikely( !(X) ) )                                                                          \
		{                                                                                               \
			debug_print_assert( 0, #X, NULL, SHORT_FILE, __LINE__, __PRETTY_FUNCTION__ );                 \
			{ ACTION; }                                                                                 \
			goto LABEL;                                                                                 \
		}                                                                                               \
																										\
	}   while( 1==0 )


#define CUSTOM_PARAM_CHECK(con, err) do { \
	if (!(con)) {custom_log("param","parameter error: %s", #con);\
	  return err;} \
	}while (0)



#define CUSTOM_ERROR_CHECK(con, err, format, ...) do { \
if (con) { \
 custom_log("error",format, ##__VA_ARGS__);\
return err;} \
}while (0)

#endif


