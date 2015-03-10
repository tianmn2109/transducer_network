#ifndef DEBUG_H
#define DEBUG_H

#define SSNP_PLATFORM_ASSERT(message) printf(message)


#define SSNP_ASSERT(message,assertion) do\
                                       { \
	                                        if(!(assertion))\
                                            {\
                                                  SSNP_PLATFORM_ASSERT(message);\
											}\
                                        } while(0)
#define SSNP_ERROR(message,expression,handler) do\
										         { \
												     if(!(expression))\
										             { \
													     SSNP_PLATFORM_ASSERT(message);\
														 handler;\
										             }\
										         }while(0)

#define SSNP_DEBUG_PRINT(message)  SSNP_PLATFORM_ASSERT(message)

#endif
