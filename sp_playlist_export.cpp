// sp_playlist_export.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#ifdef  _WIN32
	#include <windows.h>
	#include <sys/types.h>
	#include <io.h>
	#include <fcntl.h>

int gettimeofday(struct timeval* p, void* tz) {
    ULARGE_INTEGER ul; // As specified on MSDN.
    FILETIME ft;

    // Returns a 64-bit value representing the number of
    // 100-nanosecond intervals since January 1, 1601 (UTC).
    GetSystemTimeAsFileTime(&ft);

    // Fill ULARGE_INTEGER low and high parts.
    ul.LowPart = ft.dwLowDateTime;
    ul.HighPart = ft.dwHighDateTime;
    // Convert to microseconds.
    ul.QuadPart /= 10ULL;
    // Remove Windows to UNIX Epoch delta.
    ul.QuadPart -= 11644473600000000ULL;
    // Modulo to retrieve the microseconds.
    p->tv_usec = (long) (ul.QuadPart % 1000000LL);
    // Divide to retrieve the seconds.
    p->tv_sec = (long) (ul.QuadPart / 1000000LL);

    return 0;
}

# define TIMEVAlTO_TIMESPEC(tv, ts) {                                   \
        (ts)->tv_sec = (tv)->tv_sec;                                    \
        (ts)->tv_nsec = (tv)->tv_usec * 1000;                           \
}

#else
	#include <sys/time.h>
#endif

#include <pthread.h>

#include <dirent.h>
#include <cstring>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <libspotify/api.h>

#include <stdint.h>
#include <stdlib.h>
#include <map>
#include <set>
#include <vector>
#include <sstream>
#include <fstream>
#include <signal.h>

#ifndef WIN32
#endif

using namespace std;

// Exit code
int gExitCode = 0;

// For logging
ofstream gLogStream("sp_playlist_export.log");
#define LOG(log) gLogStream << log << endl << flush
#define LOG_FUNCTION(log) FunctionLog tmp_FuncLog(log)
#define LOG_OUT(log) LOG(log);\
	cout << log << endl << flush
#define LOG_ERR(log) LOG(log);\
	cerr << log << endl << flush

class FunctionLog {
public:
	FunctionLog(string i_FunctionName) {
		m_FunctionName = i_FunctionName;
		LOG(">>> " << i_FunctionName);
	}
	~FunctionLog() {
		LOG("<<< " << m_FunctionName);
	}
private:
	string m_FunctionName;
};

#define USER_AGENT "benweet"

