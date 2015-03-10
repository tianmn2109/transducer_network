#ifndef ERR_H
#define ERR_H

#include "opt.h"

typedef s8t err_t;

#define ERR_OK             0
#define ERR_OUT_OF_MEMORY -1
#define ERR_BUFFER        -2
#define ERR_VALUE         -3
#define ERR_ARGUMENT      -4
#define ERR_MEMORY        -5
#define ERR_USE           -6
#define ERR_ROUTE         -7
#define ERR_ISCONN        -8

#define ERR_IS_FATAL(e) ((e) < ERR_ISCONN)

#define ERR_CLOSED        -9
#define ERR_NOTCONNECT    -10


#endif
