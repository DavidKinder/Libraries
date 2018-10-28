/////////////////////////////////////////////////////////////////////////////
//
// DSoundEngine
// DirectSound based sound playing engine
//
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "DSoundEngine.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////////////////////////////////////////////////////////////
// Implementation of the CDSoundEngine class
////////////////////////////////////////////////////////////////////////////////

// The only instance of the sound engine
CDSoundEngine CDSoundEngine::SoundEngine;

// Get the only instance of the sound engine
CDSoundEngine& CDSoundEngine::GetSoundEngine(void)
{
  return SoundEngine;
}

// Get the status of the sound engine
CDSoundEngine::Status CDSoundEngine::GetStatus(void)
{
  return m_Status;
}

// Get the primary wave format
WAVEFORMATEX& CDSoundEngine::GetPrimaryFormat(void)
{
  return m_Format;
}

// Get the IDirectSound interface
LPDIRECTSOUND CDSoundEngine::GetInterface(void)
{
  return m_IDSound;
}

// Hidden sound engine constructor
CDSoundEngine::CDSoundEngine()
{
  m_Status = STATUS_NOT_INIT;
  m_hEvent = 0;
  m_pThread = NULL;
  m_IDSound = NULL;
  m_IDBuffer = NULL;

  // Set up the primary wave format
  ::ZeroMemory(&m_Format,sizeof(WAVEFORMATEX));
  m_Format.wFormatTag = WAVE_FORMAT_PCM;
  m_Format.nChannels = 2;
  m_Format.nSamplesPerSec = 44100;
  m_Format.wBitsPerSample = 16;
  m_Format.nBlockAlign = (WORD)(m_Format.nChannels * (m_Format.wBitsPerSample>>3));
  m_Format.nAvgBytesPerSec = m_Format.nSamplesPerSec * m_Format.nBlockAlign;
}

CDSoundEngine::~CDSoundEngine()
{
  Destroy();
}

// Initialize the DirectSound sound engine
bool CDSoundEngine::Initialize(void (*pThreadCallback)(void))
{
  if (m_Status != STATUS_NOT_INIT)
    return true;

  // Save the thread callback
  m_pThreadCallback = pThreadCallback;

  // Create an event for signalling the background thread
  m_hEvent = ::CreateEvent(NULL,FALSE,FALSE,NULL);
  if (m_hEvent != 0)
  {
    // Start the background thread
    m_pThread = AfxBeginThread(BackgroundThread,this);
    if (m_pThread != NULL)
    {
      // Create an IDirectSound interface
      if (SUCCEEDED(::DirectSoundCreate(NULL,&m_IDSound,NULL)))
      {
        // Set the DirectSound cooperation level
        HWND hMainWnd = AfxGetMainWnd()->GetSafeHwnd();
        if (SUCCEEDED(m_IDSound->SetCooperativeLevel(hMainWnd,DSSCL_PRIORITY)))
        {
          // Create the primary DirectSound buffer
          DSBUFFERDESC BufferDesc;
          ::ZeroMemory(&BufferDesc,sizeof(DSBUFFERDESC));
          BufferDesc.dwSize = sizeof(DSBUFFERDESC);
          BufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
          if (SUCCEEDED(m_IDSound->CreateSoundBuffer(&BufferDesc,&m_IDBuffer,NULL)))
          {
            // Set the wave format of the primary buffer
            if (SUCCEEDED(m_IDBuffer->SetFormat(&m_Format)))
            {
              // Start the primary buffer playing
              if (SUCCEEDED(m_IDBuffer->Play(0,0,DSBPLAY_LOOPING)))
                m_Status = STATUS_READY;
            }
          }
        }
      }
    }
  }

  if (m_Status == STATUS_READY)
    return true;
  Destroy();
  return false;
}

// Release the DirectSound interface
void CDSoundEngine::Destroy(void)
{
  StopThread();
  m_Status = STATUS_SHUT;

  if (m_IDBuffer != NULL)
  {
    m_IDBuffer->Stop();
    m_IDBuffer->Release();
  }
  m_IDBuffer = NULL;

  if (m_IDSound != NULL)
  {
    m_IDSound->Release();
    m_IDSound = NULL;
  }
}

// Stop the background thread
void CDSoundEngine::StopThread(void)
{
  if (m_pThread != NULL)
  {
    // Get the thread handle
    HANDLE hThread = m_pThread->m_hThread;

    // Signal the thread to exit
    ::SetEvent(m_hEvent);

    // Wait until the thread has exited
    ::WaitForSingleObject(hThread,INFINITE);
    m_pThread = NULL;
  }

  if (m_hEvent != 0)
  {
    ::CloseHandle(m_hEvent);
    m_hEvent = 0;
  }
}