// For libspotify usage
sp_session *gSession = (sp_session *)1;
const uint8_t gAppKey[] = {
	0x01, 0x72, 0x1C, 0xE3, 0x79, 0xB5, 0xF8, 0xA8, 0x8D, 0x17, 0x1D, 0x2F, 0x54, 0x7B, 0xFF, 0x25,
	0xE4, 0x56, 0x1E, 0xCD, 0x1E, 0x5A, 0xA0, 0x70, 0xE8, 0x54, 0xCC, 0xC0, 0x76, 0xF9, 0x82, 0x70,
	0xA4, 0xBF, 0x2C, 0x9C, 0xEA, 0xA9, 0x5D, 0x72, 0x60, 0x58, 0x5E, 0x86, 0x15, 0x85, 0x69, 0x58,
	0x68, 0x89, 0xAB, 0xF3, 0x18, 0x2C, 0xC9, 0x69, 0xEE, 0x8D, 0x36, 0x02, 0x92, 0x65, 0xF0, 0xF6,
	0x03, 0xB2, 0x09, 0x30, 0x7E, 0xE9, 0xA8, 0x1C, 0x87, 0x04, 0xB8, 0x50, 0x2B, 0xC5, 0x5A, 0x10,
	0x25, 0xF8, 0xA5, 0x63, 0x82, 0xB4, 0x70, 0xFC, 0x2F, 0x08, 0x80, 0x06, 0x31, 0xB3, 0x3A, 0xD7,
	0x54, 0xA8, 0x78, 0xCF, 0x9C, 0x51, 0xCA, 0x66, 0xE6, 0x5D, 0x9F, 0xC1, 0xD1, 0xB0, 0xA1, 0xCA,
	0x01, 0x80, 0xA3, 0xF5, 0x8A, 0x6D, 0x99, 0x6B, 0x85, 0xEB, 0xF5, 0xEC, 0x27, 0x2C, 0x7B, 0xC5,
	0xEA, 0x0E, 0x7E, 0xF3, 0x9A, 0x9D, 0x1D, 0x1A, 0x0C, 0x87, 0x0B, 0x9A, 0xB7, 0x85, 0x97, 0x1F,
	0x66, 0x87, 0x69, 0x4F, 0x27, 0xE0, 0x42, 0x80, 0xB9, 0xF5, 0x6E, 0xC5, 0x4A, 0xB2, 0xEC, 0xBF,
	0x29, 0x73, 0xBC, 0x51, 0xB0, 0x6B, 0x8B, 0x22, 0x24, 0xDB, 0x80, 0xFF, 0x82, 0xE8, 0x35, 0x5A,
	0xB2, 0xCB, 0x83, 0x7A, 0x4C, 0x94, 0x4A, 0x3F, 0x35, 0xB1, 0x8C, 0x4C, 0x9F, 0xF2, 0xD5, 0x10,
	0xDA, 0x22, 0xA9, 0x8D, 0x82, 0xD7, 0xEC, 0xB3, 0xC6, 0xA4, 0xB1, 0x49, 0x98, 0xFB, 0xEE, 0x09,
	0xC0, 0xCB, 0x37, 0xC2, 0xB9, 0xE6, 0x1F, 0xC3, 0x7A, 0x43, 0x06, 0x65, 0x75, 0xAD, 0xF3, 0x3B,
	0x46, 0x3E, 0xE6, 0x08, 0x2E, 0x5B, 0x67, 0xF5, 0x03, 0x51, 0xD8, 0x3B, 0x01, 0xDA, 0x65, 0x32,
	0x6F, 0x09, 0xA1, 0xBF, 0x57, 0x85, 0x4E, 0x75, 0x8F, 0xA5, 0x39, 0x94, 0xD8, 0xE2, 0x06, 0x86,
	0x41, 0xD3, 0xBC, 0x3C, 0xF5, 0xA0, 0xCA, 0x7F, 0x4C, 0xDA, 0x3C, 0xDE, 0xBF, 0x5D, 0xD2, 0xB4,
	0x55, 0x74, 0xDE, 0x72, 0x37, 0xB3, 0xC5, 0x3E, 0x27, 0x59, 0x35, 0x00, 0x5E, 0x1F, 0x00, 0x5E,
	0x3F, 0x13, 0xCC, 0x55, 0x9C, 0x3D, 0xB8, 0x61, 0x7D, 0x0C, 0xDC, 0x73, 0xF6, 0x38, 0x89, 0x87,
	0x6B, 0x63, 0xA7, 0x43, 0x46, 0x9A, 0x24, 0x45, 0x72, 0x9F, 0xBD, 0x7E, 0xFA, 0x98, 0x22, 0x0A,
	0x44,
};
const size_t gAppKeySize = sizeof(gAppKey);
const char* gDeviceId = "1234";

// For spotify client
pthread_mutex_t gNotifyMutex;
pthread_cond_t gNotifyCond;
bool gNotifyDo = false;
bool gPlaybackFinished = false;

// Spotify structures
sp_session_config gSpConfig;
sp_session_callbacks gSpSessionCallbacks;
sp_playlistcontainer_callbacks gSpPlaylistContainerCallbacks;
sp_playlist_callbacks gSpPlaylistCallbacks;

