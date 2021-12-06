/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
FiveBandEQAudioProcessor::FiveBandEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

FiveBandEQAudioProcessor::~FiveBandEQAudioProcessor()
{
}

//==============================================================================
const juce::String FiveBandEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FiveBandEQAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool FiveBandEQAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool FiveBandEQAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double FiveBandEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int FiveBandEQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int FiveBandEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void FiveBandEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String FiveBandEQAudioProcessor::getProgramName (int index)
{
    return {};
}

void FiveBandEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void FiveBandEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    juce::dsp::ProcessSpec spec; //
    
    spec.maximumBlockSize = samplesPerBlock; // number of samples to be processed at one time
    
    spec.numChannels = 1; // mono channels can only handle one chain at a time
    
    spec.sampleRate = sampleRate;
    
    leftChain.prepare(spec);
    
    rightChain.prepare(spec);
    
    updateFilters();


}

void FiveBandEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FiveBandEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void FiveBandEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    updateFilters();

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    juce::dsp::AudioBlock<float> block(buffer);
    
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);
    
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
    
    leftChain.process(leftContext);
    rightChain.process(rightContext);

    
    
    // need to extract left and right channel from buffer
}

//==============================================================================
bool FiveBandEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* FiveBandEQAudioProcessor::createEditor()
{
//    return new FiveBandEQAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void FiveBandEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void FiveBandEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;
    
    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq")->load();
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq")->load();
    
    settings.peak1Freq = apvts.getRawParameterValue("Peak1 Freq")->load();
    settings.peak1GainInDecibels = apvts.getRawParameterValue("Peak1 Gain")->load();
    settings.peak1Quality = apvts.getRawParameterValue("Peak1 Quality")->load();
    
    settings.peak2Freq = apvts.getRawParameterValue("Peak2 Freq")->load();
    settings.peak2GainInDecibels = apvts.getRawParameterValue("Peak2 Gain")->load();
    settings.peak2Quality = apvts.getRawParameterValue("Peak2 Quality")->load();
    
    settings.peak3Freq = apvts.getRawParameterValue("Peak3 Freq")->load();
    settings.peak3GainInDecibels = apvts.getRawParameterValue("Peak3 Gain")->load();
    settings.peak3Quality = apvts.getRawParameterValue("Peak3 Quality")->load();
    
    settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue("LowCut Slope")->load());
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("HighCut Slope")->load());
    
    
    
    return settings;
}

void FiveBandEQAudioProcessor::updatePeakFilter(const ChainSettings &chainSettings)
{
    auto peak1Coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
                                                                                chainSettings.peak1Freq,
                                                                                chainSettings.peak1Quality,
                                                                                juce::Decibels::decibelsToGain(chainSettings.peak1GainInDecibels));
    
    auto peak2Coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
                                                                                chainSettings.peak2Freq,
                                                                                chainSettings.peak2Quality,
                                                                                juce::Decibels::decibelsToGain(chainSettings.peak2GainInDecibels));
    
    auto peak3Coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
                                                                                chainSettings.peak3Freq,
                                                                                chainSettings.peak3Quality,
                                                                                juce::Decibels::decibelsToGain(chainSettings.peak3GainInDecibels));
    
//    *leftChain.get<ChainPositions::Peak1>().coefficients = *peak1Coefficients;
//    *rightChain.get<ChainPositions::Peak1>().coefficients = *peak1Coefficients;
    updateCoefficients(leftChain.get<ChainPositions::Peak1>().coefficients, peak1Coefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak1>().coefficients, peak1Coefficients);
    
    
//    *leftChain.get<ChainPositions::Peak2>().coefficients = *peak2Coefficients;
//    *rightChain.get<ChainPositions::Peak2>().coefficients = *peak2Coefficients;
    updateCoefficients(leftChain.get<ChainPositions::Peak2>().coefficients, peak2Coefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak2>().coefficients, peak2Coefficients);
    
//    *leftChain.get<ChainPositions::Peak3>().coefficients = *peak3Coefficients;
//    *rightChain.get<ChainPositions::Peak3>().coefficients = *peak3Coefficients;
    updateCoefficients(leftChain.get<ChainPositions::Peak3>().coefficients, peak3Coefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak3>().coefficients, peak3Coefficients);
}

void FiveBandEQAudioProcessor::updateCoefficients(Coefficients &old, const Coefficients &replacements)
{
    *old = *replacements;
}

void FiveBandEQAudioProcessor::updateLowCutFilters(const ChainSettings &chainSettings)
{
    auto cutCoeficcients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq,
                                                                                                       getSampleRate(),
                                                                                                       2*(chainSettings.lowCutSlope + 1));
    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
    
    updateCutFilter(leftLowCut, cutCoeficcients, chainSettings.lowCutSlope);
    updateCutFilter(rightLowCut, cutCoeficcients, chainSettings.lowCutSlope);
}

void FiveBandEQAudioProcessor::updateHighCutFilters(const ChainSettings &chainSettings)
{
    auto highCutCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq,
                                                                                                          getSampleRate(),
                                                                                                          2*(chainSettings.highCutSlope + 1));
    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();
    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    
    updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);
    updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);
    
}

void FiveBandEQAudioProcessor::updateFilters()
{
    auto chainSettings = getChainSettings(apvts);
    
    updateLowCutFilters(chainSettings);
    updatePeakFilter(chainSettings);
    updateHighCutFilters(chainSettings);
}

juce::AudioProcessorValueTreeState::ParameterLayout FiveBandEQAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("LowCut Freq",
                                                           "LowCut Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                           20.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("HighCut Freq",
                                                           "HighCut Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                           20000.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak1 Freq",
                                                           "Peak1 Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                           350.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak1 Gain",
                                                           "Peak1 Gain",
                                                           juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
                                                           0.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak1 Quality",
                                                           "Peak1 Quality",
                                                           juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
                                                           1.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak2 Freq",
                                                           "Peak2 Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                           2000.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak2 Gain",
                                                           "Peak2 Gain",
                                                           juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
                                                           0.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak2 Quality",
                                                           "Peak2 Quality",
                                                           juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
                                                           1.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak3 Freq",
                                                           "Peak3 Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                           5000.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak3 Gain",
                                                           "Peak3 Gain",
                                                           juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
                                                           0.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak3 Quality",
                                                           "Peak3 Quality",
                                                           juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
                                                           1.0f));
    
    
    juce::StringArray stringArray;
    for(int i = 0; i < 4; i++)
    {
        juce::String str;
        str << (12 + i*12);
        str << " db/Oct";
        stringArray.add(str);
    }
    
    layout.add(std::make_unique<juce::AudioParameterChoice>("LowCut Slope", "LowCut Slope", stringArray, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("HighCut Slope", "HighCut Slope", stringArray, 0));
    // these two layout adds create a drop down to choose the slope for the plugin to change how intense the slope of the cutoff frequencies are
    
    // skew factor allows slider to move differently *more slide for lower end than higher end*
    
    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FiveBandEQAudioProcessor();
}
