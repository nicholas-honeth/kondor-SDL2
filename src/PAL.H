// PAL.H - game palette (reconstructed, PAL.H lines 4-28)

// Colours 128-255 are 75% darker copies of 0-127 (used for shadows).
void makeshadowpal()
{
    for (int i = 0; i < 128; i++) {
        gamepal[384 + i * 3] = ftol(gamepal[i * 3] * 0.75);
        gamepal[384 + i * 3 + 1] = ftol(gamepal[i * 3 + 1] * 0.75);
        gamepal[384 + i * 3 + 2] = ftol(gamepal[i * 3 + 2] * 0.75);
    }
}

int load_game_pal()
{
    FILE *fil = fopen_ci("kondor.pal", "rb");
    if (fil == NULL) filerr();
    fread_z(gamepal, 1, 768, fil);
    fclose(fil);
    makeshadowpal();
    return 1;
}

void set_game_pal()
{
    for (int i = 0; i < 256; i++) setpal(i, gamepal[i * 3], gamepal[i * 3 + 1], gamepal[i * 3 + 2]);
}