// Searched playlist
string gPlaylistName;
// Playlist found
int gPlaylistFound = 0;
// Playlist
sp_playlist* gPlaylist = NULL;
// Playlist tracks list
map<string, sp_track*> gTrackList;
// Playlist tracks iterator
map<string, sp_track*>::iterator gTrackIter;
// Existing file list
set<string> gExistFileList;
// Output directory
string gOutputDir;
// Output file path
string gOutputFilePath;
// Delete output file
bool gDeleteOutputFile = false;
// Output file stream
// ofstream gOutputFileStream;
// Write the header?
#define WRITE_STATE_HEADER 0
#define WRITE_STATE_WAIT 1
#define WRITE_STATE_STARTED 2
int gWriteState = WRITE_STATE_HEADER;

string ToAnsi(const char* utf8) {
	int length = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)utf8, -1, NULL, 0);
    if (length == 0)
		return "";

	wchar_t* wide = new wchar_t[length];
    MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)utf8, -1, wide, length);

	// convert it to ANSI, use setlocale() to set your locale, if not set
    size_t convertedChars = 0;
    char* ansi = new char[length+1];
    wcstombs_s(&convertedChars, ansi, length, wide, _TRUNCATE);
	string result = ansi;
	delete wide;
	delete ansi;
	return result;
}

void ListDir(string i_Directory, set<string>& o_FileList)
{
	DIR* lDir;
	struct dirent* lDirent;
	if((lDir = opendir(i_Directory.c_str())) == NULL) {
		LOG_ERR("Error: Unable to open directory " << i_Directory);
		exit(-1);
	}

	while((lDirent = readdir(lDir)) != NULL) {
		if(lDirent->d_name[0] != '.')
		{
			stringstream lFilePathStream;
			lFilePathStream << i_Directory << lDirent->d_name;
			string lFilePath = lFilePathStream.str();
			size_t lPointPos = lFilePath.rfind('.');
			o_FileList.insert(lFilePath.substr(0, lPointPos));
		}
	}
	closedir(lDir);
}

void ReplaceAll(string& io_String, string i_Search, string i_Replace)
{
	int lPosition = 0;
	int lLength = i_Search.length();
	for(;;)
	{
		lPosition = io_String.find(i_Search, lPosition);
		if(lPosition == string::npos)
			break;
		io_String.replace(lPosition, lLength, i_Replace);
	}
}

string GetFilePath(string i_Filename)
{
	LOG_FUNCTION("FillFilePath");
	// Create file name
	LOG("File name: " << i_Filename);
	ReplaceAll(i_Filename, ";", ", ");
	ReplaceAll(i_Filename, "/", ", ");
	ReplaceAll(i_Filename, ":", " - ");
	ReplaceAll(i_Filename, "?", "");
	ReplaceAll(i_Filename, "*", ".");
	ReplaceAll(i_Filename, "\"", "'");
	ReplaceAll(i_Filename, "  ", " ");
	LOG("Formatted name: " << i_Filename);
	// Create file path
	stringstream lOutputFilePathStr;
#ifdef _WIN32
	lOutputFilePathStr << gOutputDir << "\\" << gPlaylistName << "\\" << i_Filename;
#else
	lOutputFilePathStr << gOutputDir << "/" << gPlaylistName << "/" << i_Filename;
#endif
	return lOutputFilePathStr.str();
}

void PlayTrack()
{
	LOG_FUNCTION("PlayTrack");
	for(;gTrackIter != gTrackList.end(); ++gTrackIter)
	{
		gOutputFilePath = gTrackIter->first;
		LOG("Looking for file " << gOutputFilePath);
		if(gExistFileList.find(gOutputFilePath) == gExistFileList.end()) {
			if(sp_track_get_availability(
				gSession,
				gTrackIter->second) == SP_TRACK_AVAILABILITY_AVAILABLE) {
				LOG_OUT("_ENCODE_ " << gOutputFilePath);
				sp_session_player_load(gSession, gTrackIter->second);
				sp_session_player_play(gSession, true);
				return;
			}
			else {
				LOG("File " << gOutputFilePath << " is not available");
				LOG_OUT("_COPY_   " << gOutputFilePath);
			}
		}
		else {
			LOG("File " << gOutputFilePath << " already exists");
			LOG("Skipping " << gOutputFilePath);
		}
	}
	sp_session_logout(gSession);
}

