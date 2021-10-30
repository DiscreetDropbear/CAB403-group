#ifndef MACROSH
#define MACROS_H
#include "types.h"
#include "unistd.h"

#define DISPLAY_THREAD 1 

// sleep macro where duration is milliseconds 
// we will use this macro to scale the actual sleep time up
// for testing and debugging purposes
#define SLEEP(duration) usleep(duration * 1000 * SLEEP_SCALE) 
// This will help to make the sleep times longer for testing without
// having to change any of the sleep code within any of the code
#define SLEEP_SCALE 1 

// TODO: make sure macro definitions work for struct attribute accesses
// macros to access the lpr(license plate reader), boomgate and sign for the entrance
// entrance is the entrance number (1-5)
// shm is a (void*) to the shared memory

#define ENTRANCE_LPR(entrance, shm) ((struct lpr_t *) (shm + ENTRANCE_LPR_OFFSET(entrance)))
#define ENTRANCE_LPR_OFFSET(entrance) ((entrance-1) * 288)

#define ENTRANCE_BOOM(entrance, shm) ((struct boom_t *) (shm + ENTRANCE_BOOM_OFFSET(entrance))) 
#define ENTRANCE_BOOM_OFFSET(entrance) ((entrance-1) * 288) + 96

#define ENTRANCE_SIGN(entrance, shm) ((struct sign_t *) (shm + ENTRANCE_SIGN_OFFSET(entrance))) 
#define ENTRANCE_SIGN_OFFSET(entrance) ((entrance-1) * 288) + 192
#define ENTRANCE_SIGN_DISPLAY_OFFSET(entrance) (((entrance-1) * 288) + 280) 

// macros to access the lpr(license plate reader), boomgate for the exit 
// exit is the exit number (1-5) 
// shm is a (void*) to the shared memory
#define EXIT_LPR(exit_num, shm) ((struct lpr_t *) (shm + EXIT_LPR_OFFSET(exit_num))) 
#define EXIT_LPR_OFFSET(exit_num) 1440 + ((exit_num-1) * 192)

#define EXIT_BOOM(exit_num, shm) ((struct boom_t *) (shm + EXIT_BOOM_OFFSET(exit_num))) 
#define EXIT_BOOM_OFFSET(exit_num) 1440 + ((exit_num-1) * 192) + 96 

// macros to access the lpr(license plate reader), temperature and alarm for the specified exit 
// level is the level number (1-5)
// shm is a (void*) to the shared memory
#define LEVEL_LPR(level, shm) ((struct lpr_t *) (shm + LEVEL_LPR_OFFSET(level)))
#define LEVEL_LPR_OFFSET(level) 2400 + ((level-1) * 104)

#define LEVEL_TEMP(level, shm) ((short *) (shm + LEVEL_TEMP_OFFSET(level))) 
#define LEVEL_TEMP_OFFSET(level) 2400 + ((level-1) * 104) + 96

#define LEVEL_ALARM(level, shm) ((char *) (shm + LEVEL_ALARM_OFFSET(level))) 
#define LEVEL_ALARM_OFFSET(level) 2400 + ((level-1) * 104) + 98

#endif
