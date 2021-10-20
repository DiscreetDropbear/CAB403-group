int main()
{
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

    for(j = 0; j < i; j++)
        printf("%c %c %c %c %c %c\n", value[j][0], value[j][1], value[j][2], value[j][3], value[j][4], value[j][5]);

    fclose(archivo);
    return 0;
}