void TrackEnded()
{
	LOG_FUNCTION("TrackEnded");
	sp_session_player_play(gSession, false);
	sp_session_player_unload(gSession);

	//gOutputFileStream.close();

	if(gDeleteOutputFile)
		remove(gOutputFilePath.c_str());

	gExitCode = 1;
	sp_session_logout(gSession);
}

void CleanDestDir()
{
	stringstream lOutputDirPathStr;
#ifdef _WIN32
	lOutputDirPathStr << gOutputDir << "\\" << gPlaylistName << "\\";
#else
	lOutputDirPathStr << gOutputDir << "/" << gPlaylistName << "/";
#endif
	ListDir(lOutputDirPathStr.str(), gExistFileList);
	set<string>::iterator lFileListIter = gExistFileList.begin();
	for(;lFileListIter != gExistFileList.end(); ++lFileListIter)
	{
		if(gTrackList.find(*lFileListIter) == gTrackList.end())
		{
			LOG_OUT("_REMOVE_ " << *lFileListIter);
		}
	}
}

void LoadPlaylist()
{
	LOG_FUNCTION("LoadPlaylist");
	int lTracksCount = sp_playlist_num_tracks(gPlaylist);
	LOG("Nb tracks: " << lTracksCount);
	gTrackList.clear();
	int lTrackIndex = 0;
	for(; lTrackIndex<lTracksCount; ++lTrackIndex)
	{
		sp_track* lTrack = sp_playlist_track(gPlaylist, lTrackIndex);
		stringstream lTrackNameStr;
		int lArtistsCount = sp_track_num_artists(lTrack);
		int lArtistIndex = 0;
		for(; lArtistIndex<lArtistsCount; ++lArtistIndex)
		{
			if(lArtistIndex != 0)
			{
				lTrackNameStr << ", ";
			}
			lTrackNameStr << ToAnsi(sp_artist_name(sp_track_artist(lTrack, lArtistIndex)));
		}
		lTrackNameStr << " - " << ToAnsi(sp_track_name(lTrack));

		string lTrackName = lTrackNameStr.str();
		if(lTrackName == " - ")
		{
			gPlaylistFound--;
			if(gPlaylistFound == 0) {
				LOG_ERR("Error: Unable to load playlist");
				exit(-1);
			}
			return;
		}
		string lFilePath = GetFilePath(lTrackName);
		gTrackList[lFilePath] = lTrack;
	}
	CleanDestDir();
	gTrackIter = gTrackList.begin();
	PlayTrack();
	gPlaylistFound = 0;
}

void CALLBACK SpPlaylistStateChangedCb(sp_playlist *i_Playlist, void *i_UserData)
{
	LOG_FUNCTION("SpPlaylistStateChangedCb");
	LOG(sp_playlist_name(i_Playlist));
	sp_playlist_set_in_ram(gSession, i_Playlist, true);
	sp_playlist_set_autolink_tracks(i_Playlist, true);

	pthread_mutex_lock(&gNotifyMutex);
	if(!gPlaylist && sp_playlist_is_loaded(i_Playlist) && gPlaylistName == sp_playlist_name(i_Playlist))
	{
		gPlaylist = i_Playlist;
		gPlaylistFound = 100;
	}
	pthread_cond_signal(&gNotifyCond);
	pthread_mutex_unlock(&gNotifyMutex);
}

