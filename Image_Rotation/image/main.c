#include "rotation.h"
#include "bmpfuncs.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>

void rotate_seq(const float* input, float* output, const int width, const int height, char* _theta)
{
    int dest_x, dest_y; // ������ ��ǥ

    // (x0, y0)�� ȸ���� �߽���ǥ
    float x0 = width / 2.0f; //�̹����� �ʺ��� ����
    float y0 = height / 2.0f; //�̹��� ������ ����

    // ȸ������ ���� sin, cos ����ϴ� �κ�
    const float theta = atof(_theta) * M_PI / 180;
    const float sin_theta = sinf(theta);
    const float cos_theta = cosf(theta);

    // ���� ������� image�� y��ǥ 0���� height - 1���� ���پ� �ȼ��� ä���ش�
    for (dest_y = 0; dest_y < height; dest_y++) {
        for (dest_x = 0; dest_x < width; dest_x++) {
            float xOff = dest_x - x0;
            float yOff = dest_y - y0;

            // (src_x, src_y)�� (dest_x, dest_y)�� theta��ŭ ȸ���ϱ� ���� ��ǥ
            int src_x = (int)(xOff * cos_theta + yOff * sin_theta + x0);
            int src_y = (int)(yOff * cos_theta - xOff * sin_theta + y0);
            //printf("src:(%d, %d), dest(%d, %d)\n", src_x, src_y, dest_x, dest_y);
            if ((src_x >= 0) && (src_x < width) && (src_y >= 0) && (src_y < height))
                output[dest_y * width + dest_x] = input[src_y * width + src_x]; //���ܹ߻�
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