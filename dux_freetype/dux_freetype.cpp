#include "ft2build.h"
#include FT_FREETYPE_H

int main() {
    FT_Library library;
    if (FT_Init_FreeType(&library)) {
        printf("FreeType init failed!\n");
        return -1;
    }
    printf("FreeType loaded successfully!\n");
    FT_Done_FreeType(library);
    return 0;
}
