
void * spawner(void * arg){
}

void * entrance_queue(void * arg){
    // get the thread number (1-5)
    int tn = *(int*)arg;
}

void * exit_thr(void * arg){
    // get the thread number (1-5)
    int tn = *(int*)arg;
}

void * car(void * arg){
    char * rego = (char*)arg;
}


