#include "rotation.h"
#include "bmpfuncs.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>

void rotate_seq(const float* input, float* output, const int width, const int height, char* _theta)
{
    int dest_x, dest_y; // 목적지 좌표

    // (x0, y0)은 회전의 중심좌표
    float x0 = width / 2.0f; //이미지의 너비의 절반
    float y0 = height / 2.0f; //이미지 높이의 절반

    // 회전각에 따라 sin, cos 계산하는 부분
    const float theta = atof(_theta) * M_PI / 180;
    const float sin_theta = sinf(theta);
    const float cos_theta = cosf(theta);

    // 새로 만드려는 image의 y좌표 0부터 height - 1까지 한줄씩 픽셀을 채워준다
    for (dest_y = 0; dest_y < height; dest_y++) {
        for (dest_x = 0; dest_x < width; dest_x++) {
            float xOff = dest_x - x0;
            float yOff = dest_y - y0;

            // (src_x, src_y)는 (dest_x, dest_y)가 theta만큼 회전하기 전의 좌표
            int src_x = (int)(xOff * cos_theta + yOff * sin_theta + x0);
            int src_y = (int)(yOff * cos_theta - xOff * sin_theta + y0);
            //printf("src:(%d, %d), dest(%d, %d)\n", src_x, src_y, dest_x, dest_y);
            if ((src_x >= 0) && (src_x < width) && (src_y >= 0) && (src_y < height))
                output[dest_y * width + dest_x] = input[src_y * width + src_x]; //예외발생
            else
                output[dest_y * width + dest_x] = 0.0f;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        printf("Usage: %s <src file> <dst file> <degree>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int width, height;
    float* input = readImage(argv[1], &width, &height);
    float* output_seq = (float*)malloc(sizeof(float) * width * height);
    float* output_par = (float*)malloc(sizeof(float) * width * height);

    clock_t start = clock();
    rotate(input, output_par, width, height, argv[3]);
    printf("Execution par time: %lfsec\n", (float)(clock() - start) / CLOCKS_PER_SEC);

    start = clock();
    rotate_seq(input, output_seq, width, height, argv[3]);
    printf("Execution seq time: %lfsec\n", (float)(clock() - start) / CLOCKS_PER_SEC);
    storeImage(output_par, "output_par.bmp", height, width, "output1.bmp");
    storeImage(output_seq, "output_seq.bmp", height, width, "output1.bmp");
    
    free(output_seq);
    free(output_par);
    free(input);

    return 0;
}