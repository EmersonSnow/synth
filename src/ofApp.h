#pragma once

#include "ofMain.h"
#include "ofxMidi.h"


#define PANNING_TABLE_SIZE 16385
#define WAVETABLE_SIZE 65536
#define AUDIO_BUFFER_SIZE 4096
#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_CHANNEL_NUMBER 2

enum PanningType
{
    PanningLinear,
    PanningSquared,
    PanningSine
};
enum WaveType
{
    SineWave,
    SawtoothWave,
    TriangleWave,
    SquareWave
};

struct PanningData
{
    float left[PANNING_TABLE_SIZE];
    float right[PANNING_TABLE_SIZE];
};
struct Panning
{
    float left;
    float right;
};

class zelmSynthUtil
{
public:
    static void generatePanning()
    {
        float mappedIndex;
        for (int i = 0; i < PANNING_TABLE_SIZE; i++)
        {
            mappedIndex = (i/(((PANNING_TABLE_SIZE-1.0)/2.0))-1.0);
            panningLinear.left[i] = (1 - mappedIndex) / 2;
            panningLinear.right[i] = (1 + mappedIndex) / 2;
            
            panningSquared.left[i] = sqrt((1 - mappedIndex) / 2);
            panningSquared.right[i] = sqrt((1 + mappedIndex) / 2);
            
            panningSine.left[i] = sin((1 - mappedIndex)/2 * (PI/2));
            panningSine.right[i] = sin((1 + mappedIndex)/2 * (PI/2));
        }
    }
    
    static Panning getPanning(float value, PanningType panningType)
    {
        //float redemicalise = float((int(value*1000))/1000.0);
        int index = ofMap(value, -1.0, 1.0, 0, 2000, true);
        Panning temp;
        switch(panningType)
        {
            case PanningLinear:
            {
                temp.left = panningLinear.left[index];
                temp.right = panningLinear.right[index];
                return temp;
            }
            case PanningSquared:
            {
                temp.left = panningSquared.left[index];
                temp.right = panningSquared.right[index];
                return temp;
            }
            case PanningSine:
            {
                temp.left = panningSine.left[index];
                temp.right = panningSine.right[index];
                return temp;
            }
        }
    }
    
    static void generateWavetable()
    {
        float phaseIncrement = TWO_PI / WAVETABLE_SIZE;
        float phase = 0;
        for (int i = 0; i < WAVETABLE_SIZE+1; i++)
        {
            //wavetables.push_back(*new vector<float>);
            wavetable[i] = sin(phase);
            phase += phaseIncrement;
        }
    }
    
    /*static void generateSummedWavetables()
     {
     int numSummed = 2;
     float partMult[numSummed];
     float partAmp[numSummed];
     float phase[numSummed];
     float phaseInit[numSummed];
     float phaseIncrements[numSummed];
     
     
     
     }*/
    
    static float getWavetableValue(int index)
    {
        return wavetable[index];
    }
    
private:
    static PanningData panningLinear;
    static PanningData panningSquared;
    static PanningData panningSine;
    
    //static float wavetable[2][WAVETABLE_SIZE+1];
    static float wavetable[WAVETABLE_SIZE+1];
    //static vector<vector<float>> wavetables;
    
};
class SoundObject : public ofBaseSoundOutput
{
public:
    //SoundObject();
    virtual ~SoundObject() {}
    
    virtual void processAudio(ofSoundBuffer &input, ofSoundBuffer &output)
    {
        input.copyTo(output);
    }
    
    virtual void audioOut(ofSoundBuffer &out)
    {
        if (workingBuffer.size() != out.size())
        {
            workingBuffer.resize(out.size());
            workingBuffer.setNumChannels(out.getNumChannels());
            workingBuffer.setSampleRate(out.getSampleRate());
        }
        //inputObject->audioOut(workingBuffer);
        processAudio(workingBuffer, out);
        
        if (bRecord)
            record(out);
    }
    
    
    
    virtual void record(ofSoundBuffer &out)
    {
        for (int i = 0; i < out.size(); i++)
        {
            recordData.push_back(out.getBuffer()[i]);
        }
    }
    
    void startRecord()
    {
        recordData = *new vector<float>;
        bRecord = true;
    }
    
