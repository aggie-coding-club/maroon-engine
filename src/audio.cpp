#include <windows.h>
#include <xaudio2.h>

#include <stb_vorbis.c>

#include "audio.hpp"

/**
 * @MUS_INVALID: Their is no music in queue
 * @MUS_NONE: No music playing
 */
#define MUS_INVALID -2
#define MUS_NONE -1

#define NUM_SBUFS 8
#define SBUF_LEN 2400 

typedef void WINAPI co_uninitialize_fn(void);
typedef HRESULT WINAPI co_initialize_ex_fn(LPVOID, DWORD);
typedef HRESULT WINAPI xaudio2_create_fn(
		IXAudio2 **, UINT32, XAUDIO2_PROCESSOR);
typedef short sbuf[SBUF_LEN];

struct voice_cb : IXAudio2VoiceCallback {
	void OnStreamEnd(void) override; 
	void OnVoiceProcessingPassEnd(void) override;
	void OnVoiceProcessingPassStart(UINT32 min_samples) override;
	void OnBufferEnd(void *buf_ctx) override;
	void OnBufferStart(void *buf_ctx) override;
	void OnLoopEnd(void *buf_ctx) override;
	void OnVoiceError(void *buf_ctx, HRESULT err) override;
}; 

static const WAVEFORMATEX g_wave_fmt = {
	.wFormatTag = WAVE_FORMAT_PCM, 
	.nChannels = 1,
	.nSamplesPerSec = 48000,
	.nAvgBytesPerSec = 96000,
	.nBlockAlign = 2,
	.wBitsPerSample = 16
};

static const char *const g_music_paths[COUNTOF_MUS] = {
	[MUS_SAPPHIRE_LAKE] = "sapphire-lake.ogg"
};

static HMODULE g_com_lib;
static co_uninitialize_fn *g_co_uninitialize;

static HMODULE g_xaudio2_lib;
static IXAudio2 *g_xaudio2;
static IXAudio2MasteringVoice *g_master;
static IXAudio2SourceVoice *g_source;

static HANDLE g_buf_end_ev;
static HANDLE g_stream_ev; 
static HANDLE g_stream_thrd;

/**
 * @g_mus_qi: The music queued
 */
static volatile long g_mus_qi = MUS_INVALID;

/**
 * voice_cb::OnStreamEnd() - Play when audio is done streaming 
 */
void voice_cb::OnStreamEnd(void) 
{
	SetEvent(g_buf_end_ev); 
}

void voice_cb::OnVoiceProcessingPassEnd(void) {}

void voice_cb::OnVoiceProcessingPassStart(UINT32 min_samples) {}

void voice_cb::OnBufferEnd(void *buf_ctx) {}

void voice_cb::OnBufferStart(void *buf_ctx) {}

void voice_cb::OnLoopEnd(void *buf_ctx) {}

void voice_cb::OnVoiceError(void *buf_ctx, HRESULT err) {}

/**
 * load_procs() - Load library with procedures 
 * @path: Path to library
 * @names: Names of procedures load, termianted with NULL
 * @procs: Buffer procs to load into
 *
 * Return: The loaded library, or NULL on failure
 */
static HMODULE load_procs(const char *path, 
		const char *const *names, FARPROC *procs)
{
	HMODULE lib;	
	FARPROC *proc;
	const char *const *name;

	lib = LoadLibraryA(path); 
	if (!lib) { 
		return NULL;
	}

	proc = procs;
	for (name = names; *name; name++) {
		*proc = GetProcAddress(lib, *name);
		if (!*proc) {
			FreeLibrary(lib);
			return NULL;
		}

		proc++;
	}

	return lib;
}

/**
 * load_procs_ver() - Attempt to one version of library with procedures 
 * @paths: Paths to attempt to load, in order give, termianted with NULL 
 * @procs: Buffer procs to load
 * @names: Names of procedures load, termianted with NULL
 *
 * Return: The loaded library, or NULL on failure
 */
static HMODULE load_procs_ver(const char *const *paths, 
		const char *const *names, FARPROC *procs) 
{
	const char *const *path;

	for (path = paths; *path; path++) {
		HMODULE lib;

		lib = load_procs(*path, names, procs); 
		if (lib) {
			return lib;
		}
	}
	return NULL;
}

static int start_com(void)
{
	static const char *const g_names[] = {
		"CoInitializeEx",
		"CoUninitialize",
		NULL
	};

	FARPROC procs[2];
	co_initialize_ex_fn *co_initialize_ex;
	HRESULT hr;

	g_com_lib = load_procs("ole32.dll", g_names, procs);
	if (!g_com_lib) {
		return -1;
	}

	co_initialize_ex = (co_initialize_ex_fn *) procs[0];

	hr = co_initialize_ex(NULL, COINIT_MULTITHREADED); 
	if (FAILED(hr)) {
		FreeLibrary(g_com_lib);
		return -1;
	}

	g_co_uninitialize = (co_uninitialize_fn *) procs[1];

	return 0;
}

static void end_com(void)
{
	if (g_com_lib) {
		g_co_uninitialize();
		g_co_uninitialize = NULL;
		FreeLibrary(g_com_lib);
		g_com_lib = NULL;
	}
}

void play_music(int mus_i)
{
	if (g_xaudio2_lib) {
		g_mus_qi = mus_i;
		SetEvent(g_stream_ev);
	}
}

void stop_music(void) 
{
	if (g_xaudio2_lib) {
		g_mus_qi = MUS_NONE;
	}
}

static void stop(void)
{
	g_source->Stop(0);
	g_source->FlushSourceBuffers();
}