// Add to the list of playing sounds
void CDSoundEngine::AddToList(CDSound* pSound)
{
  CSingleLock Lock(GetSoundLock(),TRUE);
  m_Sounds.AddTail(pSound);
}

// Remove this sound from the playing list
void CDSoundEngine::RemoveFromList(CDSound* pSound)
{
  CSingleLock Lock(GetSoundLock(),TRUE);
  POSITION Pos = m_Sounds.Find(pSound);
  if (Pos != NULL)
    m_Sounds.RemoveAt(Pos);
}

// Stop playing sounds of the given type
void CDSoundEngine::StopSounds(int iType)
{
  CSingleLock Lock(GetSoundLock(),TRUE);

  POSITION Pos = m_Sounds.GetHeadPosition();
  while (Pos != NULL)
  {
    CDSound* pSound = m_Sounds.GetNext(Pos);
    if (pSound->GetType() == iType)
      pSound->DestroyBuffer();
  }
}

// Count the number of playing sounds of the given type
int CDSoundEngine::CountSounds(int iType)
{
  CSingleLock Lock(GetSoundLock(),TRUE);

  int count = 0;
  POSITION Pos = m_Sounds.GetHeadPosition();
  while (Pos != NULL)
  {
    CDSound* pSound = m_Sounds.GetNext(Pos);
    if (pSound->GetType() == iType)
      count++;
  }
  return count;
}

CSyncObject* CDSoundEngine::GetSoundLock(void)
{
  return &SoundEngine.m_SoundLock;
}