    void saveRecording(string path)
    {
        ofFile f(path);
        if(ofToLower(f.getExtension())!="wav") {
            path += ".wav";
            ofLogWarning() << "Can only write wav files - will save file as " << path;
        }
        
        fstream file(ofToDataPath(path).c_str(), ios::out | ios::binary);
        if(!file.is_open()) {
            ofLogError() << "Error opening sound file '" << path << "' for writing";
            return false;
        }
        
        // write a wav header
        short myFormat = 1; // for pcm
        int mySubChunk1Size = 16;
        int bitsPerSample = 16; // assume 16 bit pcm
        int myByteRate = AUDIO_SAMPLE_RATE * 2 * bitsPerSample/8;
        short myBlockAlign = 2 * bitsPerSample/8;
        int myChunkSize = 36 + recordData.size()*bitsPerSample/8;
        int myDataSize = recordData.size()*bitsPerSample/8;
        int channels = 2;
        int samplerate = AUDIO_SAMPLE_RATE;
        
        file.seekp (0, ios::beg);
        file.write ("RIFF", 4);
        file.write ((char*) &myChunkSize, 4);
        file.write ("WAVE", 4);
        file.write ("fmt ", 4);
        file.write ((char*) &mySubChunk1Size, 4);
        file.write ((char*) &myFormat, 2); // should be 1 for PCM
        file.write ((char*) &channels, 2); // # channels (1 or 2)
        file.write ((char*) &samplerate, 4); // 44100
        file.write ((char*) &myByteRate, 4); //
        file.write ((char*) &myBlockAlign, 2);
        file.write ((char*) &bitsPerSample, 2); //16
        file.write ("data", 4);
        file.write ((char*) &myDataSize, 4);
        
#define WRITE_BUFF_SIZE 4096
        
        short writeBuff[WRITE_BUFF_SIZE];
        int pos = 0;
        while(pos<recordData.size()) {
            int len = MIN(WRITE_BUFF_SIZE, recordData.size()-pos);
            for(int i = 0; i < len; i++) {
                writeBuff[i] = (int)(recordData[pos]*32767.f);
                pos++;
            }
            file.write((char*)writeBuff, len*bitsPerSample/8);
        }
        
        file.close();
    }
    
    
    
    bool bRecord = false;
    vector<float> recordData;
    ofSoundBuffer workingBuffer;
    SoundObject *inputObject;
    
private:
    
    
};

class zelmAudioMixer : public SoundObject
{
public:
    
    void addInput(SoundObject * soundObject)
    {
        mixerInputs.push_back(soundObject);
    }
    
    void audioOut(ofSoundBuffer &out)
    {
        if (mixerInputs.size() > 0)
        {
            for (int i = 0; i < mixerInputs.size(); i++)
            {
                if (mixerInputs[i] != NULL)
                {
                    ofSoundBuffer temp;
                    temp.resize(out.size());
                    temp.setNumChannels(out.getNumChannels());
                    temp.setSampleRate(out.getSampleRate());
                    mixerInputs[i]->audioOut(temp);
                    
                    int left = 0;
                    for (int right = 1; right < temp.size(); right+=2, left+=2)
                    {
                        out.getBuffer()[left] += temp.getBuffer()[left] * (1.0/mixerInputs.size()) * panning.left;
                        out.getBuffer()[right] += temp.getBuffer()[right] * (1.0/mixerInputs.size()) * panning.right;
                        /*if ((out.getBuffer()[b] > 1.0) || (out.getBuffer()[b] < -1.0))
                         {
                         cout << "Audio not noramlised " << out.getBuffer()[b] << "\n";
                         }*/
                    }
                }
            }
        }
        out *= masterVolume;
        
        if (bRecord)
            record(out);
    }
    
    void setMasterVolume(float masterVolume)
    {
        this->masterVolume = masterVolume;
    }
    
    void setPanning(float panning, PanningType panningType)
    {
        this->panning = zelmSynthUtil::getPanning(panning, panningType);
    }
private:
    float masterVolume = 1.0;
    vector<SoundObject*> mixerInputs;
    Panning panning;
    
};

class CurveExponentGenerator
{
public:
    void generate(float duration, float curvature, bool bAscending, int sampleRate)
    {
        durationSample = duration * (float)sampleRate;
        if (bAscending)
        {
            exponentMul = pow((curvature+1.0)/curvature, 1.0/float(durationSample));
            exponentNow = curvature;
            
        } else
        {
            exponentMul = pow(curvature/(1.0+curvature), 1.0/durationSample);
            exponentNow = curvature+1.0;
        }
        for (int i = 0; i < durationSample; i++)
        {
            exponentNow *= exponentMul;
            curve.push_back((exponentNow - curvature) * 1.0);
        }
        bGenerated = true;
    }
    void generateMappedCurve(float startVolume, float endVolume)
    {
        if (!bGenerated)
            return;
        if (startVolume < endVolume)
        {
            for (int i = 0; i < curve.size(); i++)
            {
                curveMapped.push_back(ofMap(curve[i], 0.0, 1.0, startVolume, endVolume, false));
            }
        } else
        {
            for (int i = 0; i < curve.size(); i++)
            {
                curveMapped.push_back(ofMap(curve[i], 1.0, 0.0, startVolume, endVolume, false));
            }
        }
    }
    