void CALLBACK SpPlaylistContainerLoadedCb(sp_playlistcontainer *i_PlaylistContainer, void *i_UserData)
{
	LOG_FUNCTION("SpPlaylistContainerLoadedCb");
	int lPlaylistsCount = sp_playlistcontainer_num_playlists(i_PlaylistContainer);

	int lPlaylistIndex = 0;
	for (; lPlaylistIndex<lPlaylistsCount; ++lPlaylistIndex)
	{
		sp_playlist_set_in_ram(gSession, sp_playlistcontainer_playlist(i_PlaylistContainer, lPlaylistIndex), true);
		sp_playlist_add_callbacks(
				sp_playlistcontainer_playlist(i_PlaylistContainer, lPlaylistIndex),
				&gSpPlaylistCallbacks,
				(void*)(size_t)lPlaylistIndex);
	}
}

void write(int16_t value) {
	cout.write((char*)&value, sizeof(int16_t));
}

void write(int32_t value) {
	cout.write((char*)&value, sizeof(int32_t));
}

int CALLBACK SpMusicDeliveryCb(sp_session *session, const sp_audioformat *format, const void *frames, int num_frames)
{
	/*
	if(gFaacEncHdl == NULL)
	{
		gFaacEncHdl = faacEncOpen( format->sample_rate, format->channels, &gSamplesInput, &gMaxBytesOutput );
		gEncodedDataBuffer.resize ( gMaxBytesOutput );

		faacEncConfigurationPtr lAacEncoderCfg;
		lAacEncoderCfg = faacEncGetCurrentConfiguration ( gFaacEncHdl );
		lAacEncoderCfg->aacObjectType = LOW;
		lAacEncoderCfg->mpegVersion = MPEG4;
		//myFormat->useLfe = 0;
		//myFormat->useTns = 0;
		//lAacEncoderCfg->allowMidside = 0;
		//lAacEncoderCfg->bitRate = 128000;
		//lAacEncoderCfg->bandWidth = -1;
		lAacEncoderCfg->quantqual = 120;
		lAacEncoderCfg->outputFormat = 1;
		lAacEncoderCfg->inputFormat = FAAC_INPUT_16BIT;

		if ( !faacEncSetConfiguration (gFaacEncHdl, lAacEncoderCfg) ) {
			LOG_ERR("Error: Unsupported output format");
			exit(-1);
		}

		gAacFileStream.open(gOutputFilePath.c_str(), ios_base::out | ios_base::binary);
		if(!gAacFileStream)
		{
			LOG_ERR("Error: Unable to write file " << gOutputFilePath.c_str());
			exit(-1);
		}
	}

	int lSamplesCount = num_frames * format->channels;
	int lBytesWritten = 0;
	int lSamplesRead = 0;
	for(; lSamplesRead<lSamplesCount; lSamplesRead += gSamplesInput)
	{
		lBytesWritten = faacEncEncode ( gFaacEncHdl, (int32_t *)((int16_t *)frames+lSamplesRead), gSamplesInput, &gEncodedDataBuffer[0], gMaxBytesOutput );
		gAacFileStream.write((char *)&gEncodedDataBuffer[0], lBytesWritten);
	}
	*/

	if(gWriteState == WRITE_STATE_HEADER)
	{
		cout.setf(ios_base::binary);

		const int bytes_per_sample = 2;
		const int byte_rate = format->sample_rate * format->channels * bytes_per_sample;

		// RIFF header
		cout.write("RIFF", 4);
		write(int32_t(32));
		cout.write("WAVE", 4);

		// WAVE fmt sub-chunk
		cout.write("fmt ", 4);
		write(int32_t(16));                                  // Subchunk1Size
		write(int16_t(1));                                   // AudioFormat
		write(int16_t(format->channels));                    // NumChannels
		write(int32_t(format->sample_rate));                 // SampleRate
		write(int32_t(byte_rate));                           // ByteRate
		write(int16_t(format->channels * bytes_per_sample)); // BlockAlign
		write(int16_t(bytes_per_sample * 8));                // BitsPerSample

		// Data sub-chunk
		cout.write("data", 4);
		write(int32_t(0));
		gWriteState = WRITE_STATE_WAIT;
	}

	uint32_t* buffer = (uint32_t*)frames;
	uint32_t* buffer_end = buffer + num_frames;

	while(buffer != buffer_end && *buffer == 0) {
		buffer++;
	}

	if(gWriteState == WRITE_STATE_WAIT) {
		if(buffer != buffer_end)
			gWriteState = WRITE_STATE_STARTED;
	}
	else if(gWriteState == WRITE_STATE_STARTED) {
		if(buffer != buffer_end)
			buffer = (uint32_t*)frames;
	}

	// Write the audio data.
	int buffer_len = buffer_end - buffer;
	if(buffer_len > 0)
		cout.write((const char*)buffer, buffer_len * sizeof(uint32_t));

	return num_frames;
}

