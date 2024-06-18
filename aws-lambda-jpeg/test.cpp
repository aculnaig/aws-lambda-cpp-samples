#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#include <ImageMagick-7/MagickWand/MagickWand.h>

int main(int argc, char **argv)
{
    if (argc != 2) 
        exit(EXIT_FAILURE);

    const char *filename = argv[1];    
    FILE *file = fopen(filename, "rb");

    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    rewind(file);
    
    uint8_t *buffer = (uint8_t *) malloc(filesize);

    fread(buffer, filesize, 1, file);

    MagickBooleanType Status;
    ExceptionType Exception;
    char *WandError;
    MagickWand *Wand = NewMagickWand();
    
    Exception = MagickGetExceptionType(Wand);
    if (Exception != UndefinedException) {
        WandError = MagickGetException(Wand, &Exception);
        fprintf(stderr, "NewMagickWand(): %s\n", WandError); 
        MagickRelinquishMemory(WandError);
        exit(EXIT_FAILURE); 
    }

    Status = MagickReadImageBlob(Wand, buffer, filesize);
    if (Status == MagickFalse) {
        WandError = MagickGetException(Wand, &Exception);
        fprintf(stderr, "MagickReadImageBlob(): %s\n", WandError); 
        MagickRelinquishMemory(WandError);
        exit(EXIT_FAILURE); 
    }

    size_t NProperties;
    char **Exif = MagickGetImageProperties(Wand, "*:*", &NProperties);
    Exception = MagickGetExceptionType(Wand);
    if (Exception != UndefinedException) {
        WandError = MagickGetException(Wand, &Exception);
        fprintf(stderr, "MagickGetImageProperties(): %s\n", WandError); 
        MagickRelinquishMemory(WandError);
        exit(EXIT_FAILURE); 
    }

    for (int i = 0; Exif[i] != NULL; i++) {
        char *Property = MagickGetImageProperty(Wand, Exif[i]);
        Exception = MagickGetExceptionType(Wand);
        if (Exception != UndefinedException) {
            WandError = MagickGetException(Wand, &Exception);
            fprintf(stderr, "MagickGetImageProperty(): %s\n", WandError);
            MagickRelinquishMemory(WandError);
            MagickRelinquishMemory(Property);
            break;
        }
        printf("%s: %s\n", Exif[i], Property);
        MagickRelinquishMemory(Property);
    }

    free(buffer);
    fclose(file);
    MagickRelinquishMemory(Exif);
    DestroyMagickWand(Wand); 

    exit(EXIT_SUCCESS);
}
