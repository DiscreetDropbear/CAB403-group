int main(){

    char ** values;
    
    get_regos_from_file("file.txt", &values)
}

int select_valid_rego(map * outside, char** rego){
    
    int num = rand() %(100000 + 1);
    pair_t pair;

    int ret = get_nth_item(outside, num, &pair);
    
    // failure
    if(ret != 0){
        return ret; 
    }
    
    *rego = pair.key;

    // success
    return 0;
}

int generate_rego(char ** rego){ 

    *rego = malloc(7);
    if(*rego == NULL){
        return -1; 
    }

	for (int l = 0; l < 3; l++) {
        (*rego)[l] = rand() %(90 - 65 + 1) + 65;
    }

    // Generate three random numbers
	for (int i = 3; i < 6; i++) {
        (*rego)[i] = rand() %(57 - 48 + 1) + 48;
    }
    (*rego)[6] = '\0';
    
    return 0;
}

char * get_next_rego(Map * inside, Map * outside){
    
}

int get_regos(char * filename, (char**) * regos) 
    // find number of lines in the file
    size_t n; 

    // allocate array of char pointers on the heap
    char ** values = malloc(sizeof(char*) * n); 
    assert(values != NULL);

    // allocate char array on heap for each rego
    for(int i = 0; i<n; i++){
        values[i] = malloc(sizeof(char) * 7);
        assert(values[i] != NULL); 
    }

    // read each rego from the file into the character array
}  