static stb_vorbis *update_vorbis(stb_vorbis *vorb)
{
	long i;
	const char *path;
	char full_path[260];

	i = InterlockedExchange(&g_mus_qi, MUS_INVALID);
	if (i == MUS_INVALID) {
		return vorb;
	}

	stb_vorbis_close(vorb);
	path = g_music_paths[i];
	sprintf(full_path, "res/music/%s", path);
	if (i == MUS_NONE) {
		stop();
		return NULL;
	} 
	vorb = stb_vorbis_open_filename(full_path, NULL, NULL);
	if (!vorb) {
		fprintf(stderr, "vorbis: Failed to open %s\n", path);
	} 
	return vorb;
}

static void update_audio(void)
{
	sbuf *bufs;
	stb_vorbis *vorb;
	int i;
	XAUDIO2_VOICE_STATE vs;

	bufs = (sbuf *) malloc(NUM_SBUFS * sizeof(*bufs)); 
	if (!bufs) {
		fprintf(stderr, "music: OOM\n");
		return;
	}

	vorb = NULL;
	i = 0;
	while (1) {
		short *buf;
		int count;
		XAUDIO2_BUFFER xbuf;
		HRESULT hr;

		vorb = update_vorbis(vorb);
		if (!vorb) {
			break;
		}

		buf = bufs[i];
		count = stb_vorbis_get_samples_short(vorb, 1, &buf, SBUF_LEN);
		if (count <= 0) {
			break;
		}

		memset(&xbuf, 0, sizeof(xbuf));
		xbuf.Flags = XAUDIO2_END_OF_STREAM; 
		xbuf.pAudioData = (BYTE *) (bufs + i);
		xbuf.AudioBytes = count * 2; 

		hr = g_source->SubmitSourceBuffer(&xbuf);
		if (FAILED(hr)) {
			fprintf(stderr, "music: SubmitSourceBuffer failed\n");
			break;
		}

		hr = g_source->Start(0);
		if (FAILED(hr)) {
			fprintf(stderr, "music: Start failed\n");
			break;
		}

		i++;
		if (i >= NUM_SBUFS) {
			i = 0;
		}

		while (1) {
			if (g_mus_qi == MUS_NONE) {
				stop();
				break;
			} 
			g_source->GetState(&vs);
			if (vs.BuffersQueued < NUM_SBUFS - 1) {
				break;
			}
			WaitForSingleObject(g_buf_end_ev, INFINITE);
		}
	}

	stb_vorbis_close(vorb);
	do {
		if (g_mus_qi != MUS_INVALID) {
			stop();
		}
		g_source->GetState(&vs);
	} while (vs.BuffersQueued > 0);
	free(bufs);
}

static DWORD stream_proc(LPVOID ctx)
{
	UNREFERENCED_PARAMETER(ctx);

	while (WaitForSingleObject(g_stream_ev, INFINITE) == WAIT_OBJECT_0) {
		update_audio();
	}

	return 0;
}

int init_xaudio2(void)
{
	static const char *const paths[] = {
		"XAudio2_9.dll",
		"XAudio2_8.dll",
		"XAudio2_7.dll",
		NULL
	};

	static const char *const names[] = {
		"XAudio2Create",
		NULL
	};

	static voice_cb vcb;

	FARPROC proc;
	xaudio2_create_fn *xaudio2_create;
	HRESULT hr;

	if (start_com() < 0) {
		return -1;
	}

	g_xaudio2_lib = load_procs_ver(paths, names, &proc);
	if (!g_xaudio2_lib) {
		goto end_com;
	}

	xaudio2_create = (xaudio2_create_fn *) proc;

	hr = xaudio2_create(&g_xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	if (FAILED(hr)) {
		goto free_lib;
	}

    	hr = g_xaudio2->CreateMasteringVoice(&g_master, XAUDIO2_DEFAULT_CHANNELS, 
			XAUDIO2_DEFAULT_SAMPLERATE, 0, NULL, NULL, 
			AudioCategory_GameEffects);
	if (FAILED(hr)) {
		/*all voices are destroyed by releasing g_xaudio2*/
		goto release_xaudio2;
	}

    	hr = g_xaudio2->CreateSourceVoice(&g_source, &g_wave_fmt, 0, 
			XAUDIO2_DEFAULT_FREQ_RATIO, &vcb, NULL, NULL);
	if (FAILED(hr)) {
		goto release_xaudio2;
	}

	g_buf_end_ev = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (!g_buf_end_ev) {
		goto release_xaudio2;
	}

	g_stream_ev = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (!g_stream_ev) {
		goto close_buf_end_ev;
	}

	g_stream_thrd = CreateThread(NULL, 0, stream_proc, NULL, 0, 0);
	if (!g_stream_thrd) {
		goto close_stream_ev;
	}	

	return 0;
close_stream_ev:
	CloseHandle(g_stream_ev);
close_buf_end_ev:
	CloseHandle(g_buf_end_ev);
release_xaudio2:
	g_xaudio2->Release();
free_lib:
	FreeLibrary(g_xaudio2_lib);
	g_xaudio2_lib = NULL;
end_com:
	end_com();
	fprintf(stderr, "xaudio2: Failed to initialize\n");
	return -1;
}

void end_xaudio2(void)
{
	if (g_xaudio2_lib) { 
		CloseHandle(g_stream_thrd);
		CloseHandle(g_stream_ev);
		CloseHandle(g_buf_end_ev);
		g_xaudio2->Release();
		FreeLibrary(g_xaudio2_lib);
		g_xaudio2_lib = NULL;
		end_com();
	}
}
