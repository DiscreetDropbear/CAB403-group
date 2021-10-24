int get_regos(char * filename, char** * regos)
{
    //have to find number of lines in file
    size_t n; 

    char ** values = malloc(sizeof(char*) * n); 
    assert(values != NULL);

    for(int i = 0; i<n; i++){
        values[i] = malloc(sizeof(char) * 7);
        assert(values[i] != NULL); 
    }

    int i,j;
    int value[100][7];
    FILE *archivo;
    archivo = fopen("plate.txt","r");
    if (archivo == NULL)
        exit(1);
    i=0;
    while (feof(archivo) == 0)
    {
        fscanf( archivo, "%c %c %c %c %c %c\n", &value[i][0],&value[i][1],&value[i][2],&value[i][3],&value[i][4],&value[i][5]);
        printf("%c %c %c %c %c %c\n", value[i][0], value[i][1], value[i][2], value[i][3], value[i][4], value[i][5]);
        i++;
    }

    fclose(archivo);
    return 0;
}