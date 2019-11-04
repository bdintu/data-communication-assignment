#include <iostream>

#define HEIGHT 320
#define WHDTH 240

using namespace std;

//unsigned med = 0xEFFFFF; /* 00000000 11000000 11000000 11000000 */
unsigned q0 = 7000;
unsigned q1 = 17000;
unsigned q2 = 27000;

FILE* readNextBMP(FILE* fp, unsigned id) {
    char path[20];
    sprintf(path,"C:\\out\\%u", id);
    strcat(path, ".bmp");
    fp = fopen(path, "rb");
    //cout << "PC2 : ---- path:" << path << ", fp:" << fp << endl;
    return fp;
}

FILE* waitNextBMP(FILE* fp, unsigned id) {
    printf("PC2 : start find next id %d\n", id);
    FILE *fp_superman;
    do {
        printf("PC2 : wait 2s for id %d\n", id);
        Sleep(2000);
        fp_superman = readNextBMP(fp_superman, id);
    }
    while (fp_superman == NULL);
    fp = fp_superman;
    fclose(fp_superman);
    return fp;
}

/*
unsigned countBlackWhite(FILE *fp, unsigned id) {
    unsigned black = 0;
    unsigned rgb_int;
    unsigned char rgb_char[3];

    fseek(fp, 374*sizeof(unsigned char), SEEK_SET);

    for (unsigned h=0; h<HEIGHT; ++h) {
        for (unsigned w=0; w<WHDTH; ++w) {
            fread(rgb_char, 3*sizeof(unsigned char), 1, fp);
            rgb_int = 0;
            memcpy(&rgb_int, rgb_char,3);
            if (rgb_int < med) {
                ++black;
            }
        }
    }
    cout << "PC2 : num:" << id << ", black:" << black << endl;
    return black;
}
*/

unsigned separation(unsigned black) {
    if (black < q0) {
        //cout << "PC2 : very white" << endl;
        return 5;
    }

    if (black < q1) {
        //cout << "PC2 : small" << endl;
        return 0;
    }

    if (black < q2) {
        //cout << "PC2 : medium" << endl;
        return 1;
    }

    //cout << "PC2 : large" << endl;
    return 2;
}

unsigned countBlack(unsigned id) {
    char path[20];
    sprintf(path, "C:\\out\\%u.bmp", id);
    bitmap_image image(path);

   if (!image)
   {
      printf("Error - Failed to open: 1.bmp\n");
      return 1;
   }

   unsigned int black = 0;
   const unsigned int height = image.height();
   const unsigned int width  = image.width();

   rgb_t colour3, colour4, colour7, colour8;
   image.get_pixel(160, 120, colour3);
   image.get_pixel(161, 120, colour4);
   if (abs(colour3.red -colour4.red ) > 32)  {
            printf("---------------- pic bit moseg, ");
            printf("%d, %d\n", abs(colour3.red -colour4.red ), abs(colour3.green -colour4.green));
        return 0;
   }

   for (std::size_t y = 0; y < height; ++y)
   {
      for (std::size_t x = 0; x < width; ++x)
      {
         rgb_t colour;
         image.get_pixel(x, y, colour);
         if (colour.red     < 96 &&
             colour.blue    < 96 &&
             colour.green   < 96 )
            ++black;
      }
   }

   return black;
}
