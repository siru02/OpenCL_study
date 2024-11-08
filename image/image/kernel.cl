__kernel void rotate_image(__global float* buffer_src, __global float* buffer_dest, 
    const float sin_theta, const float cos_theta, 
    const float x0, const float y0, const int width, const int height) 
{
    // 현재 연산중인 PE는 global_size범위중 한 요소에 대해서 연산하므로 구하고자하는 좌표의 x, y 좌표를 얻어온다
    int dest_x = get_global_id(0); // 현재 PE의 x좌표에 대한 global_id를 얻어온다
    int dest_y = get_global_id(1); // 현재 PE의 y좌표에 대한 global_id를 얻어온다

    // output[x][y] = input[src_x][src_y]이다
    float xOff = dest_x - x0;
    float yOff = dest_y - y0;

    // (src_x, src_y)는 (dest_x, dest_y)가 theta만큼 회전하기 전의 좌표
    int src_x = (int)(xOff * cos_theta + yOff * sin_theta + x0);
    int src_y = (int)(yOff * cos_theta - xOff * sin_theta + y0);
    if ((src_x >= 0) && (src_x < width) && (src_y >= 0) && (src_y < height))
        buffer_dest[dest_y * width + dest_x] = buffer_src[src_y * width + src_x];
    else
        buffer_dest[dest_y * width + dest_x] = 0.0f;
}