#include <cstring>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <libspotify/api.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>
#include <map>
#include <vector>
#include <sstream>
#include <fstream>
#include <faac.h>
#include <signal.h>
#include <dirent.h>

using namespace std;

// Exit code
int g_ExitCode = 0;

// For logging
ofstream g_LogStream("sp_playlist_export.log");
#define LOG(log) g_LogStream << log << endl << flush
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
sp_session *g_Session;
const uint8_t g_AppKey[] = {
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
const size_t g_AppKeySize = sizeof(g_AppKey);

// For spotify client
pthread_mutex_t g_NotifyMutex;
pthread_cond_t g_NotifyCond;
bool g_NotifyDo = false;
bool g_PlaybackFinished = false;

// Spotify structures
sp_session_config g_SpConfig;
sp_session_callbacks g_SpSessionCallbacks;
sp_playlistcontainer_callbacks g_SpPlaylistContainerCallbacks;
sp_playlist_callbacks g_SpPlaylistCallbacks;

// Aac encoder handle
faacEncHandle g_FaacEncHdl;
// Encoded data buffer
vector<unsigned char> g_EncodedDataBuffer;
unsigned long g_SamplesInput;
unsigned long g_MaxBytesOutput;
ofstream g_AacFileStream;

// Searched playlist
string g_PlaylistName;
// Playlist found
bool g_PlaylistFound = false;
// Playlist
sp_playlist* g_Playlist = NULL;
// Playlist tracks list
map<string, sp_track*> g_TrackList;
// Playlist tracks iterator
map<string, sp_track*>::iterator g_TrackIter;
// Output directory
string g_OutputDir;
// Output file path
string g_OutputFilePath;
// Delete output file
bool g_DeleteOutputFile = false;

void ListDir(string i_Directory, vector<string>& o_FileList)
{
    DIR* l_Dir;
    struct dirent* l_Dirent;
    if((l_Dir = opendir(i_Directory.c_str())) == NULL) {
        LOG_ERR("Error: Unable to open directory " << i_Directory);
        exit(-1);
    }

    while((l_Dirent = readdir(l_Dir)) != NULL) {
        if(l_Dirent->d_name[0] != '.')
        {
            stringstream l_FilePath;
            l_FilePath << i_Directory << l_Dirent->d_name;
            o_FileList.push_back(l_FilePath.str());
        }
    }
    closedir(l_Dir);
}

void ReplaceAll(string& io_String, string i_Search, string i_Replace)
{
    int l_Position = 0;
    int l_Length = i_Search.length();
    for(;;)
    {
        l_Position = io_String.find(i_Search, l_Position);
        if(l_Position == string::npos)
            break;
        io_String.replace(l_Position, l_Length, i_Replace);
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
    stringstream l_OutputFilePathStr;
    l_OutputFilePathStr << g_OutputDir << "/" << g_PlaylistName << "/" << i_Filename << ".aac";
    return l_OutputFilePathStr.str();
}

void PlayTrack()
{
    LOG_FUNCTION("PlayTrack");
    for(;;)
    {
        if(g_TrackIter == g_TrackList.end())
        {
            sp_session_logout(g_Session);
            return;
        }
        g_OutputFilePath = g_TrackIter->first;
        LOG("Looking for file " << g_OutputFilePath);
        ifstream l_File(g_OutputFilePath.c_str());
        if(!l_File && sp_track_get_availability(
                g_Session,
                g_TrackIter->second) == SP_TRACK_AVAILABILITY_AVAILABLE)
            break;
        LOG("File " << g_OutputFilePath << " already exists");
        LOG("Skipping " << g_OutputFilePath);
        ++g_TrackIter;
    }
    g_FaacEncHdl = NULL;
    LOG_OUT("Writing " << g_OutputFilePath);
    sp_session_player_load(g_Session, g_TrackIter->second);
    sp_session_player_play(g_Session, true);
}

void TrackEnded()
{
    LOG_FUNCTION("TrackEnded");
    sp_session_player_play(g_Session, false);
    sp_session_player_unload(g_Session);

    g_AacFileStream.close();
    faacEncClose(g_FaacEncHdl);

    if(g_DeleteOutputFile)
        remove(g_OutputFilePath.c_str());

    g_ExitCode = 1;
    sp_session_logout(g_Session);
}

void CleanDestDir()
{
    vector<string> l_FileList;
    stringstream l_OutputDirPathStr;
    l_OutputDirPathStr << g_OutputDir << "/" << g_PlaylistName << "/";
    ListDir(l_OutputDirPathStr.str(), l_FileList);
    vector<string>::iterator l_FileListIter = l_FileList.begin();
    for(;l_FileListIter != l_FileList.end(); ++l_FileListIter)
    {
        if(g_TrackList.find(*l_FileListIter) == g_TrackList.end())
        {
            LOG_OUT("Removing " << *l_FileListIter);
            remove(l_FileListIter->c_str());
        }
    }
}

void LoadPlaylist()
{
    LOG_FUNCTION("LoadPlaylist");
    int l_TracksCount = sp_playlist_num_tracks(g_Playlist);
    LOG("Nb tracks: " << l_TracksCount);
    int l_TrackIndex = 0;
    for(; l_TrackIndex<l_TracksCount; ++l_TrackIndex)
    {
        sp_track* l_Track = sp_playlist_track(g_Playlist, l_TrackIndex);
        stringstream l_TrackNameStr;
        int l_ArtistsCount = sp_track_num_artists(l_Track);
        int l_ArtistIndex = 0;
        for(; l_ArtistIndex<l_ArtistsCount; ++l_ArtistIndex)
        {
            if(l_ArtistIndex != 0)
            {
                l_TrackNameStr << ", ";
            }
            l_TrackNameStr << sp_artist_name(sp_track_artist(l_Track, l_ArtistIndex));
        }
        l_TrackNameStr << " - " << sp_track_name(l_Track);
        if(l_TrackNameStr.str() == " - ")
        {
            LOG_ERR("Error: Unable to load playlist");
            exit(-1);
        }
        string l_FilePath = GetFilePath(l_TrackNameStr.str());
        g_TrackList[l_FilePath] = l_Track;
    }
    CleanDestDir();
    g_TrackIter = g_TrackList.begin();
    PlayTrack();
}

void SpPlaylistStateChangedCb(sp_playlist *i_Playlist, void *i_UserData)
{
    LOG_FUNCTION("SpPlaylistStateChangedCb");
    LOG(sp_playlist_name(i_Playlist));
    sp_playlist_set_in_ram(g_Session, i_Playlist, true);
    sp_playlist_set_autolink_tracks(i_Playlist, true);

    pthread_mutex_lock(&g_NotifyMutex);
    if(!g_Playlist && sp_playlist_is_loaded(i_Playlist) && g_PlaylistName == sp_playlist_name(i_Playlist))
    {
        g_Playlist = i_Playlist;
        g_PlaylistFound = true;
    }
    pthread_cond_signal(&g_NotifyCond);
    pthread_mutex_unlock(&g_NotifyMutex);
}

void SpPlaylistContainerLoadedCb(sp_playlistcontainer *i_PlaylistContainer, void *i_UserData)
{
    LOG_FUNCTION("SpPlaylistContainerLoadedCb");
    int l_PlaylistsCount = sp_playlistcontainer_num_playlists(i_PlaylistContainer);

    int l_PlaylistIndex = 0;
    for (; l_PlaylistIndex<l_PlaylistsCount; ++l_PlaylistIndex)
    {
        sp_playlist_set_in_ram(g_Session, sp_playlistcontainer_playlist(i_PlaylistContainer, l_PlaylistIndex), true);
        sp_playlist_add_callbacks(
                sp_playlistcontainer_playlist(i_PlaylistContainer, l_PlaylistIndex),
                &g_SpPlaylistCallbacks,
                (void*)(size_t)l_PlaylistIndex);
    }
}

int SpMusicDeliveryCb(sp_session *session, const sp_audioformat *format, const void *frames, int num_frames)
{
    if(g_FaacEncHdl == NULL)
    {
        g_FaacEncHdl = faacEncOpen( format->sample_rate, format->channels, &g_SamplesInput, &g_MaxBytesOutput );
        g_EncodedDataBuffer.resize ( g_MaxBytesOutput );

        faacEncConfigurationPtr l_AacEncoderCfg;
        l_AacEncoderCfg = faacEncGetCurrentConfiguration ( g_FaacEncHdl );
        l_AacEncoderCfg->aacObjectType = LOW;
        l_AacEncoderCfg->mpegVersion = MPEG4;
        //myFormat->useLfe = 0;
        //myFormat->useTns = 0;
        //l_AacEncoderCfg->allowMidside = 0;
        //l_AacEncoderCfg->bitRate = 128000;
        //l_AacEncoderCfg->bandWidth = -1;
        l_AacEncoderCfg->quantqual = 120;
        l_AacEncoderCfg->outputFormat = 1;
        l_AacEncoderCfg->inputFormat = FAAC_INPUT_16BIT;

        if ( !faacEncSetConfiguration (g_FaacEncHdl, l_AacEncoderCfg) ) {
            LOG_ERR("Error: Unsupported output format");
            exit(-1);
        }

        g_AacFileStream.open(g_OutputFilePath.c_str(), ios_base::out | ios_base::binary);
        if(!g_AacFileStream)
        {
            LOG_ERR("Error: Unable to write file " << g_OutputFilePath.c_str());
            exit(-1);
        }
    }

    int l_SamplesCount = num_frames * format->channels;
    int l_BytesWritten = 0;
    int l_SamplesRead = 0;
    for(; l_SamplesRead<l_SamplesCount; l_SamplesRead += g_SamplesInput)
    {
        l_BytesWritten = faacEncEncode ( g_FaacEncHdl, (int32_t *)((int16_t *)frames+l_SamplesRead), g_SamplesInput, &g_EncodedDataBuffer[0], g_MaxBytesOutput );
        g_AacFileStream.write((char *)&g_EncodedDataBuffer[0], l_BytesWritten);
    }

    return num_frames;
}

void SpEndOfTrackCb(sp_session *session)
{
    LOG_FUNCTION("SpEndOfTrackCb");
    LOG("Encoding finished");
    pthread_mutex_lock(&g_NotifyMutex);
    g_PlaybackFinished = true;
    pthread_cond_signal(&g_NotifyCond);
    pthread_mutex_unlock(&g_NotifyMutex);
}

void Interrupt(int i_Signal)
{
    LOG_FUNCTION("Interrupt");
    pthread_mutex_lock(&g_NotifyMutex);
    g_PlaybackFinished = true;
    g_DeleteOutputFile = true;
    pthread_cond_signal(&g_NotifyCond);
    pthread_mutex_unlock(&g_NotifyMutex);
}

void SpLoggedInCb(sp_session *session, sp_error error)
{
    LOG_FUNCTION("SpLoggedInCb");
    if (SP_ERROR_OK != error) {
        LOG_ERR("Error: login failed: " << sp_error_message(error));
        exit(-1);
    }
    signal(SIGINT, Interrupt);
    sp_session_set_connection_rules(session, SP_CONNECTION_RULE_NETWORK);
    sp_session_set_connection_type(session, SP_CONNECTION_TYPE_WIFI );
    sp_session_preferred_bitrate(session, SP_BITRATE_320k);
    sp_session_set_volume_normalization(session, true);

    sp_user* user = sp_session_user(session) ;
    LOG("User logged: " << sp_user_display_name(user));

    LOG("Looking for playlist " << g_PlaylistName);
    sp_playlistcontainer* pc = sp_session_playlistcontainer(session);
    sp_playlistcontainer_add_callbacks(
        pc,
        &g_SpPlaylistContainerCallbacks,
        NULL);
}

void SpLoggedOutCb(sp_session *session)
{
    LOG_FUNCTION("SpLoggedOutCb");
    sp_session_release(session);
    g_LogStream.close();
    exit(g_ExitCode);
}

void SpConnectionErrorCb(sp_session *session, sp_error error)
{
    LOG_FUNCTION("SpConnectionErrorCb");
    LOG_ERR("Error: " << sp_error_message(error));
    exit(-1);
}

void SpLogMessageCb(sp_session *session, const char *data)
{
    LOG_FUNCTION("SpLogMessageCb");
    LOG(data);
}

void SpNotifyMainThreadCb(sp_session *session)
{
    pthread_mutex_lock(&g_NotifyMutex);
    g_NotifyDo = true;
    pthread_cond_signal(&g_NotifyCond);
    pthread_mutex_unlock(&g_NotifyMutex);
}


void InitStructures()
{
    LOG_FUNCTION("InitGlobal");

    memset(&g_SpConfig, 0, sizeof(sp_session_config));
    g_SpConfig.api_version = SPOTIFY_API_VERSION;
    g_SpConfig.cache_location = "/home/benweet/.config/spotify";
    g_SpConfig.settings_location = "/home/benweet/.config/spotify";
    g_SpConfig.application_key = g_AppKey;
    g_SpConfig.application_key_size = g_AppKeySize;
    g_SpConfig.user_agent = USER_AGENT;
    g_SpConfig.callbacks = &g_SpSessionCallbacks;
    //g_SpConfig.dont_save_metadata_for_playlists = true;
    //g_SpConfig.initially_unload_playlists = true;
    //g_SpConfig.tracefile = "libspotify.log";

    memset(&g_SpSessionCallbacks, 0, sizeof(sp_session_callbacks));
    g_SpSessionCallbacks.logged_in = &SpLoggedInCb;
    g_SpSessionCallbacks.logged_out = &SpLoggedOutCb;
    g_SpSessionCallbacks.connection_error = &SpConnectionErrorCb;
    g_SpSessionCallbacks.notify_main_thread = &SpNotifyMainThreadCb;
    g_SpSessionCallbacks.music_delivery = &SpMusicDeliveryCb;
    g_SpSessionCallbacks.log_message = &SpLogMessageCb;
    g_SpSessionCallbacks.end_of_track = &SpEndOfTrackCb;

    memset(&g_SpPlaylistContainerCallbacks, 0, sizeof(sp_playlistcontainer_callbacks));
    g_SpPlaylistContainerCallbacks.container_loaded = &SpPlaylistContainerLoadedCb;

    memset(&g_SpPlaylistCallbacks, 0, sizeof(sp_playlist_callbacks));
    g_SpPlaylistCallbacks.playlist_state_changed = &SpPlaylistStateChangedCb;
}

void MainLoop()
{
    LOG_FUNCTION("MainLoop");

    pthread_mutex_lock(&g_NotifyMutex);

    int l_NextTimeout = 0;
    for (;;) {
        if (l_NextTimeout == 0) {
            while(!g_NotifyDo && !g_PlaybackFinished && !g_PlaylistFound)
                pthread_cond_wait(&g_NotifyCond, &g_NotifyMutex);
        } else {
            struct timespec ts;

#if _POSIX_TIMERS > 0
            clock_gettime(CLOCK_REALTIME, &ts);
#else
            struct timeval tv;
            gettimeofday(&tv, NULL);
            TIMEVAL_TO_TIMESPEC(&tv, &ts);
#endif
            ts.tv_sec += l_NextTimeout / 1000;
            ts.tv_nsec += (l_NextTimeout % 1000) * 1000000;

            pthread_cond_timedwait(&g_NotifyCond, &g_NotifyMutex, &ts);
        }

        g_NotifyDo = false;
        pthread_mutex_unlock(&g_NotifyMutex);

        if(g_PlaybackFinished)
        {
            TrackEnded();
            g_PlaybackFinished = false;
        }

        if(g_PlaylistFound)
        {
            LoadPlaylist();
            g_PlaylistFound = false;
        }

        do {
            sp_session_process_events(g_Session, &l_NextTimeout);
        } while (l_NextTimeout == 0);

        pthread_mutex_lock(&g_NotifyMutex);
    }
}


int main(int argc, char* argv[])
{
    if(argc != 5)
    {
        LOG_ERR("usage: sp_playlist_export <user> <password> <outdir> <playlist>");
        exit(-1);
    }
    g_PlaylistName = argv[4];
    g_OutputDir = argv[3];

    InitStructures();

    sp_error l_SpError = sp_session_create(&g_SpConfig, &g_Session);
    if (SP_ERROR_OK != l_SpError) {
        LOG_ERR("Error: failed to create session: " << sp_error_message(l_SpError));
        exit(-1);
    }

    pthread_mutex_init(&g_NotifyMutex, NULL);
    pthread_cond_init(&g_NotifyCond, NULL);

    LOG("Calling sp_session_login");
    sp_session_login(g_Session, argv[1], argv[2], false, NULL);

    MainLoop();
    return 0;
}