void CALLBACK SpEndOfTrackCb(sp_session *session)
{
	LOG_FUNCTION("SpEndOfTrackCb");
	LOG("Encoding finished");
	pthread_mutex_lock(&gNotifyMutex);
	gPlaybackFinished = true;
	pthread_cond_signal(&gNotifyCond);
	pthread_mutex_unlock(&gNotifyMutex);
}

void Interrupt(int i_Signal)
{
	LOG_FUNCTION("Interrupt");
	pthread_mutex_lock(&gNotifyMutex);
	gPlaybackFinished = true;
	gDeleteOutputFile = true;
	pthread_cond_signal(&gNotifyCond);
	pthread_mutex_unlock(&gNotifyMutex);
}

void CALLBACK SpLoggedInCb(sp_session *session, sp_error error)
{
	LOG_FUNCTION("SpLoggedInCb");
	if (SP_ERROR_OK != error) {
		LOG_ERR("Error: login failed: " << sp_error_message(error));
		exit(-1);
	}
	signal(SIGINT, Interrupt);
	sp_session_set_connection_rules(session, SP_CONNECTION_RULE_NETWORK);
	sp_session_set_connection_type(session, SP_CONNECTION_TYPE_WIRED);
	sp_session_preferred_bitrate(session, SP_BITRATE_320k);
	sp_session_set_volume_normalization(session, true);

	sp_user* user = sp_session_user(session) ;
	LOG("User logged: " << sp_user_display_name(user));

	LOG("Looking for playlist " << gPlaylistName);
	sp_playlistcontainer* pc = sp_session_playlistcontainer(session);
	sp_playlistcontainer_add_callbacks(
		pc,
		&gSpPlaylistContainerCallbacks,
		NULL);
}

void CALLBACK SpLoggedOutCb(sp_session *session)
{
	LOG_FUNCTION("SpLoggedOutCb");
	sp_session_release(session);
	gLogStream.close();
	exit(gExitCode);
}

void CALLBACK SpConnectionErrorCb(sp_session *session, sp_error error)
{
	LOG_FUNCTION("SpConnectionErrorCb");
	LOG_ERR("Error: " << sp_error_message(error));
	exit(-1);
}

void CALLBACK SpLogMessageCb(sp_session *session, const char *data)
{
	LOG_FUNCTION("SpLogMessageCb");
	LOG(data);
}

void CALLBACK SpNotifyMainThreadCb(sp_session *session)
{
	pthread_mutex_lock(&gNotifyMutex);
	gNotifyDo = true;
	pthread_cond_signal(&gNotifyCond);
	pthread_mutex_unlock(&gNotifyMutex);
}

void CALLBACK SpMetadataUpdatedCb(sp_session *session)
{
	LOG_FUNCTION("SpMetadataUpdatedCb");
}

void CALLBACK SpMessageToUserCb(sp_session *session, const char *message)
{
	LOG_FUNCTION("SpMessageToUserCb");
	LOG(message);
}

void CALLBACK SpPlayTokenLostCb(sp_session *session)
{
	LOG_FUNCTION("SpPlayTokenLostCb");
}

