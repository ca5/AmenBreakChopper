/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AmenBreakChopperAudioProcessor::AmenBreakChopperAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       mValueTreeState(*this, nullptr, "PARAMETERS", createParameterLayout())
#endif
{
}

AmenBreakChopperAudioProcessor::~AmenBreakChopperAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AmenBreakChopperAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterInt>("delayTime", "Delay Time", 0, 15, 0));

    return layout;
}

juce::AudioProcessorValueTreeState& AmenBreakChopperAudioProcessor::getValueTreeState()
{
    return mValueTreeState;
}

const juce::String AmenBreakChopperAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AmenBreakChopperAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AmenBreakChopperAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AmenBreakChopperAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AmenBreakChopperAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AmenBreakChopperAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AmenBreakChopperAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AmenBreakChopperAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String AmenBreakChopperAudioProcessor::getProgramName (int index)
{
    return {};
}

void AmenBreakChopperAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void AmenBreakChopperAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mSampleRate = sampleRate;

    const int numChannels = getTotalNumInputChannels();
    // Calculate buffer size for 16 eighth notes at a very slow BPM (e.g., 30 BPM)
    // to ensure the buffer is always large enough.
    // Max delay = 16 * (1/8 note) = 2 whole notes = 8 beats.
    // Duration of 8 beats at 30 BPM = 8 * (60 / 30) = 8 * 2 = 16 seconds.
    const double maxDelayTimeInSeconds = 16.0;
    const int delayBufferSize = static_cast<int>(maxDelayTimeInSeconds * sampleRate);

    mDelayBuffer.setSize(numChannels, delayBufferSize);
    mDelayBuffer.clear();
}

void AmenBreakChopperAudioProcessor::releaseResources()
{
    mDelayBuffer.setSize(0, 0);
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AmenBreakChopperAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void AmenBreakChopperAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const int bufferLength = buffer.getNumSamples();
    const int delayBufferLength = mDelayBuffer.getNumSamples();

    double bpm = 120.0;
    if (auto* playHead = getPlayHead())
    {
        if (auto positionInfo = playHead->getPosition())
        {
            if (positionInfo->getBpm().hasValue())
                bpm = *positionInfo->getBpm();
        }
    }

    auto* delayTimeParam = mValueTreeState.getRawParameterValue("delayTime");
    const int currentDelayTime = static_cast<int>(delayTimeParam->load());

    double eighthNoteTime = (60.0 / bpm) / 2.0;
    int delayTimeInSamples = static_cast<int>(eighthNoteTime * currentDelayTime * mSampleRate);

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        const float* bufferData = buffer.getReadPointer(channel);
        float* delayBufferData = mDelayBuffer.getWritePointer(channel);
        auto* channelData = buffer.getWritePointer (channel);

        for (int i = 0; i < bufferLength; ++i)
        {
            const int readPosition = (mWritePosition - delayTimeInSamples + i + delayBufferLength) % delayBufferLength;
            const float delayedSample = delayBufferData[readPosition];

            const float inputSample = bufferData[i];
            float* writePointer = &delayBufferData[(mWritePosition + i) % delayBufferLength];
            *writePointer = inputSample; // No feedback

            channelData[i] = delayedSample; // 100% Wet
        }
    }

    mWritePosition = (mWritePosition + bufferLength) % delayBufferLength;
}

//==============================================================================
bool AmenBreakChopperAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AmenBreakChopperAudioProcessor::createEditor()
{
    return new AmenBreakChopperAudioProcessorEditor (*this);
}

//==============================================================================
void AmenBreakChopperAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = mValueTreeState.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AmenBreakChopperAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(mValueTreeState.state.getType()))
            mValueTreeState.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AmenBreakChopperAudioProcessor();
}
