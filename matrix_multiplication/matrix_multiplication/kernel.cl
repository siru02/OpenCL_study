__kernel void matrix_multiplication(__global float* A, __global float* B, __global float* C, int ROW, int COL) {
    int x = get_global_id(0); // 현재 PE의 x좌표에 대한 global_id를 얻어온다
    int y = get_global_id(1); // 현재 PE의 y좌표에 대한 global_id를 얻어온다
    
    // C[x][y]를 계산하기 위해서
    // A의 x행 원소들을 전부 가져오고
    // B의 y열 원소들을 전부 가져온다

    int i, j, k;
    for (i = 0; i < ROW; i++) {
        for (j = 0; j < COL; j++) {
            for (k = 0; k < COL; k++) {
                C[i * COL + j] += A[i * COL + k] * B[k * COL + j];
            }
        }
    }
}