void CALLBACK SpStreamingErrorCb(sp_session *session, sp_error error)
{
	LOG_FUNCTION("SpStreamingErrorCb");
	LOG_ERR("Error: " << sp_error_message(error));
	exit(-1);
}

void CALLBACK SpUserinfoUpdatedCb(sp_session *session)
{
	LOG_FUNCTION("SpUserinfoUpdatedCb");
}

void CALLBACK SpStartPlaybackCb(sp_session *session)
{
	LOG_FUNCTION("SpStartPlaybackCb");
}

void CALLBACK SpGetAudioBufferStatsCb(sp_session *session, sp_audio_buffer_stats *stats)
{
	LOG_FUNCTION("SpGetAudioBufferStatsCb");
}

void CALLBACK SpOfflineStatusUpdatedCb(sp_session *session)
{
	LOG_FUNCTION("SpOfflineStatusUpdatedCb");
}

void CALLBACK SpOfflineErrorCb(sp_session *session, sp_error error)
{
	LOG_FUNCTION("SpOfflineErrorCb");
	LOG_ERR("Error: " << sp_error_message(error));
	exit(-1);
}

void CALLBACK SpCredentialsBlobUpdatedCb(sp_session *session, const char *blob)
{
	LOG_FUNCTION("SpCredentialsBlobUpdatedCb");
}

void CALLBACK SpConnectionstateUpdatedCb(sp_session *session)
{
	LOG_FUNCTION("SpConnectionstateUpdatedCb");
}

void CALLBACK SpScrollableErrorCb(sp_session *session, sp_error error)
{
	LOG_FUNCTION("SpScrollableErrorCb");
	LOG_ERR("Error: " << sp_error_message(error));
	exit(-1);
}

void CALLBACK SpPrivateSessionModeChangedCb(sp_session *session, bool is_private)
{
	LOG_FUNCTION("SpPrivateSessionModeChangedCb");
}


void InitStructures()
{
	LOG_FUNCTION("InitGlobal");

	memset(&gSpConfig, 0, sizeof(sp_session_config));
	gSpConfig.api_version = SPOTIFY_API_VERSION;
#ifdef _WIN32
	gSpConfig.cache_location = "Z:\\Temp\\LibSpotify";
	gSpConfig.settings_location = "Z:\\Temp\\LibSpotify";
#else
	gSpConfig.cache_location = "/home/benweet/.config/spotify";
	gSpConfig.settings_location = "/home/benweet/.config/spotify";
#endif
	gSpConfig.application_key = gAppKey;
	gSpConfig.application_key_size = gAppKeySize;
	gSpConfig.user_agent = USER_AGENT;
	gSpConfig.callbacks = &gSpSessionCallbacks;
	//gSpConfig.dont_save_metadata_for_playlists = true;
	gSpConfig.initially_unload_playlists = true;
	gSpConfig.tracefile = "libspotify.log";
	gSpConfig.device_id = gDeviceId;

	memset(&gSpSessionCallbacks, 0, sizeof(sp_session_callbacks));
	gSpSessionCallbacks.logged_in = &SpLoggedInCb;
	gSpSessionCallbacks.logged_out = &SpLoggedOutCb;
	gSpSessionCallbacks.connection_error = &SpConnectionErrorCb;
	gSpSessionCallbacks.notify_main_thread = &SpNotifyMainThreadCb;
	gSpSessionCallbacks.music_delivery = &SpMusicDeliveryCb;
	gSpSessionCallbacks.log_message = &SpLogMessageCb;
	gSpSessionCallbacks.end_of_track = &SpEndOfTrackCb;
	gSpSessionCallbacks.stop_playback = &SpEndOfTrackCb;
	gSpSessionCallbacks.metadata_updated = &SpMetadataUpdatedCb;
	gSpSessionCallbacks.message_to_user = &SpMessageToUserCb;
	gSpSessionCallbacks.play_token_lost = &SpPlayTokenLostCb;
	gSpSessionCallbacks.streaming_error = &SpStreamingErrorCb;
	gSpSessionCallbacks.userinfo_updated = &SpUserinfoUpdatedCb;
	gSpSessionCallbacks.start_playback = &SpStartPlaybackCb;
	gSpSessionCallbacks.get_audio_buffer_stats = &SpGetAudioBufferStatsCb;
	gSpSessionCallbacks.offline_status_updated = &SpOfflineStatusUpdatedCb;
	gSpSessionCallbacks.offline_error = &SpOfflineErrorCb;
	gSpSessionCallbacks.credentials_blob_updated = &SpCredentialsBlobUpdatedCb;
	gSpSessionCallbacks.connectionstate_updated = &SpConnectionstateUpdatedCb;
	gSpSessionCallbacks.scrobble_error = &SpScrollableErrorCb;
	gSpSessionCallbacks.private_session_mode_changed = &SpPrivateSessionModeChangedCb;

	memset(&gSpPlaylistContainerCallbacks, 0, sizeof(sp_playlistcontainer_callbacks));
	gSpPlaylistContainerCallbacks.container_loaded = &SpPlaylistContainerLoadedCb;

	memset(&gSpPlaylistCallbacks, 0, sizeof(sp_playlist_callbacks));
	gSpPlaylistCallbacks.playlist_state_changed = &SpPlaylistStateChangedCb;

	pthread_mutex_init(&gNotifyMutex, NULL);
	pthread_cond_init(&gNotifyCond, NULL);
}

