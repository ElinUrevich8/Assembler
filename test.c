#include <stdio.h>
#include <string.h>

int main() {
    FILE *f = fopen("filesamples/makro1.as", "r");
    char line[81];
    int line_num = 0;
    if (!f) {
        printf("Error: Failed to open file\n");
        return 1;
    }
    while (fgets(line, 81, f)) {
        line_num++;
        printf("Line %d: '%s' (length: %zu)\n", line_num, line, strlen(line));
    }
    if (feof(f)) printf("EOF reached\n");
    if (ferror(f)) printf("File read error\n");
    fclose(f);
    return 0;
}