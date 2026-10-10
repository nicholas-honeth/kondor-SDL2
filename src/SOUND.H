// SOUND.H - Sound Blaster sound effects (reconstructed, SOUND.H lines 22-330)
//
// The original programs the SB DSP for 8-bit mono auto-init DMA at time constant 0xA5
// (1000000/(256-165) = ~10989 Hz) over a 32 KB ring, with an IRQ every 8 KB block.
// Effects are pre-converted to signed deltas and *added* into the ring at the current
// DMA position (no clipping, 8-bit wrap-around, exactly as the original). The IRQ
// handler resets the block that finished playing one interrupt earlier to silence.
//
// Port: the DMA controller, the DSP and the PIC are replaced by the platform audio
// device, which plays the same ring and calls ServiceIRQ() at the same block boundaries.
// The IRQ/DMA auto-detection (getsb/getdma/getirq and ServiceIRQ2..11) has nothing to
// probe any more and reduces to fixed values.

static const unsigned SB_RATE = 1000000 / (256 - 0xA5);
static const unsigned DMA_RING = 0x8000, DMA_BLOCK = 0x2000;

// Bytes already played inside the current 8 KB block (from the DMA count register).
unsigned long DMAInCurrentBlock()
{
    unsigned long DMACurrent = plat::audio_position();
    return DMACurrent & 0x1FFF;
}

int isplaying(WaveData *Voice)
{
    plat::audio_lock();
    unsigned long current = DMAInCurrentBlock() + bytes_played;
    plat::audio_unlock();
    return Voice->finishing > current ? 1 : 0;
}

static void ServiceIRQ();

void StartPlayBack()
{
    WriteDSP(0xD1);                  // speaker on
    WriteDSP(0x40); WriteDSP(0xA5);  // time constant
    WriteDSP(0x48); WriteDSP(0xFF); WriteDSP(0x1F);   // block size 0x2000
    WriteDSP(0x1C);                  // 8-bit auto-init DMA output
    if (!plat::audio_start(DMABuffer, DMA_RING, DMA_BLOCK, SB_RATE, ServiceIRQ)) soundblaster = 0;
}

void ClearBuffer(unsigned Buffer)
{
    memset(DMABuffer + (Buffer << 13), 0x80, 0x2000);
}

void MixVoice(WaveData *Voice)
{
    if (!soundblaster || !soundfx) return;
    plat::audio_lock();
    unsigned OffSet = plat::audio_position();
    for (unsigned long Counter = 0; Counter <= Voice->length; Counter++) {
        DMABuffer[OffSet] += Voice->data[Counter];
        OffSet = (OffSet + 1) & 0x7FFF;
    }
    Voice->finishing = bytes_played + DMAInCurrentBlock() + Voice->length;
    plat::audio_unlock();
}

void LoadVoice(WaveData *Voice, const char *FileName)
{
    FILE *WAVFile = fopen_ci(FileName, "rb");
    if (WAVFile == NULL) filerr();
    fseek(WAVFile, 0, SEEK_END);
    Voice->length = ftell(WAVFile) - 48;          // skip the 44-byte RIFF header (+2)
    Voice->data = (uchar *)malloc(Voice->length + 2);
    if (Voice->data == NULL) memerr("WAVS");
    fseek(WAVFile, 46, SEEK_SET);
    fread_z(Voice->data, Voice->length + 2, 1, WAVFile);
    fclose(WAVFile);
}

// Unsigned 8-bit sample -> signed delta (-32..31) stored as a byte: (s >> 2) + 0xE0.
void ConvertVoice(WaveData *Voice)
{
    for (unsigned long OffSet = 0; OffSet <= Voice->length; OffSet++) {
        Voice->data[OffSet] >>= 2;
        Voice->data[OffSet] += 0xE0;
    }
}

// Called by the platform audio thread (with the audio lock held) after every block.
static void ServiceIRQ()
{
    ClearBuffer(CBuffer);
    CBuffer = (CBuffer + 1) & 3;
    bytes_played += 0x2000;
}

void AssignBuffer()
{
    // The original allocated twice if the first block crossed a 64 KB DMA page.
    DMABuffer = (uchar *)malloc(0x8000);
    if (DMABuffer == NULL) memerr("DMA BUFFER");
    memset(DMABuffer, 0x80, 0x8000);   // original cleared 0x7FFF bytes; the last byte stayed garbage
}

uchar getdma() { return 1; }
void getirq() { IRQ = 5; }

void getsb()
{
    DMA = getdma();
    getirq();
}

void initsound()
{
    static const char *files[10] = {"kondor7.dat", "kondor8.dat", "kondor9.dat", "kondor10.dat",
                                    "kondor11.dat", "kondor12.dat", "kondor13.dat", "kondor14.dat",
                                    "kondor15.dat", "kondor16.dat"};
    for (int i = 0; i < 10; i++) {
        LoadVoice(&VoiceData[i], files[i]);
        ConvertVoice(&VoiceData[i]);
    }
    AssignBuffer();
    CBuffer = 3;
    StartPlayBack();
}

void closesound()
{
    plat::audio_stop();
    for (int i = 0; i < 10; i++) free(VoiceData[i].data);
    free(DMABuffer);
    closesb();
}