    vector<float> curve;
    vector<float> curveMapped;
    
private:
    bool bGenerated = false;
    int durationSample;
    float exponentMin;
    float exponentMul;
    float exponentNow;
};

/*class PlayFromWavetable : public SoundObject
 {
 public:
 void setWavetable(WavetableGenerator * _wavetableGenetator)
 {
 this->wavetableGenerator = _wavetableGenetator;
 }
 void processAudio(ofSoundBuffer &in, ofSoundBuffer &out)
 {
 int numFrames = out.getNumFrames();
 int numChannels = out.getNumChannels();
 for (int i = 0; i < numFrames; i++, wavetableIndex++)
 {
 for (int c = 0; c < numChannels; c++)
 {
 out[i*numChannels +c] = wavetableGenerator->wavetable[wavetableIndex];
 }
 if (wavetableIndex == (wavetableGenerator->wavetable.size()-1))
 {
 wavetableIndex = -1;
 }
 }
 }
 
 private:
 WavetableGenerator * wavetableGenerator;
 int wavetableIndex = 0;
 };*/

struct Segment
{
    bool bVolumeChange;
    int sampleDuration;
    float duration;
    float startVolume;
    float endVolume;
    CurveExponentGenerator curveExponentGenerator;
    
    int curveIndex;
};

class ToneGenerator3 : public SoundObject
{
public:
    void setup(float frequency, WaveType waveType, int sampleRate)
    {
        this->frequency = frequency;
        this->waveType = waveType;
        
        panning.left = 1.0;
        panning.right = 1.0;
        switch(waveType)
        {
            case SineWave:
            {
                //wavetableGenerator.generate(sampleRate);
                phase = 0;
                float freTI = WAVETABLE_SIZE / (float)sampleRate;
                phaseIncrement = freTI * frequency;
                break;
            }
            case SawtoothWave:
            {
                phase = -1;
                phaseIncrement = (2 * frequency) / sampleRate;
                break;
            }
            case TriangleWave:
            {
                phase = 0;
                phaseIncrement = (TWO_PI / sampleRate) * frequency;
                twoDividePI = 2.0/PI;
            }
        }
        
        bSoundAltered = true;
        bSoundSamplePeriodFinished = false;
    }
    void addSegment(float duration, float startVolume, float endVolume, float exponential, bool bSineWaveOscillator = false, int numberOscillator = 0)
    {
        Segment temp;
        temp.duration = duration;
        temp.startVolume = startVolume;
        temp.endVolume = endVolume;
        temp.sampleDuration = duration * AUDIO_SAMPLE_RATE;
        if (startVolume == endVolume)
        {
            temp.bVolumeChange = false;
        } else
        {
            temp.curveExponentGenerator.generate(duration, exponential,
                                                 (startVolume < endVolume) ? true : false, AUDIO_SAMPLE_RATE);
            temp.curveExponentGenerator.generateMappedCurve(startVolume, endVolume);
            temp.bVolumeChange = true;
        }
        segments.push_back(temp);
        
        bSoundAltered = true;
    }
    
    void setPanning(float panning, PanningType panningType)
    {
        this->panning = zelmSynthUtil::getPanning(panning, panningType);
        
        bSoundAltered = true;
    }
    