// Background thread loop
UINT CDSoundEngine::BackgroundThread(LPVOID)
{
  while (true)
  {
    // Wait until signalled or 100ms have elapsed
    if (::WaitForSingleObject(SoundEngine.m_hEvent,100) == WAIT_OBJECT_0)
      return 0;

    {
      // Get the lock on the list of playing sounds
      CSingleLock Lock(GetSoundLock(),TRUE);

      // Update each sound buffer in turn
      DWORD Tick = ::GetTickCount();
      POSITION Pos = SoundEngine.m_Sounds.GetHeadPosition();
      while (Pos != NULL)
      {
        CDSound* pSound = SoundEngine.m_Sounds.GetNext(Pos);

        // Check if the sound has finished, otherwise write more sample data
        if (pSound->IsSoundOver(Tick))
          pSound->DestroyBuffer();
        else
          pSound->FillBuffer(pSound->GetWriteSize());
      }
    }

    // If a callback has been specified, call it (with no locks held)
    if (SoundEngine.m_pThreadCallback != NULL)
      (*(SoundEngine.m_pThreadCallback))();
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Implementation of the CDSound class
////////////////////////////////////////////////////////////////////////////////

CDSound::CDSound()
{
  ::ZeroMemory(&m_Format,sizeof(WAVEFORMATEX));
  m_IDSBuffer = NULL;

  m_WritePos = 0;
  m_Active = false;
  m_StartTime = 0;
}

CDSound::~CDSound()
{
  DestroyBuffer();
}

// Remove this sound from the playing list
void CDSound::RemoveFromList(void)
{
  CDSoundEngine::GetSoundEngine().RemoveFromList(this);
}

// Create the secondary DirectSound buffer for this sound
bool CDSound::CreateBuffer(int Channels, int Frequency, int BitsPerSample)
{
  if (m_IDSBuffer != NULL)
    return false;

  CDSoundEngine& Engine = CDSoundEngine::GetSoundEngine();
  if (Engine.GetStatus() != CDSoundEngine::STATUS_READY)
    return false;

  // Set up the wave format
  m_Format.wFormatTag = WAVE_FORMAT_PCM;
  m_Format.nChannels = (WORD)Channels;
  m_Format.nSamplesPerSec = Frequency;
  m_Format.wBitsPerSample = (WORD)BitsPerSample;
  m_Format.nBlockAlign = (WORD)(Channels * (BitsPerSample>>3));
  m_Format.nAvgBytesPerSec = Frequency * m_Format.nBlockAlign;

  // Create a secondary DirectSound buffer
  DSBUFFERDESC BufferDesc;
  ::ZeroMemory(&BufferDesc,sizeof(DSBUFFERDESC));
  BufferDesc.dwSize = sizeof(DSBUFFERDESC);
  BufferDesc.dwFlags =
    DSBCAPS_CTRLVOLUME|DSBCAPS_GETCURRENTPOSITION2|DSBCAPS_STICKYFOCUS;
  BufferDesc.dwBufferBytes = GetBufferSize();
  BufferDesc.lpwfxFormat = &m_Format;
  HRESULT Result = Engine.GetInterface()->CreateSoundBuffer(&BufferDesc,
    &m_IDSBuffer,NULL);
  if (FAILED(Result))
      m_IDSBuffer = NULL;

  return (m_IDSBuffer != NULL);
}

// Get the size of the buffer in bytes. Secondary buffers are always
// two seconds long.
int CDSound::GetBufferSize(void)
{
  return m_Format.nAvgBytesPerSec * 2;
}

// Get the maximum amount of data that can be written to the buffer
int CDSound::GetWriteSize(void)
{
  if (m_IDSBuffer == NULL)
    return 0;

  // Get the position of the play cursor
  DWORD PlayCursor, WriteCursor;
  if (FAILED(m_IDSBuffer->GetCurrentPosition(&PlayCursor,&WriteCursor)))
    return 0;

  // Writing point is ahead of play cursor
  if (m_WritePos > (int)PlayCursor)
    return GetBufferSize() - (m_WritePos - PlayCursor);

  // Writing point is behind play cursor
  return PlayCursor - m_WritePos;
}

// Fill a region of the buffer with the next sample
bool CDSound::FillBuffer(int Bytes)
{
  if (m_IDSBuffer == NULL)
    return false;

  BYTE* Buffer1 = NULL;
  DWORD Buffer1Len = 0;
  BYTE* Buffer2 = NULL;
  DWORD Buffer2Len = 0;

  // Lock the buffer
  HRESULT LockResult = m_IDSBuffer->Lock(m_WritePos,Bytes,
    (LPVOID*)&Buffer1,&Buffer1Len,(LPVOID*)&Buffer2,&Buffer2Len,0);

  if (FAILED(LockResult))
  {
    // If needed, attempt to restore the buffer
    if (LockResult == DSERR_BUFFERLOST)
    {
      if (FAILED(m_IDSBuffer->Restore()))
        return false;

      // Try to lock the buffer again
      if (FAILED(m_IDSBuffer->Lock(m_WritePos,Bytes,
        (LPVOID*)&Buffer1,&Buffer1Len,(LPVOID*)&Buffer2,&Buffer2Len,0)))
        return false;
    }
    else
      return false;
  }

  // Write into the buffer
  WriteSampleData(Buffer1,Buffer1Len);
  if (Buffer2 != NULL)
    WriteSampleData(Buffer2,Buffer2Len);

  // Update the writing point
  m_WritePos += Buffer1Len + Buffer2Len;
  m_WritePos %= GetBufferSize();

  // Unlock the buffer
  if (FAILED(m_IDSBuffer->Unlock(Buffer1,Buffer1Len,Buffer2,Buffer2Len)))
    return false;
  return true;
}

// Destroy this sound's DirectSound buffer
void CDSound::DestroyBuffer(void)
{
  if (m_IDSBuffer != NULL)
  {
    m_IDSBuffer->Stop();
    m_IDSBuffer->Release();
  }
  m_IDSBuffer = NULL;

  m_WritePos = 0;
  m_Active = false;
  m_StartTime = 0;
}

// Set the volume for this sound's DirectSound buffer
void CDSound::SetBufferVolume(LONG Volume)
{
  if (m_IDSBuffer != NULL)
  {
    // The SetVolume() call to DirectSound requires a volume
    // in 100ths of a decibel.
    m_IDSBuffer->SetVolume(Volume);
  }
}

// Play the sound buffer
bool CDSound::PlayBuffer(bool PauseState)
{
  if (m_IDSBuffer == NULL)
    return false;

  if (!PauseState)
  {
    if (FAILED(m_IDSBuffer->Play(0,0,DSBPLAY_LOOPING)))
      return false;
  }
  m_Active = true;

  // Store the time at which the sound started playing
  m_StartTime = ::GetTickCount();

  // Add to the array of playing sounds
  CDSoundEngine::GetSoundEngine().AddToList(this);
  return true;
}

// Pause or restart the sound buffer
void CDSound::Pause(bool PauseState)
{
  if (m_IDSBuffer != NULL)
  {
    if (PauseState)
      m_IDSBuffer->Stop();
    else
      m_IDSBuffer->Play(0,0,DSBPLAY_LOOPING);
  }
}

// Get the status of the sound buffer
DWORD CDSound::GetStatus(void)
{
  if (m_IDSBuffer != NULL)
  {
    DWORD status = 0;
    if (SUCCEEDED(m_IDSBuffer->GetStatus(&status)))
      return status;
  }
  return 0;
}
