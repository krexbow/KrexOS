void kernel_main(void) {
    volatile char* video_memory = (volatile char*)0xB8000;
    
    // --- СТРОКА 1: Welcome to KrexOS v0.0.1! ---
    // Строка 6 экрана, самый левый край (Column = 0).
    // Формула: (6 * 80 + 0) * 2 = 960
    int offset1 = 1120; 
    char c1 = 0x0A; // Ярко-зелёный

    video_memory[offset1]      = 'W';  video_memory[offset1 + 1]  = c1;
    video_memory[offset1 + 2]  = 'e';  video_memory[offset1 + 3]  = c1;
    video_memory[offset1 + 4]  = 'l';  video_memory[offset1 + 5]  = c1;
    video_memory[offset1 + 6]  = 'c';  video_memory[offset1 + 7]  = c1;
    video_memory[offset1 + 8]  = 'o';  video_memory[offset1 + 9]  = c1;
    video_memory[offset1 + 10] = 'm';  video_memory[offset1 + 11] = c1;
    video_memory[offset1 + 12] = 'e';  video_memory[offset1 + 13] = c1;
    video_memory[offset1 + 14] = ' ';  video_memory[offset1 + 15] = c1;
    video_memory[offset1 + 16] = 't';  video_memory[offset1 + 17] = c1;
    video_memory[offset1 + 18] = 'o';  video_memory[offset1 + 19] = c1;
    video_memory[offset1 + 20] = ' ';  video_memory[offset1 + 21] = c1;
    video_memory[offset1 + 22] = 'K';  video_memory[offset1 + 23] = c1;
    video_memory[offset1 + 24] = 'r';  video_memory[offset1 + 25] = c1;
    video_memory[offset1 + 26] = 'e';  video_memory[offset1 + 27] = c1;
    video_memory[offset1 + 28] = 'x';  video_memory[offset1 + 29] = c1;
    video_memory[offset1 + 30] = 'O';  video_memory[offset1 + 31] = c1;
    video_memory[offset1 + 32] = 'S';  video_memory[offset1 + 33] = c1;
    video_memory[offset1 + 34] = ' ';  video_memory[offset1 + 35] = c1;
    video_memory[offset1 + 36] = 'v';  video_memory[offset1 + 37] = c1;
    video_memory[offset1 + 38] = '0';  video_memory[offset1 + 39] = c1;
    video_memory[offset1 + 40] = '.';  video_memory[offset1 + 41] = c1;
    video_memory[offset1 + 42] = '0';  video_memory[offset1 + 43] = c1;
    video_memory[offset1 + 44] = '.';  video_memory[offset1 + 45] = c1;
    video_memory[offset1 + 46] = '1';  video_memory[offset1 + 47] = c1;
    video_memory[offset1 + 48] = '!';  video_memory[offset1 + 49] = c1;

    // --- СТРОКА 2: Powered by Krexbow ---
    // Строка 7 экрана (прямо под первой), самый левый край (Column = 0).
    // Формула: (7 * 80 + 0) * 2 = 1120
    int offset2 = 1280; 
    char c2 = 0x0F; // Ярко-белый

    video_memory[offset2]      = 'P';  video_memory[offset2 + 1]  = c2;
    video_memory[offset2 + 2]  = 'o';  video_memory[offset2 + 3]  = c2;
    video_memory[offset2 + 4]  = 'w';  video_memory[offset2 + 5]  = c2;
    video_memory[offset2 + 6]  = 'e';  video_memory[offset2 + 7]  = c2;
    video_memory[offset2 + 8]  = 'r';  video_memory[offset2 + 9]  = c2;
    video_memory[offset2 + 10] = 'e';  video_memory[offset2 + 11] = c2;
    video_memory[offset2 + 12] = 'd';  video_memory[offset2 + 13] = c2;
    video_memory[offset2 + 14] = ' ';  video_memory[offset2 + 15] = c2;
    video_memory[offset2 + 16] = 'b';  video_memory[offset2 + 17] = c2;
    video_memory[offset2 + 18] = 'y';  video_memory[offset2 + 19] = c2;
    video_memory[offset2 + 20] = ' ';  video_memory[offset2 + 21] = c2;
    video_memory[offset2 + 22] = 'K';  video_memory[offset2 + 23] = c2;
    video_memory[offset2 + 24] = 'r';  video_memory[offset2 + 25] = c2;
    video_memory[offset2 + 26] = 'e';  video_memory[offset2 + 27] = c2;
    video_memory[offset2 + 28] = 'x';  video_memory[offset2 + 29] = c2;
    video_memory[offset2 + 30] = 'b';  video_memory[offset2 + 31] = c2;
    video_memory[offset2 + 32] = 'o';  video_memory[offset2 + 33] = c2;
    video_memory[offset2 + 34] = 'w';  video_memory[offset2 + 35] = c2;

    while (1) {
        
    }
}