    void start()
    {
        segmentIndex = 0;
        bStart = true;
    }
    void processAudio(ofSoundBuffer &in, ofSoundBuffer &out)
    {
        if (!bStart)
            return;
        
        float frequencyRadian = TWO_PI / in.getSampleRate();
        int numFrames = out.getNumFrames();
        int numChannels = out.getNumChannels();
        
        for (int i = 0; i < numFrames; i++, currentSampleCount++)
        {
            if (!bSoundAltered && bSoundSamplePeriodFinished)
            {
                out[i*numChannels] = audioPeriod[audioPeriodCount];
                out[i*numChannels+1] = audioPeriod[audioPeriodCount+1];
                if ((audioPeriodCount += 2) >= audioPeriod.size())
                {
                    audioPeriodCount = 0;
                }
            } else
            {
                if (bSoundSamplePeriodFinished)
                {
                    bSoundSamplePeriodFinished = false;
                }
                if (bSoundAltered)
                {
                    audioPeriod.clear();
                    bSoundAltered = false;
                }
                switch(state)
                {
                    case 0:
                        segments[segmentIndex].curveIndex = 0;
                        if (!segments[segmentIndex].bVolumeChange)
                        {
                            volume = segments[segmentIndex].endVolume;
                            state = 2;
                        } else
                        {
                            state = 1;
                        }
                        break;
                    case 1:
                        if (segments[segmentIndex].sampleDuration >= currentSampleCount)
                        {
                            volume = segments[segmentIndex].curveExponentGenerator.curveMapped[segments[segmentIndex].curveIndex];
                            segments[segmentIndex].curveIndex++;
                            
                        } else
                        {
                            state = 3;
                        }
                        break;
                    case 2:
                        if (segments[segmentIndex].sampleDuration <= currentSampleCount)
                        {
                            state = 2;
                        }
                        break;
                    case 3:
                        volume = segments[segmentIndex].endVolume;
                        currentSampleCount = -1;
                        if (++segmentIndex == segments.size())
                        {
                            //segmentIndex = 0;
                            audioPeriodCount = 0;
                            //bSoundAltered = false;
                            bSoundSamplePeriodFinished = true;
                        }
                        state = 0;
                        break;
                }
                
                switch (waveType)
                {
                    case SineWave:
                    {
                        //Increased wavetable size by 4, to elimate this calcation;
                        /*wavetableIndexBase = floor(wavetableIndex);
                         wavetableIndexFract = wavetableIndex - wavetableIndexBase;
                         wavetableValue1 = zelmSynthUtil::getWavetableValue(wavetableIndex);
                         wavetableValue2 = zelmSynthUtil::getWavetableValue(wavetableIndex+1);
                         wavetableValue1 = wavetableValue1 + ((wavetableValue2 - wavetableValue1) * wavetableIndexFract);*/
                        //out[i*numChannels] = wavetableValue1 * volume * panning.left;
                        //out[i*numChannels+1] = wavetableValue1 * volume * panning.right;
                        //cout << panning.left << " " << panning.right << "\n";
                        out[i*numChannels] = zelmSynthUtil::getWavetableValue(wavetableIndex) * volume * panning.left;
                        out[i*numChannels+1] = zelmSynthUtil::getWavetableValue(wavetableIndex) * volume * panning.right;
                        if ((wavetableIndex += phaseIncrement) >= (WAVETABLE_SIZE))
                        {
                            wavetableIndex -= WAVETABLE_SIZE;
                        }
                        break;
                    }
                    case SawtoothWave:
                    {
                        out[i*numChannels] = phase * volume * panning.left;
                        out[i*numChannels+1] = phase * volume * panning.right;
                        phase += phaseIncrement;
                        if (phase >= 1)
                        {
                            phase -= 2;
                        }
                        break;
                    }
                    case TriangleWave:
                    {
                        triangleValue = (phase * twoDividePI);
                        if (phase < 0)
                        {
                            triangleValue = 1.0 + triangleValue;
                        } else
                        {
                            triangleValue = 1.0 - triangleValue;
                        }
                        out[i*numChannels] = triangleValue * volume * panning.left;
                        out[i*numChannels+1] = triangleValue * volume * panning.right;
                        
                        if ((phase += phaseIncrement) >= PI)
                        {
                            phase -= TWO_PI;
                        }
                    }
                }
                audioPeriod.push_back(out[i*numChannels]);
                audioPeriod.push_back(out[i*numChannels+1]);
            }
        }
    }
    
    /*float attackTime;
     float attackStartVolume;
     float decayTime;
     float decayEndVolume;
     float peakVolume;*/
    
private:
    bool bSoundAltered = false;
    bool bSoundSamplePeriodFinished = false;
    //bool bPlayingFromSavedTable = false;
    
    int audioPeriodCount;
    vector<float> audioPeriod;
    
    bool bStart = false;
    
    WaveType waveType;
    
    int state;
    int currentSampleCount;
    int segmentIndex;
    
    float frequency;
    float duration;
    float phase;
    float phaseIncrement;
    
    float volume;
    Panning panning;
    //WavetableGenerator wavetableGenerator;
    float wavetableIndex = 0;
    /*int wavetableIndexBase;
     float wavetableIndexFract;
     float wavetableValue1;
     float wavetableValue2;*/
    
    float twoDividePI;
    float triangleValue;
    //float wavetableIncrement;
    
    vector<Segment> segments;
    
    
};

class MidiController : public ofxMidiListener
{
    public:
        void setup(WaveType waveType, int sampleRate, float totalDuration, float attackDuration, float decayDuration, float cutoffDuration)
        {
        
        }
        void newMidiMessage(ofxMidiMessage& eventArgs)
        {
            midiMessage = eventArgs;
        }
    private:
        ofxMidiIn midiIn;
        ofxMidiMessage midiMessage;
    
        WaveType waveType;
        int sampleRate;
        float totalDuration;
        float attackDuation;
        float decayDuration;
        float cutoffDuration;
    
        zelmAudioMixer audioMixer;
        vector<ToneGenerator3> toneGenerators;
    
};

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		
};
