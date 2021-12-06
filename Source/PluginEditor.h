/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/

struct CustomRotarySlider : juce::Slider{
  CustomRotarySlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox){

  }
};

struct ResponseCurveComponent: public juce::Component,
juce::AudioProcessorParameter::Listener, juce::Timer
{
  ResponseCurveComponent(FiveBandEQAudioProcessor&);
  ~ResponseCurveComponent();
  void parameterValueChanged (int parameterIndex, float newValue) override;
  void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {};
  void timerCallback() override;
  void paint(juce::Graphics& g) override;
private:
  FiveBandEQAudioProcessor& audioProcessor;
  juce::Atomic<bool> parametersChanged {false};
  MonoChain monoChain;
};

class FiveBandEQAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    FiveBandEQAudioProcessorEditor (FiveBandEQAudioProcessor&);
    ~FiveBandEQAudioProcessorEditor() override;

    //==============================================================================
    void resized() override;
    
private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    FiveBandEQAudioProcessor& audioProcessor;

    

    CustomRotarySlider peak1FreqSlider,
    peak2FreqSlider,
    peak3FreqSlider,
    peak1GainSlider,
    peak2GainSlider,
    peak3GainSlider,
    peak1QualitySlider,
    peak2QualitySlider,
    peak3QualitySlider,
    lowCutFreqSlider,
    highCutFreqSlider,
    lowCutSlopeSlider,
    highCutSlopeSlider;

    ResponseCurveComponent responseCurveComponent;

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;
    Attachment peak1FreqSliderAttachment,
               peak2FreqSliderAttachment,
               peak3FreqSliderAttachment,
               peak1GainSliderAttachment,
               peak2GainSliderAttachment,
               peak3GainSliderAttachment,
               peak1QualitySliderAttachment,
               peak2QualitySliderAttachment,
               peak3QualitySliderAttachment,
               lowCutFreqSliderAttachment,
               highCutFreqSliderAttachment,
               lowCutSlopeSliderAttachment,
               highCutSlopeSliderAttachment;

    std::vector<juce::Component*> getComps();



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FiveBandEQAudioProcessorEditor)
};
