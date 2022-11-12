#include <windows.h>
#include <xaudio2.h>

#include <stb_vorbis.c>

typedef void WINAPI co_uninitialize_fn(void);
typedef HRESULT WINAPI co_initialize_ex_fn(LPVOID, DWORD);
typedef HRESULT WINAPI xaudio2_create_fn(
		IXAudio2 **, UINT32, XAUDIO2_PROCESSOR);

static const WAVEFORMATEX g_wave_fmt = {
    .wFormatTag = WAVE_FORMAT_PCM, 
    .nChannels = 1,
    .nSamplesPerSec = 44100,
    .nAvgBytesPerSec = 44100,
    .nBlockAlign = 2,
    .wBitsPerSample = 16
};

HMODULE g_com_lib;
co_uninitialize_fn *g_co_uninitialize;

HMODULE g_xaudio2_lib;
IXAudio2 *g_xaudio2;
IXAudio2MasteringVoice *g_master;
IXAudio2SourceVoice *g_source;

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

int init_xaudio2(void)
{
	static const char *const g_paths[] = {
		"XAudio2_9.dll",
		"XAudio2_8.dll",
		"XAudio2_7.dll",
		NULL
	};

	static const char *const g_names[] = {
		"XAudio2Create",
		NULL
	};

	FARPROC proc;
	xaudio2_create_fn *xaudio2_create;
	HRESULT hr;

	if (start_com() < 0) {
		return -1;
	}

	g_xaudio2_lib = load_procs_ver(g_paths, g_names, &proc);
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
			XAUDIO2_DEFAULT_FREQ_RATIO, NULL, NULL, NULL);
	if (FAILED(hr)) {
		goto release_xaudio2;
	}

	return 0;
release_xaudio2:
	g_xaudio2->Release();
free_lib:
	FreeLibrary(g_xaudio2_lib);
	g_xaudio2_lib = NULL;
end_com:
	end_com();
	return -1;
}

void end_xaudio2(void)
{
	if (g_xaudio2_lib) { 
		g_xaudio2->Release();
		FreeLibrary(g_xaudio2_lib);
		g_xaudio2_lib = NULL;
		end_com();
	}
}
