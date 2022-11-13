#define MUS_SAPPHIRE_LAKE 0
#define COUNTOF_MUS 1

/**
 * init_xaudio2() - Init sound engine
 *
 * Retrun: Return zero and success and negative on failure
 */
int init_xaudio2(void);

/**
 * end_xaudio2() - End xaudio2 engine
 */
void end_xaudio2(void);

void play_music(int mus_i);
void stop_music(void);