void MainLoop()
{
	LOG_FUNCTION("MainLoop");

	pthread_mutex_lock(&gNotifyMutex);

	int lNextTimeout = 0;
	for (;;) {
		if (lNextTimeout == 0) {
			while(!gNotifyDo && !gPlaybackFinished && !gPlaylistFound)
				pthread_cond_wait(&gNotifyCond, &gNotifyMutex);
		} else {
			struct timespec ts;

#if _POSIX_TIMERS > 0
			clock_gettime(CLOCK_REALTIME, &ts);
#else
			struct timeval tv;
			gettimeofday(&tv, NULL);
			TIMEVAlTO_TIMESPEC(&tv, &ts);
#endif
			ts.tv_sec += lNextTimeout / 1000;
			ts.tv_nsec += (lNextTimeout % 1000) * 1000000;

			pthread_cond_timedwait(&gNotifyCond, &gNotifyMutex, &ts);
		}

		gNotifyDo = false;
		pthread_mutex_unlock(&gNotifyMutex);

		if(gPlaybackFinished)
		{
			TrackEnded();
			gPlaybackFinished = false;
		}

		if(gPlaylistFound)
		{
			LoadPlaylist();
		}

		do {
			sp_session_process_events(gSession, &lNextTimeout);
		} while (lNextTimeout == 0);

		pthread_mutex_lock(&gNotifyMutex);
	}
}


int main(int argc, char* argv[])
{
	int result = _setmode( _fileno( stdout ), _O_BINARY );
	if( result == -1 ) {
		LOG_ERR( "Cannot set mode" );
	}
	else {
		LOG( "'stdout' successfully changed to binary mode" );
	}
	setlocale (LC_ALL,"");

	if(argc != 5)
	{
		LOG_ERR("usage: sp_playlist_export <user> <password> <outdir> <playlist>");
		exit(-1);
	}
	gPlaylistName = argv[4];
	gOutputDir = argv[3];

	InitStructures();

	sp_error lSpError = sp_session_create(&gSpConfig, &gSession);
	if (SP_ERROR_OK != lSpError) {
		LOG_ERR("Error: failed to create session: " << sp_error_message(lSpError));
		exit(-1);
	}

	LOG("Calling sp_session_login");
	sp_session_login(gSession, argv[1], argv[2], false, NULL);

	MainLoop();
	return 0;
}

