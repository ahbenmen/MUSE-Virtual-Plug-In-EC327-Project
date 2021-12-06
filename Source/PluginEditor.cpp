/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
ResponseCurveComponent::ResponseCurveComponent(FiveBandEQAudioProcessor& p) : audioProcessor(p)
{
    const auto& params = audioProcessor.getParameters();
    for(auto param: params){
        param->addListener(this);
    }

    startTimerHz(60);
}
ResponseCurveComponent::~ResponseCurveComponent()
{
    const auto& params = audioProcessor.getParameters();
    for(auto param: params){
        param->removeListener(this);
    }
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void ResponseCurveComponent::timerCallback()
{
    if( parametersChanged.compareAndSetBool(false,true))
    {
        //update monochain
        auto chainSettings=getChainSettings(audioProcessor.apvts);
        auto peak1Coefficients=makePeak1Filter(chainSettings, audioProcessor.getSampleRate());
        auto peak2Coefficients=makePeak2Filter(chainSettings, audioProcessor.getSampleRate());
        auto peak3Coefficients=makePeak3Filter(chainSettings, audioProcessor.getSampleRate());
        updateCoefficients(monoChain.get<ChainPositions::Peak1>().coefficients,peak1Coefficients);
        updateCoefficients(monoChain.get<ChainPositions::Peak2>().coefficients,peak2Coefficients);
        updateCoefficients(monoChain.get<ChainPositions::Peak3>().coefficients,peak3Coefficients);
        auto lowCutCoefficients= makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
        auto highCutCoefficients= makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
        updateCutFilter(monoChain.get<ChainPositions::LowCut>(),lowCutCoefficients, chainSettings.lowCutSlope);
        updateCutFilter(monoChain.get<ChainPositions::HighCut>(),highCutCoefficients, chainSettings.highCutSlope);
        //signal new draw of response curve
        repaint();
    }
}
void ResponseCurveComponent::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::black);

    auto responseArea = getLocalBounds();

    auto w=responseArea.getWidth();

    auto& lowcut = monoChain.get<ChainPositions::LowCut>();
    auto& peak1 = monoChain.get<ChainPositions::Peak1>();
    auto& peak2 = monoChain.get<ChainPositions::Peak2>();
    auto& peak3 = monoChain.get<ChainPositions::Peak3>();
    auto& highcut = monoChain.get<ChainPositions::HighCut>();

    auto sampleRate= audioProcessor.getSampleRate();

    std::vector<double> mags;

    mags.resize(w);

    for(int i=0; i<w; ++i){
        double mag=1.f;
        auto freq = mapToLog10(double(i)/double(w), 20.0,20000.0);

        if(! monoChain.isBypassed<ChainPositions::Peak1>() )
            mag*=peak1.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(! monoChain.isBypassed<ChainPositions::Peak2>() )
            mag*=peak2.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(! monoChain.isBypassed<ChainPositions::Peak3>() )
            mag*=peak3.coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if(! lowcut.isBypassed<0>() )
            mag*=lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(! lowcut.isBypassed<1>() )
            mag*=lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(! lowcut.isBypassed<2>() )
            mag*=lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(! lowcut.isBypassed<3>() )
            mag*=lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if(! highcut.isBypassed<0>() )
            mag*=highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(! highcut.isBypassed<1>() )
            mag*=highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(! highcut.isBypassed<2>() )
            mag*=highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(! highcut.isBypassed<3>() )
            mag*=highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        mags[i]=Decibels::gainToDecibels(mag);
    }

    Path responseCurve;

    const double outputMin = responseArea.getBottom();
    const double outputMax=responseArea.getY();
    auto map = [outputMin, outputMax](double input)
    {
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };

    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));

    for( size_t i=1; i<mags.size(); ++i)
    {
        responseCurve.lineTo(responseArea.getX()+i, map(mags[i]));
    }

    g.setColour(Colours::white);
    g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);
    g.setColour(Colours::blue);
    g.strokePath(responseCurve, PathStrokeType(2.f));
}
//==============================================================================
FiveBandEQAudioProcessorEditor::FiveBandEQAudioProcessorEditor (FiveBandEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
responseCurveComponent(audioProcessor),
peak1FreqSliderAttachment(audioProcessor.apvts, "Peak1 Freq", peak1FreqSlider),
peak1GainSliderAttachment(audioProcessor.apvts, "Peak1 Gain", peak1GainSlider),
peak1QualitySliderAttachment(audioProcessor.apvts, "Peak1 Quality", peak1QualitySlider),
peak2FreqSliderAttachment(audioProcessor.apvts, "Peak2 Freq", peak2FreqSlider),
peak2GainSliderAttachment(audioProcessor.apvts, "Peak2 Gain", peak2GainSlider),
peak2QualitySliderAttachment(audioProcessor.apvts, "Peak2 Quality", peak2QualitySlider),
peak3FreqSliderAttachment(audioProcessor.apvts, "Peak3 Freq", peak3FreqSlider),
peak3GainSliderAttachment(audioProcessor.apvts, "Peak3 Gain", peak3GainSlider),
peak3QualitySliderAttachment(audioProcessor.apvts, "Peak3 Quality", peak3QualitySlider),
lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCutSlope", lowCutSlopeSlider),
highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCutSlope", highCutSlopeSlider)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    for( auto* comp : getComps () )
    {
        addAndMakeVisible(comp);
    }
    
    setSize (1000, 800);
}

FiveBandEQAudioProcessorEditor::~FiveBandEQAudioProcessorEditor()
{
    
}

//==============================================================================




void FiveBandEQAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * .33);
    responseCurveComponent.setBounds(responseArea);
    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth()*.20);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * .25);

    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight()*.5));
    lowCutSlopeSlider.setBounds(lowCutArea);
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight()*.5));
    highCutSlopeSlider.setBounds(highCutArea);

    auto peakArea = bounds;

    auto peak1Area = peakArea.removeFromLeft(peakArea.getWidth()*.33);
    auto peak3Area= peakArea.removeFromRight(peakArea.getWidth()*.5);

    peak2FreqSlider.setBounds(peakArea.removeFromTop(peakArea.getHeight() * .33));
    peak2GainSlider.setBounds(peakArea.removeFromTop(peakArea.getHeight() * .33));
    peak2QualitySlider.setBounds(peakArea);


    peak1FreqSlider.setBounds(peak1Area.removeFromTop(peak1Area.getHeight() * .33));
    peak1GainSlider.setBounds(peak1Area.removeFromTop(peak1Area.getHeight() * .33));
    peak1QualitySlider.setBounds(peak1Area);

    
    peak3FreqSlider.setBounds(peak3Area.removeFromTop(peak3Area.getHeight() * .33));
    peak3GainSlider.setBounds(peak3Area.removeFromTop(peak3Area.getHeight() * .33));
    peak3QualitySlider.setBounds(peak3Area);


}










std::vector<juce::Component*> FiveBandEQAudioProcessorEditor::getComps()
{
    return
    {
        &peak1FreqSlider,
        &peak2FreqSlider,
        &peak3FreqSlider,
        &peak1GainSlider,
        &peak2GainSlider,
        &peak3GainSlider,
        &peak1QualitySlider,
        &peak2QualitySlider,
        &peak3QualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider,
        &responseCurveComponent
    };
}