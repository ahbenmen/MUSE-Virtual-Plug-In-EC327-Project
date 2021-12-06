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
struct LookAndFeel : juce::LookAndFeel_V4
{
  void drawRotarySlider (juce::Graphics&,
                          int x, int y, int width, int height, float sliderPosProportional,
                          float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;
};
struct RotarySliderWithLabels : juce::Slider{
  RotarySliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix) : param(&rap), suffix(unitSuffix), juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox){
    setLookAndFeel(&lnf);
  }
  ~RotarySliderWithLabels(){
    setLookAndFeel(nullptr);
  }
  struct LabelPos{
    float pos;
    juce::String label;
  };
  juce::Array<LabelPos> labels;
  void paint(juce::Graphics& g) override;
  juce::Rectangle<int> getSliderBounds() const;
  int getTextHeight() const {return 14; }
  juce::String getDisplayString() const;
private:
  LookAndFeel lnf;
  juce::RangedAudioParameter* param;
  juce::String suffix;
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

    

    RotarySliderWithLabels peak1FreqSlider,
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
