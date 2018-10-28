/////////////////////////////////////////////////////////////////////////////
//
// DSoundEngine
// DirectSound based sound playing engine
//
/////////////////////////////////////////////////////////////////////////////

#ifndef DSOUND_ENGINE_H_
#define DSOUND_ENGINE_H_

// Use oldest DirectSound interfaces available
#define DIRECTSOUND_VERSION 0

#pragma warning(disable : 4201)
#include <mmsystem.h>
#include <dsound.h>
#pragma warning(default : 4201)

class CDSound;

////////////////////////////////////////////////////////////////////////////////
// Declaration of the CDSoundEngine class
////////////////////////////////////////////////////////////////////////////////

class CDSoundEngine
{
public:
  ~CDSoundEngine();

  enum Status
  {
    STATUS_NOT_INIT,
    STATUS_READY,
    STATUS_SHUT
  };

  static CDSoundEngine& GetSoundEngine(void);
  Status GetStatus(void);
  WAVEFORMATEX& GetPrimaryFormat(void);
  LPDIRECTSOUND GetInterface(void);

  bool Initialize(void (*pThreadCallback)(void) = NULL);
  void Destroy(void);
  void StopThread(void);

  void AddToList(CDSound* pSound);
  void RemoveFromList(CDSound* pSound);
  void StopSounds(int iType);
  int CountSounds(int iType);

  static CSyncObject* GetSoundLock(void);

protected:
  CDSoundEngine();
  static UINT BackgroundThread(LPVOID);

  static CDSoundEngine SoundEngine;

  Status m_Status;
  WAVEFORMATEX m_Format;
  LPDIRECTSOUND m_IDSound;
  LPDIRECTSOUNDBUFFER m_IDBuffer;

  // Background thread
  HANDLE m_hEvent;
  CWinThread* m_pThread;
  void (*m_pThreadCallback)(void);

  // Array of playing sounds
  CCriticalSection m_SoundLock;
  CList<CDSound*,CDSound*> m_Sounds;
};

////////////////////////////////////////////////////////////////////////////////
// Declaration of the CDSound class
////////////////////////////////////////////////////////////////////////////////

class CDSound
{
public:
  CDSound();
  virtual ~CDSound();

  void SetBufferVolume(LONG Volume);
  int GetWriteSize(void);
  bool FillBuffer(int Bytes);
  void DestroyBuffer(void);

  virtual void WriteSampleData(unsigned char* pSample, int iSampleLen) = 0;
  virtual bool IsSoundOver(DWORD Tick) = 0;
  virtual int GetType(void) = 0;

protected:
  // This must be called from the most derived class' destructor
  void RemoveFromList(void);
  
  bool CreateBuffer(int Channels, int Frequency, int BitsPerSample);
  int GetBufferSize(void);
  bool PlayBuffer(bool PauseState);
  void Pause(bool PauseState);
  DWORD GetStatus(void);

  WAVEFORMATEX m_Format;
  LPDIRECTSOUNDBUFFER m_IDSBuffer;
  int m_WritePos;

  // Whether the sound is on the list of active sounds processed by the
  // background thread: this will be changed by the background thread.
  volatile bool m_Active;

  // Time at which playback started
  DWORD m_StartTime;
};

#endif // DSOUND_ENGINE_H_
